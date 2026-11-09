"""JSON interface to SixParSolve for annual CEC module degradation."""

import argparse
import contextlib
import json
import math
import multiprocessing
import os
from pathlib import Path
import sys


CIRCUIT_KEYS = ("a", "IL", "Io", "Rs", "Rsh", "Adj")
IV_KEYS = ("Vmp", "Imp", "Voc", "Isc")


def finite_number(value, name):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(f"{name} must be a finite number")
    if not math.isfinite(value):
        raise ValueError(f"{name} must be a finite number")
    return float(value)


def degradation_rates(values, years, name):
    if not isinstance(values, list) or len(values) not in (1, years):
        raise ValueError(f"{name} must contain one rate or analysis_period rates")
    rates = [finite_number(value, name) for value in values]
    if any(value < 0 or value >= 100 for value in rates):
        raise ValueError(f"{name} rates must be in [0, 100) percent")
    if len(rates) > 1 and rates[0] != 0:
        raise ValueError(f"{name}[0] must be zero: year 1 is the baseline")
    return rates * years if len(rates) == 1 else rates


def solve_year(module, guess, ipopt):
    import SixParSolve as sixpar
    import pyomo.environ as pyo
    from pyomo.opt import check_optimal_termination

    solver = pyo.SolverFactory("ipopt", executable=ipopt)
    if not solver.available(exception_flag=False):
        raise RuntimeError("IPOPT executable is unavailable")
    solver.options["max_iter"] = sixpar.MAX_ITER
    model = sixpar.create_model(gamma_curve_dt=3)
    row = {
        "V_mp_ref": module["Vmp"], "I_mp_ref": module["Imp"],
        "V_oc_ref": module["Voc"], "I_sc_ref": module["Isc"],
        "alpha_sc": module["alpha_sc"], "beta_oc": module["beta_oc"],
        "gamma_pmp": module["gamma_pmp"],
    }
    if not sixpar.set_parameters(model.solver, row):
        raise RuntimeError("SixParSolve could not set module parameters")
    sixpar.set_empirical_initial_guess(model)
    sixpar.set_initial_guess(model, *(guess[key] for key in CIRCUIT_KEYS))
    result, scaled_model, code = sixpar.solve_model(model, solver, tee=False)
    if result is None or not check_optimal_termination(result):
        raise RuntimeError(f"SixParSolve did not converge (code {code})")
    residuals = sixpar.get_constraint_infeas(model)
    if any(not math.isfinite(value) or value > 1e-5 for value in residuals):
        raise RuntimeError("SixParSolve returned an infeasible solution")
    return dict(zip(CIRCUIT_KEYS, sixpar.get_params_from_model(model)))


def run_request(request, solve=solve_year, ipopt=None):
    years = request["analysis_period"]
    if isinstance(years, bool) or not isinstance(years, int) or years < 1:
        raise ValueError("analysis_period must be a positive integer")
    voltage_rates = degradation_rates(request["V_deg"], years, "V_deg")
    current_rates = degradation_rates(request["I_deg"], years, "I_deg")
    baseline = {
        key: finite_number(request["cec_model"][key], key)
        for key in IV_KEYS + CIRCUIT_KEYS + ("alpha_sc", "beta_oc", "gamma_pmp")
    }
    if any(baseline[key] <= 0 for key in IV_KEYS + ("a", "IL", "Io", "Rsh")):
        raise ValueError("IV points, a, IL, Io and Rsh must be positive")
    if baseline["Rs"] < 0 or baseline["Voc"] <= baseline["Vmp"] or baseline["Isc"] < baseline["Imp"]:
        raise ValueError("Invalid CEC module IV points or series resistance")
    results = []
    voltage_factor = current_factor = 1.0
    guess = baseline
    for year in range(years):
        module = dict(baseline)
        if year:
            voltage_factor *= 1 - voltage_rates[year] / 100
            current_factor *= 1 - current_rates[year] / 100
        for key in ("Vmp", "Voc", "beta_oc"):
            module[key] *= voltage_factor
        for key in ("Imp", "Isc", "alpha_sc"):
            module[key] *= current_factor
        if year and (voltage_factor != 1 or current_factor != 1):
            try:
                module.update(solve(module, guess, ipopt))
            except Exception as error:
                raise RuntimeError(f"Year {year + 1}: {error}") from error
        for key in CIRCUIT_KEYS:
            module[key] = finite_number(module[key], key)
        if any(module[key] <= 0 for key in ("a", "IL", "Io", "Rsh")) or module["Rs"] < 0:
            raise RuntimeError(f"Year {year + 1}: invalid circuit parameters")
        results.append({"year": year + 1, **{key: module[key] for key in IV_KEYS + CIRCUIT_KEYS + ("alpha_sc", "beta_oc")}})
        guess = module
    return {"status": "success", "years": results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ipopt", help="Path to IPOPT (defaults to bundled solver or PATH)")
    parser.add_argument("--input", type=Path, help="Read request JSON from this file instead of stdin")
    parser.add_argument("--output", type=Path, help="Write response JSON to this file instead of stdout")
    args = parser.parse_args()
    bundled = Path(getattr(sys, "_MEIPASS", Path(__file__).parent)) / "ipopt" / ("ipopt.exe" if os.name == "nt" else "ipopt")
    ipopt = args.ipopt or (str(bundled) if bundled.is_file() else None)
    exit_code = 0
    try:
        if args.input:
            with args.input.open(encoding="utf-8") as source:
                request = json.load(source)
        else:
            request = json.load(sys.stdin)
        with contextlib.redirect_stdout(sys.stderr):
            response = run_request(request, ipopt=ipopt)
    except Exception as error:
        response = {"status": "error", "message": str(error)}
        exit_code = 1
    payload = json.dumps(response, allow_nan=False) + "\n"
    if args.output:
        args.output.write_text(payload, encoding="utf-8")
    else:
        sys.stdout.write(payload)
    return exit_code


if __name__ == "__main__":
    multiprocessing.freeze_support()
    sys.exit(main())