"""Build the CEC worker with an external SixParSolve source and IPOPT distribution."""

import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sixpar", required=True, type=Path, help="Path to SixParSolve.py")
    parser.add_argument("--ipopt-dir", required=True, type=Path, help="Directory containing IPOPT and its shared libraries")
    args = parser.parse_args()
    sixpar = args.sixpar.resolve()
    ipopt_dir = args.ipopt_dir.resolve()
    ipopt_name = "ipopt.exe" if os.name == "nt" else "ipopt"
    if sixpar.name != "SixParSolve.py" or not sixpar.is_file():
        parser.error("--sixpar must point to SixParSolve.py")
    if not (ipopt_dir / ipopt_name).is_file():
        parser.error(f"--ipopt-dir must contain {ipopt_name}")
    root = Path(__file__).resolve().parent
    command = [
        sys.executable, "-m", "PyInstaller", "--noconfirm", "--clean", "--onefile",
        "--name", "cec_degradation", "--distpath", str(root / "dist"),
        "--workpath", str(root / "build"), "--specpath", str(root / "build"),
        "--paths", str(sixpar.parent), "--hidden-import", "SixParSolve",
        "--collect-all", "pyomo", "--collect-all", "pint",
    ]
    for binary in ipopt_dir.iterdir():
        is_runtime = binary.name == ipopt_name or binary.suffix.lower() in (".dll", ".so", ".dylib") or ".so." in binary.name
        if binary.is_file() and not binary.name.startswith("._") and is_runtime:
            command.extend(["--add-binary", f"{binary}{os.pathsep}ipopt"])
    command.append(str(root / "cec_degradation.py"))
    environment = dict(os.environ)
    if os.name == "nt":
        native_paths = [Path(sys.base_prefix) / "Library" / "bin", Path(sys.base_prefix) / "DLLs", Path(sys.base_prefix)]
        environment["PATH"] = os.pathsep.join(str(path) for path in native_paths) + os.pathsep + environment.get("PATH", "")
    subprocess.run(command, check=True, env=environment)


if __name__ == "__main__":
    main()