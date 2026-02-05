from __future__ import annotations

import argparse
import json
from pathlib import Path

import matplotlib.pyplot as plt


def plot_curve(points, axis: str, key: str, out_path: Path, title: str) -> None:
    ref = [p["ref"][axis] for p in points]
    meas = [p[key][axis] for p in points]

    mn = min(ref + meas)
    mx = max(ref + meas)

    plt.figure()
    plt.scatter(ref, meas, label=key)
    plt.plot([mn, mx], [mn, mx], label="y=x")
    plt.title(title)
    plt.xlabel(f"reference_{axis}")
    plt.ylabel(f"measured_{key}_{axis}")
    plt.grid(True)
    plt.legend()
    plt.savefig(out_path, dpi=150, bbox_inches="tight")
    plt.close()


def main() -> int:
    ap = argparse.ArgumentParser(description="Calibration curves (CALIBRATED) for X/Y/Z axes.")
    ap.add_argument("--json", type=Path, required=True, help="Path to calibration report JSON")
    ap.add_argument("--outdir", type=Path, default=Path("plots"), help="Output dir (default: plots)")
    args = ap.parse_args()

    data = json.loads(args.json.read_text(encoding="utf-8"))
    points = data["points"]

    args.outdir.mkdir(parents=True, exist_ok=True)

    for axis in ("x", "y", "z"):
        out_path = args.outdir / f"calib_curve_{axis}_corr.png"
        plot_curve(
            points,
            axis=axis,
            key="corr_mean",
            out_path=out_path,
            title=f"Calibration curve (calibrated) — {axis.upper()} axis",
        )
        print(f"saved: {out_path.resolve()}")

    print(f"read:  {args.json.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# python scripts\curve_corr_all_axes.py --json .\data\data_calib.json
