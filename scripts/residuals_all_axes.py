from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib.pyplot as plt


def plot_residuals(points, axis: str, out_path: Path) -> None:
    ref = [p["ref"][axis] for p in points]
    res_raw = [p["res_raw"][axis] for p in points]
    res_corr = [p["res_corr"][axis] for p in points]

    mn = min(ref)
    mx = max(ref)

    plt.figure()
    plt.scatter(ref, res_raw, label="raw residual")
    plt.scatter(ref, res_corr, label="calibrated residual")
    plt.plot([mn, mx], [0.0, 0.0], label="y=0")
    plt.title(f"Residuals vs reference — {axis.upper()} axis (raw vs calibrated)")
    plt.xlabel(f"reference_{axis}")
    plt.ylabel(f"residual_{axis} (measured - reference)")
    plt.grid(True)
    plt.legend()
    plt.savefig(out_path, dpi=150, bbox_inches="tight")
    plt.close()


def main() -> int:
    ap = argparse.ArgumentParser(description="Residuals vs reference for X/Y/Z axes (raw vs calibrated).")
    ap.add_argument("--json", type=Path, required=True, help="Path to calibration report JSON")
    ap.add_argument("--outdir", type=Path, default=Path("plots"), help="Output dir (default: plots)")
    args = ap.parse_args()

    data = json.loads(args.json.read_text(encoding="utf-8"))
    points = data["points"]

    args.outdir.mkdir(parents=True, exist_ok=True)

    for axis in ("x", "y", "z"):
        out_path = args.outdir / f"residuals_{axis}_raw_vs_corr.png"
        plot_residuals(points, axis=axis, out_path=out_path)
        print(f"saved: {out_path.resolve()}")

    print(f"read:  {args.json.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# python scripts\residuals_all_axes.py --json .\data\data_calib.json
