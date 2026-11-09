# CEC Voltage/Current Degradation: Change Summary

## Purpose

Added a standalone Python worker and SSC integration to calculate annual CEC
module parameters from voltage/current degradation. The worker uses the existing
external `SixParSolve.py` Pyomo model and is packaged with IPOPT using PyInstaller;
SSC does not require an installed Python interpreter to run the executable.

## Changes Made

- **Python worker:** [cec_degradation.py](cec_degradation.py) accepts the analysis
  period, `V_deg`, `I_deg`, baseline CEC parameters, and temperature coefficients.
  It returns JSON containing `Vmp`, `Imp`, `Voc`, `Isc`, `a`, `IL`, `Io`, `Rs`,
  `Rsh`, and `Adj` for each year, plus the year index, `alpha_sc`, and `beta_oc`.
  Both stdin/stdout and `--input`/`--output` file transport are supported.
- **Packaging:** [build.py](build.py) bundles the supplied model source, Python
  dependencies, IPOPT, and solver runtime libraries into a single executable.
  Added [requirements.txt](requirements.txt) and ignore rules for generated
  artifacts and the isolated build environment. The external model source was
  not modified or copied into SSC source control.
- **SSC handoff:** [lib_pv_io_manager.cpp](../../shared/lib_pv_io_manager.cpp)
  initializes the baseline model before launching the worker, uses a unique
  temporary request/response directory per call, validates the complete JSON
  response, and populates the yearly `cecModels` vector. Invalid or incomplete
  results are rejected rather than partially applied.
- **Annual selection:** Added the missing `setAnalysisYear` declaration in
  [lib_pv_io_manager.h](../../shared/lib_pv_io_manager.h), bounds checking, and
  updates to module voltage and self-shading fill factor when switching years.
- **SSC inputs:** Registered `use_voltage_current_degradation`,
  `voltage_degradation`, `current_degradation`, `cec_degradation_executable`, and
  `cec_gamma_pmp` in [cmod_pvsamv1.cpp](../../ssc/cmod_pvsamv1.cpp), retaining
  the existing annual-loop model-switching hook.

## Behavior and Configuration

This option requires CEC database mode, lifetime output, and a positive analysis
period. Set `cec_degradation_executable` to the executable's absolute path.
`cec_gamma_pmp` is required because the baseline CEC model object does not store it.

Year 1 preserves the supplied baseline. Later years compound annual percentage
losses. Supply either one repeating rate or one entry per analysis year, with
the first schedule entry zero. Voltage losses scale Vmp, Voc, and beta_oc;
current losses scale Imp, Isc, and alpha_sc. gamma_pmp remains constant.
Degraded years are refitted using the previous year's solution as the initial
guess; only converged solutions passing a constraint-residual check are accepted.
These losses are additional to existing DC degradation inputs, so overlapping
losses must be disabled to avoid double counting.

## Verification and Remaining Work

- Seven automated tests passed for annual schedules, compounding, validation,
  solver errors, and JSON transport.
- Built [cec_degradation.exe](dist/cec_degradation.exe) and verified a three-year
  solve with bundled IPOPT while Python and solver directories were absent from
  `PATH`. Packaged file-mode error handling also passed.
- The updated C++ shared-library target compiled successfully. Full SSC linking
  was blocked by unresolved CSP/battery symbols outside this change, so the
  complete SSC simulation workflow remains unverified.
- SAM distribution/installer integration is not included. Other operating
  systems require separate builds; solver redistribution licenses need review.

See [README.md](README.md) for build instructions and the JSON contract.