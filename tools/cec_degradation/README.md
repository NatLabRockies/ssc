# CEC Degradation Worker

This worker calls the existing `SixParSolve.py` Pyomo model to refit a CEC
module after annual voltage/current degradation. It has no SAM UI dependency.
The original model source is supplied at build time, not copied into SSC.

## Build

Use a Python environment containing the model dependencies and PyInstaller:

```powershell
python -m venv tools/cec_degradation/.venv
tools/cec_degradation/.venv/Scripts/python.exe -m pip install -r tools/cec_degradation/requirements.txt
tools/cec_degradation/.venv/Scripts/python.exe tools/cec_degradation/build.py --sixpar "C:\path\to\SixParSolve.py" --ipopt-dir "C:\path\to\ipopt\bin"
```

The IPOPT directory must contain `ipopt.exe` (or `ipopt` on Unix) and its
runtime libraries. Its files are bundled into the executable. Verify the
solver distribution's redistribution licenses before distributing it.
Build separately on each target OS. Output is
`tools/cec_degradation/dist/cec_degradation.exe` on Windows.

Set SSC input `cec_degradation_executable` to its absolute path. SSC only
launches it when `use_voltage_current_degradation=1`, which requires the CEC
database module model and lifetime output. `cec_gamma_pmp` (%/C) is also
required: it is not stored in `cec6par_module_t` but is necessary for fitting.

## Protocol

Read a single JSON request on stdin and write one JSON response on stdout.
Alternatively use `--input request.json --output response.json`. Diagnostics
go to stderr. Errors return a nonzero exit code and an error JSON object.
`--ipopt PATH` overrides the bundled solver for source-level testing.

```json
{
  "analysis_period": 3,
  "V_deg": [0.2],
  "I_deg": [0.3],
  "cec_model": {
    "Vmp": 30, "Imp": 8, "Voc": 38, "Isc": 9,
    "a": 1.5, "IL": 9.1, "Io": 1e-10,
    "Rs": 0.3, "Rsh": 300, "Adj": 2,
    "alpha_sc": 0.004, "beta_oc": -0.12, "gamma_pmp": -0.4
  }
}
```

Degradation rates are percentages, not fractions or remaining-capacity
factors. One rate repeats annually from year 2 onward. A schedule must have
`analysis_period` entries with its first entry zero (year 1 is the baseline).
Each later entry is that year's additional loss, compounded with prior years.
For example `V_deg=[1]` gives voltage factors `1, 0.99, 0.9801`.
Both Vmp/Voc scale with the voltage factor; Imp/Isc scale with the current
factor. Absolute beta_oc/alpha_sc scale with their respective factors,
preserving their relative temperature coefficients. gamma_pmp stays constant.
The baseline circuit parameters seed year 2; each solution seeds the next year.
Year 1 and zero-degradation years reuse the baseline without an IPOPT call.

Success is `{"status":"success","years":[...]}` with exactly one record per
analysis year. Records contain `year` (1-based), `Vmp`, `Imp`, `Voc`, `Isc`,
`a`, `IL`, `Io`, `Rs`, `Rsh`, `Adj`, `alpha_sc`, and `beta_oc`. The latter two
are needed to apply each year's fitted CEC model consistently in SSC.
Failure is `{"status":"error","message":"Year 2: ..."}`; partial results
are never applied. Only converged solutions passing a constraint-residual
check are accepted; the model's approximate-fit fallback is not used.

These voltage/current losses are additional to existing `dc_degradation` and
other lifetime losses. Set overlapping loss inputs to zero to avoid counting
the same physical degradation twice.

## Tests

```powershell
python -m unittest discover -s tools/cec_degradation -v
```

Protocol tests inject a solver and do not require Pyomo or IPOPT. A real-solver
and packaged-executable smoke test are also needed before deployment.