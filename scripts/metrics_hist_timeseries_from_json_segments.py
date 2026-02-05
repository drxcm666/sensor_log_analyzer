from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
from math import sqrt

import numpy as np
import matplotlib.pyplot as plt


def load_csv_cols(path: Path) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Reads CSV with header: t_ms,ax,ay,az"""
    t: list[int] = []
    ax: list[float] = []
    ay: list[float] = []
    az: list[float] = []

    with path.open("r", encoding="utf-8", newline="") as f:
        r = csv.DictReader(f)
        if r.fieldnames is None:
            raise ValueError(f"Empty CSV: {path}")

        need = {"t_ms", "ax", "ay", "az"}
        fields = {h.strip() for h in r.fieldnames}
        if not need.issubset(fields):
            raise ValueError(f"Unexpected header in {path}: {list(r.fieldnames)}")

        bad = 0
        for row in r:
            try:
                t.append(int(float(row["t_ms"])))
                ax.append(float(row["ax"]))
                ay.append(float(row["ay"]))
                az.append(float(row["az"]))
            except Exception:
                bad += 1
                continue

    if not t:
        raise ValueError(f"No valid rows in {path}")
    if bad:
        print(f"[{path.name}] bad rows skipped: {bad}")

    return (
        np.asarray(t, dtype=np.int64),
        np.asarray(ax, dtype=np.float64),
        np.asarray(ay, dtype=np.float64),
        np.asarray(az, dtype=np.float64),
    )


def calc_metrics(arr: np.ndarray) -> dict[str, float]:
    mean = float(np.mean(arr))
    mae = float(np.mean(np.abs(arr)))
    rmse = float(sqrt(np.mean(arr * arr)))
    maxabs = float(np.max(np.abs(arr)))
    std = float(np.std(arr, ddof=0))
    return {"mean": mean, "mae": mae, "rmse": rmse, "maxabs": maxabs, "std": std}


def main() -> int:
    ap = argparse.ArgumentParser(
        description=(
            "Residual histograms + metrics using ALL steady samples, "
            "segmented by JSON meta (npos, L, steady_start/end, refs)."
        )
    )
    ap.add_argument("--json", type=Path, required=True, help="Calibration report JSON (data_calib.json)")
    ap.add_argument("--raw_csv", type=Path, required=True, help="Raw CSV (t_ms,ax,ay,az)")
    ap.add_argument("--corr_csv", type=Path, required=True, help="Calibrated CSV (t_ms,ax,ay,az)")
    ap.add_argument("--outdir", type=Path, default=Path("plots"), help="Output dir (default: plots)")
    ap.add_argument("--bins", type=int, default=60, help="Histogram bins (default: 60)")
    args = ap.parse_args()

    data = json.loads(args.json.read_text(encoding="utf-8"))
    meta = data["meta"]
    points = data["points"]

    npos = int(meta["npos"])
    L = int(meta["L"])
    steady_start = float(meta["steady_start"])
    steady_end = float(meta["steady_end"])

    if not (0.0 <= steady_start < steady_end <= 1.0):
        raise ValueError(f"Bad steady range: {steady_start}..{steady_end}")

    # refs per position from JSON
    refs: list[tuple[float, float, float]] = []
    for p in points[:npos]:
        r = p["ref"]
        refs.append((float(r["x"]), float(r["y"]), float(r["z"])))

    t_r, ax_r, ay_r, az_r = load_csv_cols(args.raw_csv)
    t_c, ax_c, ay_c, az_c = load_csv_cols(args.corr_csv)

    n = min(len(t_r), len(t_c))
    if len(t_r) != len(t_c):
        print(f"warn: csv length differs raw={len(t_r)} corr={len(t_c)} -> using n={n}")

    # trim both
    t_r, ax_r, ay_r, az_r = t_r[:n], ax_r[:n], ay_r[:n], az_r[:n]
    t_c, ax_c, ay_c, az_c = t_c[:n], ax_c[:n], ay_c[:n], az_c[:n]

    need_total = npos * L
    if n < need_total:
        new_npos = n // L
        print(f"warn: not enough samples for npos*L ({need_total}), have n={n}. Using npos={new_npos}.")
        npos = new_npos
        refs = refs[:npos]
        if npos == 0:
            raise ValueError("Not enough samples even for 1 block of length L.")

    w0 = int(L * steady_start)
    w1 = int(L * steady_end)
    if w1 <= w0:
        raise ValueError("Steady window is empty after int conversion. Increase L or change fractions.")

    # Collect residuals across ALL steady samples (as parts first)
    res_raw_parts: dict[str, list[np.ndarray]] = {"x": [], "y": [], "z": []}
    res_cor_parts: dict[str, list[np.ndarray]] = {"x": [], "y": [], "z": []}

    # Small validation: compare segment means vs JSON means (rough sanity check)
    print("\nSanity check: steady mean vs JSON point mean (first few positions):")
    for i in range(npos):
        base = i * L
        a = base + w0
        b = base + w1

        rx, ry, rz = refs[i]

        raw_seg = np.stack([ax_r[a:b], ay_r[a:b], az_r[a:b]], axis=1)
        cor_seg = np.stack([ax_c[a:b], ay_c[a:b], az_c[a:b]], axis=1)

        raw_mean = raw_seg.mean(axis=0)
        cor_mean = cor_seg.mean(axis=0)

        # Append per-sample residuals
        res_raw_parts["x"].append(ax_r[a:b] - rx)
        res_raw_parts["y"].append(ay_r[a:b] - ry)
        res_raw_parts["z"].append(az_r[a:b] - rz)

        res_cor_parts["x"].append(ax_c[a:b] - rx)
        res_cor_parts["y"].append(ay_c[a:b] - ry)
        res_cor_parts["z"].append(az_c[a:b] - rz)

        if i < 3:
            pj = points[i]
            j_raw = np.array([pj["raw_mean"]["x"], pj["raw_mean"]["y"], pj["raw_mean"]["z"]], dtype=np.float64)
            j_cor = np.array([pj["corr_mean"]["x"], pj["corr_mean"]["y"], pj["corr_mean"]["z"]], dtype=np.float64)
            print(
                f"pos {i+1}: |raw_mean-JSON| max={np.max(np.abs(raw_mean-j_raw)):.6g}, "
                f"|corr_mean-JSON| max={np.max(np.abs(cor_mean-j_cor)):.6g}"
            )

    # concat into final arrays
    res_raw: dict[str, np.ndarray] = {k: np.concatenate(res_raw_parts[k]) for k in ("x", "y", "z")}
    res_cor: dict[str, np.ndarray] = {k: np.concatenate(res_cor_parts[k]) for k in ("x", "y", "z")}

    args.outdir.mkdir(parents=True, exist_ok=True)

    # Histograms
    for axis in ("x", "y", "z"):
        plt.figure()
        plt.hist(res_raw[axis], bins=args.bins, alpha=0.6, label="raw residuals")
        plt.hist(res_cor[axis], bins=args.bins, alpha=0.6, label="calibrated residuals")
        plt.title(f"Residuals histogram (steady samples) — {axis.upper()} axis")
        plt.xlabel(f"residual_{axis} (measured - reference)")
        plt.ylabel("count")
        plt.grid(True)
        plt.legend()
        out_path = args.outdir / f"hist_residuals_{axis}_raw_vs_corr.png"
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        plt.close()
        print(f"saved: {out_path.resolve()}")

    # Metrics table
    csv_path = args.outdir / "residual_metrics_timeseries.csv"
    with csv_path.open("w", encoding="utf-8", newline="") as f:
        w = csv.writer(f)
        w.writerow(["mode", "axis", "n_samples", "mean", "mae", "rmse", "maxabs", "std"])
        for axis in ("x", "y", "z"):
            m_raw = calc_metrics(res_raw[axis])
            m_cor = calc_metrics(res_cor[axis])

            w.writerow(["raw", axis, len(res_raw[axis]), m_raw["mean"],
                        m_raw["mae"], m_raw["rmse"], m_raw["maxabs"], m_raw["std"]])
            w.writerow(["corr", axis, len(res_cor[axis]), m_cor["mean"],
                        m_cor["mae"], m_cor["rmse"], m_cor["maxabs"], m_cor["std"]])

    print(f"saved: {csv_path.resolve()}")

    # Bar charts for RMSE and MaxAbs
    # (compute metrics once per axis)
    m_raw_by_axis = {a: calc_metrics(res_raw[a]) for a in ("x", "y", "z")}
    m_cor_by_axis = {a: calc_metrics(res_cor[a]) for a in ("x", "y", "z")}

    def bar(metric: str, out_name: str, title: str) -> None:
        axes_lbl = ["X", "Y", "Z"]
        raw_vals = [m_raw_by_axis[a][metric] for a in ("x", "y", "z")]
        cor_vals = [m_cor_by_axis[a][metric] for a in ("x", "y", "z")]

        x = np.arange(3)
        width = 0.35
        plt.figure()
        plt.bar(x - width / 2, raw_vals, width, label="raw")
        plt.bar(x + width / 2, cor_vals, width, label="calibrated")
        plt.xticks(x, axes_lbl)
        plt.title(title)
        plt.xlabel("axis")
        plt.ylabel(metric)
        plt.grid(True, axis="y")
        plt.legend()
        out_path = args.outdir / out_name
        plt.savefig(out_path, dpi=150, bbox_inches="tight")
        plt.close()
        print(f"saved: {out_path.resolve()}")

    bar("rmse", "bar_rmse_timeseries.png", "RMSE (steady samples) — raw vs calibrated")
    bar("maxabs", "bar_maxabs_timeseries.png", "MaxAbs error (steady samples) — raw vs calibrated")

    print(f"\nread json:  {args.json.resolve()}")
    print(f"read raw:   {args.raw_csv.resolve()}")
    print(f"read corr:  {args.corr_csv.resolve()}")
    print(f"npos={npos}, L={L}, steady=[{steady_start},{steady_end}] -> window [{w0},{w1}) per block")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""
python scripts\\metrics_hist_timeseries_from_json_segments.py --json .\\data\\data_calib.json
--raw_csv .\\data\\data.csv --corr_csv .\\data\\data_calib.csv

"""
