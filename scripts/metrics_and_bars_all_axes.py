from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
from math import sqrt

import numpy as np
import matplotlib.pyplot as plt


def metrics(arr: np.ndarray) -> dict[str, float]:
    mean = float(np.mean(arr))
    mae = float(np.mean(np.abs(arr)))
    rmse = float(sqrt(np.mean(arr * arr)))
    maxabs = float(np.max(np.abs(arr)))
    std = float(np.std(arr, ddof=0))
    return {"mean": mean, "mae": mae, "rmse": rmse, "maxabs": maxabs, "std": std}


def main() -> int:
    ap = argparse.ArgumentParser(description="Metrics table + bar charts (RMSE, MaxAbs) for X/Y/Z axes.")
    ap.add_argument("--json", type=Path, required=True, help="Path to calibration report JSON (with res_raw/res_corr)")
    ap.add_argument("--outdir", type=Path, default=Path("plots"), help="Output directory (default: plots)")
    args = ap.parse_args()

    data = json.loads(args.json.read_text(encoding="utf-8"))
    points = data["points"]

    args.outdir.mkdir(parents=True, exist_ok=True)

    rows = []
    for axis in ("x", "y", "z"):
        raw = np.array([p["res_raw"][axis] for p in points], dtype=np.float64)
        corr = np.array([p["res_corr"][axis] for p in points], dtype=np.float64)

        m_raw = metrics(raw)
        m_cor = metrics(corr)

        rows.append(("raw", axis, m_raw))
        rows.append(("corr", axis, m_cor))

    # ---- Save CSV table
    csv_path = args.outdir / "residual_metrics.csv"
    with csv_path.open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["mode", "axis", "mean", "mae", "rmse", "maxabs", "std"])
        for mode, axis, m in rows:
            w.writerow([mode, axis, m["mean"], m["mae"], m["rmse"], m["maxabs"], m["std"]])
    print(f"saved: {csv_path.resolve()}")

    # ---- Print summary to console
    print("\nResidual metrics (from 8 calibration points):")
    for axis in ("x", "y", "z"):
        m_raw = next(m for mode, ax, m in rows if mode == "raw" and ax == axis)
        m_cor = next(m for mode, ax, m in rows if mode == "corr" and ax == axis)
        print(f"\nAxis {axis.upper()}:")
        print(f"  RMSE:   raw={m_raw['rmse']:.6g}   corr={m_cor['rmse']:.6g}")
        print(f"  MaxAbs: raw={m_raw['maxabs']:.6g} corr={m_cor['maxabs']:.6g}")
        print(f"  MAE:    raw={m_raw['mae']:.6g}    corr={m_cor['mae']:.6g}")
        print(f"  Mean:   raw={m_raw['mean']:.6g}   corr={m_cor['mean']:.6g}")
        print(f"  Std:    raw={m_raw['std']:.6g}    corr={m_cor['std']:.6g}")

    # ---- Bar chart helper (grouped bars)
    def bar_metric(metric_name: str, out_name: str, title: str) -> None:
        axes = ["X", "Y", "Z"]
        raw_vals = []
        cor_vals = []
        for a in ("x", "y", "z"):
            m_raw = next(m for mode, ax, m in rows if mode == "raw" and ax == a)
            m_cor = next(m for mode, ax, m in rows if mode == "corr" and ax == a)
            raw_vals.append(m_raw[metric_name])
            cor_vals.append(m_cor[metric_name])

        x = np.arange(len(axes))
        width = 0.35

        plt.figure()
        plt.bar(x - width / 2, raw_vals, width, label="raw")
        plt.bar(x + width / 2, cor_vals, width, label="calibrated")
        plt.xticks(x, axes)
        plt.title(title)
        plt.xlabel("axis")
        plt.ylabel(metric_name)
        plt.grid(True, axis="y")
        plt.legend()

        out_path = args.outdir / out_name
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        plt.close()
        print(f"saved: {out_path.resolve()}")

    bar_metric("rmse", "bar_rmse_raw_vs_corr.png", "RMSE (raw vs calibrated) — by axis")
    bar_metric("maxabs", "bar_maxabs_raw_vs_corr.png", "MaxAbs error (raw vs calibrated) — by axis")

    print(f"\nread: {args.json.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# python scripts\metrics_and_bars_all_axes.py --json .\data\data_calib.json
