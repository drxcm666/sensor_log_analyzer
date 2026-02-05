from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib.pyplot as plt


def main() -> int:
    ap = argparse.ArgumentParser(description="Histograms of residuals (raw vs calibrated) for X/Y/Z axes.")
    ap.add_argument("--json", type=Path, required=True, help="Path to calibration report JSON (with res_raw/res_corr)")
    ap.add_argument("--outdir", type=Path, default=Path("plots"), help="Output directory (default: plots)")
    ap.add_argument("--bins", type=int, default=12, help="Histogram bins (default: 12)")
    args = ap.parse_args()

    data = json.loads(args.json.read_text(encoding="utf-8"))
    points = data["points"]

    args.outdir.mkdir(parents=True, exist_ok=True)

    for axis in ("x", "y", "z"):
        raw = [p["res_raw"][axis] for p in points]
        corr = [p["res_corr"][axis] for p in points]

        plt.figure()
        plt.hist(raw, bins=args.bins, alpha=0.6, label="raw residuals")
        plt.hist(corr, bins=args.bins, alpha=0.6, label="calibrated residuals")
        plt.title(f"Residuals histogram — {axis.upper()} axis (raw vs calibrated)")
        plt.xlabel(f"residual_{axis} (measured - reference)")
        plt.ylabel("count")
        plt.grid(True)
        plt.legend()

        out_path = args.outdir / f"hist_residuals_{axis}_raw_vs_corr.png"
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        plt.close()
        print(f"saved: {out_path.resolve()}")

    print(f"read:  {args.json.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# python scripts\hist_residuals_all_axes.py --json .\data\data_calib.json
