import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from cec_degradation import CIRCUIT_KEYS, run_request


class DegradationTests(unittest.TestCase):
    def request(self, years=3):
        return {
            "analysis_period": years, "V_deg": [1], "I_deg": [2],
            "cec_model": {
                "Vmp": 30, "Imp": 8, "Voc": 38, "Isc": 9,
                "a": 1.5, "IL": 9.1, "Io": 1e-10, "Rs": 0.3,
                "Rsh": 300, "Adj": 2, "alpha_sc": 0.004,
                "beta_oc": -0.12, "gamma_pmp": -0.4,
            },
        }

    def test_baseline_and_compounding(self):
        request = self.request()
        calls = []

        def solve(module, guess, ipopt):
            calls.append(dict(module))
            return {key: guess[key] for key in CIRCUIT_KEYS}

        years = run_request(request, solve=solve)["years"]
        self.assertEqual(len(years), 3)
        self.assertEqual(len(calls), 2)
        self.assertEqual(years[0]["Vmp"], 30)
        self.assertAlmostEqual(years[2]["Vmp"], 30 * 0.99 ** 2)
        self.assertAlmostEqual(years[2]["Isc"], 9 * 0.98 ** 2)
        self.assertAlmostEqual(years[2]["alpha_sc"], 0.004 * 0.98 ** 2)
        self.assertEqual(set(years[0]), {"year", "Vmp", "Imp", "Voc", "Isc", *CIRCUIT_KEYS, "alpha_sc", "beta_oc"})

    def test_annual_schedule(self):
        request = self.request()
        request["V_deg"] = [0, 1, 3]
        request["I_deg"] = [0, 0, 0]
        years = run_request(request, solve=lambda module, guess, ipopt: {})["years"]
        self.assertAlmostEqual(years[2]["Voc"], 38 * 0.99 * 0.97)

    def test_zero_degradation_does_not_require_solver(self):
        request = self.request()
        request["V_deg"] = request["I_deg"] = [0]
        response = run_request(request, solve=lambda *args: self.fail("Unexpected solve"))
        self.assertEqual(response["years"][2]["Vmp"], 30)

    def test_invalid_inputs(self):
        for field, value in (("analysis_period", 0), ("V_deg", []),
                             ("V_deg", [100]), ("I_deg", [float("nan")]),
                             ("I_deg", [2, 2, 2]), ("V_deg", [1, 2])):
            with self.subTest(field=field, value=value):
                request = self.request()
                request[field] = value
                with self.assertRaises(ValueError):
                    run_request(request)

    def test_solver_failure_identifies_year(self):
        def fail(*args):
            raise RuntimeError("no convergence")

        with self.assertRaisesRegex(RuntimeError, "Year 2: no convergence"):
            run_request(self.request(), solve=fail)

    def test_file_protocol(self):
        with tempfile.TemporaryDirectory(prefix="cec worker test ") as directory:
            source = Path(directory) / "request.json"
            target = Path(directory) / "response.json"
            source.write_text(json.dumps(self.request(1)), encoding="utf-8")
            result = subprocess.run(
                [sys.executable, str(Path(__file__).with_name("cec_degradation.py")),
                 "--input", str(source), "--output", str(target)],
                capture_output=True, text=True, check=True,
            )
            response = json.loads(target.read_text(encoding="utf-8"))
            self.assertEqual(response["status"], "success")
            self.assertEqual(response["years"][0]["Vmp"], 30)
            self.assertEqual(result.stdout, "")

    def test_cli_error_is_json_and_nonzero(self):
        result = subprocess.run(
            [sys.executable, str(Path(__file__).with_name("cec_degradation.py"))],
            input="{}", capture_output=True, text=True,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertEqual(json.loads(result.stdout)["status"], "error")


if __name__ == "__main__":
    unittest.main()