from __future__ import annotations

import argparse
import csv
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt


def load_imu_csv(path: Path) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """
    Expected header: t_ms,ax,ay,az
    Returns: t_ms (int64), ax, ay, az (float64)
    """
    t_list: list[int] = []
    ax_list: list[float] = []
    ay_list: list[float] = []
    az_list: list[float] = []

    with path.open("r", encoding="utf-8", newline="") as f:
        r = csv.DictReader(f)
        if r.fieldnames is None:
            raise ValueError(f"Empty CSV or missing header: {path}")

        fields = [h.strip() for h in r.fieldnames]
        need = {"t_ms", "ax", "ay", "az"}
        if not need.issubset(set(fields)):
            raise ValueError(f"Unexpected header in {path}: {fields}")

        bad = 0
        total = 0
        for row in r:
            total += 1
            try:
                t = int(float(row["t_ms"]))
                ax = float(row["ax"])
                ay = float(row["ay"])
                az = float(row["az"])
            except (TypeError, ValueError, KeyError):
                bad += 1
                continue

            t_list.append(t)
            ax_list.append(ax)
            ay_list.append(ay)
            az_list.append(az)

    if not t_list:
        raise ValueError(f"No valid rows read from: {path}")

    if bad:
        print(f"[{path.name}] ok={len(t_list)} bad={bad} total={total}")

    return (
        np.asarray(t_list, dtype=np.int64),
        np.asarray(ax_list, dtype=np.float64),
        np.asarray(ay_list, dtype=np.float64),
        np.asarray(az_list, dtype=np.float64),
    )


def main() -> int:
    ap = argparse.ArgumentParser(
        description="3 interactive time-series plots: raw vs calibrated for ax/ay/az."
    )
    ap.add_argument("--raw", type=Path, required=True, help="Raw CSV path (before calibration)")
    ap.add_argument("--corr", type=Path, required=True, help="Calibrated CSV path (after calibration)")
    ap.add_argument("--outdir", type=Path, default=Path("plots"), help="Output directory (default: plots)")
    ap.add_argument("--stride", type=int, default=1, help="Plot every Nth sample (default: 1 = all)")
    ap.add_argument("--show", action="store_true", help="Open interactive windows (zoom/pan)")
    args = ap.parse_args()

    if args.stride < 1:
        raise ValueError("--stride must be >= 1")

    t_raw, ax_raw, ay_raw, az_raw = load_imu_csv(args.raw)
    t_cor, ax_cor, ay_cor, az_cor = load_imu_csv(args.corr)

    n = min(len(t_raw), len(t_cor))
    if len(t_raw) != len(t_cor):
        print(f"warn: length differs raw={len(t_raw)} corr={len(t_cor)} -> using n={n}")

    # Trim to same length
    t_raw, ax_raw, ay_raw, az_raw = t_raw[:n], ax_raw[:n], ay_raw[:n], az_raw[:n]
    t_cor, ax_cor, ay_cor, az_cor = t_cor[:n], ax_cor[:n], ay_cor[:n], az_cor[:n]

    # Time sanity check
    max_dt = int(np.max(np.abs(t_raw - t_cor)))
    if max_dt != 0:
        print(f"warn: t_ms mismatch between raw/corr, max |Δt_ms| = {max_dt}")

    # time from start, seconds
    t = (t_raw - t_raw[0]).astype(np.float64) / 1000.0

    # stride
    s = args.stride
    t = t[::s]
    ax_raw, ay_raw, az_raw = ax_raw[::s], ay_raw[::s], az_raw[::s]
    ax_cor, ay_cor, az_cor = ax_cor[::s], ay_cor[::s], az_cor[::s]

    args.outdir.mkdir(parents=True, exist_ok=True)

    def make_plot(name: str, y_raw: np.ndarray, y_cor: np.ndarray, out_name: str) -> None:
        plt.figure()
        plt.plot(t, y_raw, label=f"{name} raw")
        plt.plot(t, y_cor, label=f"{name} calibrated")
        plt.title(f"{name}: raw vs calibrated")
        plt.xlabel("time (s)")
        plt.ylabel(name)
        plt.grid(True)
        plt.legend()

        out_path = args.outdir / out_name
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        print(f"saved: {out_path.resolve()}")

    make_plot("ax", ax_raw, ax_cor, "time_ax_raw_vs_corr.png")
    make_plot("ay", ay_raw, ay_cor, "time_ay_raw_vs_corr.png")
    make_plot("az", az_raw, az_cor, "time_az_raw_vs_corr.png")

    if args.show:
        plt.show()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

# python scripts\plot_time_raw_vs_corr_3axes_interactive.py --raw .\data\data.csv --corr .\data\data_calib.csv --show
