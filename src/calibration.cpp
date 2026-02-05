#include "calibration.hpp"
#include "writer.hpp"
#include "csv.hpp"

#include <fstream>
#include <cmath>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fmt/core.h>
#include <array>

namespace sla
{

    static Stats to_stats(const WelfordStats &w)
    {
        Stats s;
        if (w.count() == 0)
            return s;

        s.count = static_cast<int>(w.count());
        s.min = w.min();
        s.max = w.max();
        s.mean = w.mean();
        s.std = w.stddev();
        return s;
    }

    static std::vector<Position> read_position(const std::filesystem::path &path, std::string &error)
    {
        error.clear();

        std::ifstream pos_in(path);
        if (!pos_in)
        {
            error = "Error, can't open file: " + path.string();
            return {};
        }

        std::string h1, h2;
        if (!(pos_in >> h1 >> h2)) // consume header: INNER OUTER
        {
            error = "Error: bad header in " + path.string();
            return {};
        }

        std::vector<Position> positions;

        int inner{0}, outer{0};
        while (pos_in >> inner >> outer)
            positions.push_back({inner, outer});

        if (positions.empty())
        {
            error = "Error: no position found in " + path.string();
            return {};
        }

        return positions;
    }

    static double deg2rad(double deg)
    {
        double PI{3.14159265359};
        return deg * (PI / 180.0);
    }

    static Vec3 gravity_true(double g, int inner_deg, int outer_deg)
    {
        const double phi = deg2rad(static_cast<double>(inner_deg));
        const double psi = deg2rad(static_cast<double>(outer_deg));

        // a_true = Rx(-phi) * Rz(-psi) * [g,0,0]
        const double ax = g * std::cos(psi);
        const double ay = (-g * std::sin(psi)) * std::cos(phi);
        const double az = (g * std::sin(psi)) * std::sin(phi);

        return {ax, ay, az};
    }

    static void build_system_Ax_eq_Y(
        const std::array<Vec3, 8> &a_true,
        const std::array<double, 8> &ax_mean,
        const std::array<double, 8> &ay_mean,
        const std::array<double, 8> &az_mean,
        int npos,
        std::vector<std::array<double, 12>> &A,
        std::vector<double> &Y)
    {
        const int rows = 3 * npos;
        A.assign(rows, std::array<double, 12>{});
        Y.assign(rows, 0.0);

        for (int i = 0; i < npos; ++i)
        {
            const Vec3 t = a_true[i]; // gravity
            const double mx = ax_mean[i];
            const double my = ay_mean[i];
            const double mz = az_mean[i];

            const int r0 = 3 * i + 0; // mx
            const int r1 = 3 * i + 1; // my
            const int r2 = 3 * i + 2; // mz

            // --- row for mx: [tx ty tz  0 0 0  0 0 0  1 0 0] = mx
            A[r0][0] = t.x; // row0: g*m11 + bx = mx
            A[r0][1] = t.y;
            A[r0][2] = t.z;
            A[r0][9] = 1.0;
            Y[r0] = mx;

            // --- row for my: [0 0 0  tx ty tz  0 0 0  0 1 0] = my
            A[r1][3] = t.x;
            A[r1][4] = t.y;
            A[r1][5] = t.z;
            A[r1][10] = 1.0;
            Y[r1] = my;

            // --- row for mz: [0 0 0  0 0 0  tx ty tz  0 0 1] = mz
            A[r2][6] = t.x;
            A[r2][7] = t.y;
            A[r2][8] = t.z;
            A[r2][11] = 1.0;
            Y[r2] = mz;
        }
    }

    /*
    A:24×12, Aᵀ:12×24, y:24×1, AᵀA=12×12, Aᵀy:12×1

             ┌────────────────────────────────────────────────────┐
             │ m11 m12 m13 | m21 m22 m23 | m31 m32 m33 | bx by bz │
        ┌────┼────────────────────────────────────────────────────┤
        │    │ tx0 ty0 tz0 |  0   0   0  |  0   0   0  |  1  0  0 │  row0  -> mx0
        │    │  0   0   0  | tx0 ty0 tz0 |  0   0   0  |  0  1  0 │  row1  -> my0
        │    │  0   0   0  |  0   0   0  | tx0 ty0 tz0 |  0  0  1 │  row2  -> mz0
        │    │							                          │
        │    │ tx1 ty1 tz1 |  0   0   0  |  0   0   0  |  1  0  0 │  row3  -> mx1
        │    │  0   0   0  | tx1 ty1 tz1 |  0   0   0  |  0  1  0 │  row4  -> my1
        │    │  0   0   0  |  0   0   0  | tx1 ty1 tz1 |  0  0  1 │  row5  -> mz1
        │    │							                          │
        │    │                   ... (i=7) ...                    │
        │    │							                          │
        │    │ tx7 ty7 tz7 |  0   0   0  |  0   0   0  |  1  0  0 │  row21 -> mx7
        │    │  0   0   0  | tx7 ty7 tz7 |  0   0   0  |  0  1  0 │  row22 -> my7
        │    │  0   0   0  |  0   0   0  | tx7 ty7 tz7 |  0  0  1 │  row23 -> mz7
        └────┴────────────────────────────────────────────────────┘

                ┌─────┐
    y (24×1)  = │ mx0 │
                │ my0 │
                │ mz0 │
                │ mx1 │
                │ my1 │
                │ mz1 │
                │ ... │
                │ mx7 │
                │ my7 │
                │ mz7 │
                └─────┘
    */

    // (At*A)*x = At*Y
    static void compute_normal_equations(
        const std::vector<std::array<double, 12>> &A,
        const std::vector<double> &Y,
        std::array<std::array<double, 12>, 12> &AtA,
        std::array<double, 12> &AtY)
    {
        // zero init
        for (auto &row : AtA)
            row.fill(0.0);
        AtY.fill(0.0);

        const int rows = (int)A.size();
        for (int r = 0; r < rows; r++)
        {
            const auto &ar = A[r]; // row r of A
            const auto &yr = Y[r]; // y value for row r

            // Example for r=0 (mx row, position 0):
            // ar = [t0.x, t0.y, t0.z, 0, 0, 0, 0, 0, 0, 1, 0, 0], yr = mx0.
            // i=0 -> AtY[0] += t0.x * mx0, i=1 -> AtY[1] += t0.y * mx0, i=3 -> AtY[3] += 0 * mx0.
            // Example for r=1 (my row, position 0):
            // ar = [0, 0, 0, t0.x, t0.y, t0.z, 0, 0, 0, 0, 1, 0], yr = my0.
            // Aty += A^T * y

            // Aty += A^T * y
            for (int i = 0; i < 12; i++)
            {
                AtY[i] += ar[i] * yr;
            }

            // AtA += A^T * A
            for (int i = 0; i < 12; i++)
            {
                const double ai = ar[i];
                if (ai == 0.0)
                    continue;

                for (int j = 0; j < 12; ++j)
                    AtA[i][j] += ai * ar[j];
            }
        }
    }

    static bool solve_gauss_12(
        std::array<std::array<double, 12>, 12> A,
        std::array<double, 12> b,
        std::array<double, 12> &x,
        std::string &error)
    {

        // Pass a and b as copies (so they can be destroyed during the method)
        // x — result

        const int n = 12;

        for (int col = 0; col < n; ++col)
        {
            // 1) find pivot row
            int piv = col;
            double best = std::abs(A[col][col]);

            for (int r = col + 1; r < n; ++r)
            {
                double v = std::abs(A[r][col]);
                if (v > best)
                {
                    best = v;
                    piv = r;
                }
            }

            // 2) degeneracy check
            const double eps = 1e-12;
            if (best < eps)
            {
                error = "Singular/ill-conditioned system: pivot too small at col=" + std::to_string(col);
                return false;
            }

            // 3) swap rows
            if (piv != col)
            {
                std::swap(A[piv], A[col]);
                std::swap(b[piv], b[col]);
            }

            // 4) elimination below
            const double diag = A[col][col];
            for (int r = col + 1; r < n; ++r)
            {
                const double f = A[r][col] / diag;
                if (f == 0.0)
                    continue;

                // A[r] -= f * A[col]
                for (int c = col; c < n; ++c)
                    A[r][c] -= f * A[col][c];

                b[r] -= f * b[col];
            }
        }

        // back substitution
        x.fill(0.0);
        for (int r = n - 1; r >= 0; --r)
        {
            double s = b[r];
            for (int c = r + 1; c < n; ++c)
                s -= A[r][c] * x[c];

            const double diag = A[r][r];
            const double eps = 1e-12;
            if (std::abs(diag) < eps)
            {
                error = "Singular/ill-conditioned system: zero diagonal at r=" + std::to_string(r);
                return false;
            }

            x[r] = s / diag;
        }

        return true;
    }

    static void unpack_params(
        const std::array<double, 12> &x,
        Mat3 &M,
        Vec3 &b)
    {
        M.a[0][0] = x[0];
        M.a[0][1] = x[1];
        M.a[0][2] = x[2];
        M.a[1][0] = x[3];
        M.a[1][1] = x[4];
        M.a[1][2] = x[5];
        M.a[2][0] = x[6];
        M.a[2][1] = x[7];
        M.a[2][2] = x[8];

        b.x = x[9];
        b.y = x[10];
        b.z = x[11];
    }

    static Vec3 vec3_sub(const Vec3 &a, const Vec3 &b)
    {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    static Vec3 mat3_mul_vec3(const Mat3 &M, const Vec3 &v)
    {
        Vec3 r;
        r.x = M.a[0][0] * v.x + M.a[0][1] * v.y + M.a[0][2] * v.z;
        r.y = M.a[1][0] * v.x + M.a[1][1] * v.y + M.a[1][2] * v.z;
        r.z = M.a[2][0] * v.x + M.a[2][1] * v.y + M.a[2][2] * v.z;
        return r;
    }

    // Invert 3x3 matrix. Returns false if det ~ 0.
    static bool invert_mat3(const Mat3 &M, Mat3 &Minv, std::string &error)
    {
        const double a00 = M.a[0][0], a01 = M.a[0][1], a02 = M.a[0][2];
        const double a10 = M.a[1][0], a11 = M.a[1][1], a12 = M.a[1][2];
        const double a20 = M.a[2][0], a21 = M.a[2][1], a22 = M.a[2][2];

        const double c00 = (a11 * a22 - a12 * a21);
        const double c01 = -(a10 * a22 - a12 * a20);
        const double c02 = (a10 * a21 - a11 * a20);

        const double c10 = -(a01 * a22 - a02 * a21);
        const double c11 = (a00 * a22 - a02 * a20);
        const double c12 = -(a00 * a21 - a01 * a20);

        const double c20 = (a01 * a12 - a02 * a11);
        const double c21 = -(a00 * a12 - a02 * a10);
        const double c22 = (a00 * a11 - a01 * a10);

        const double det = a00 * c00 + a01 * c01 + a02 * c02;

        const double eps = 1e-12;
        if (std::abs(det) < eps)
        {
            error = "M is singular / ill-conditioned: det=" + std::to_string(det);
            return false;
        }

        const double inv_det = 1.0 / det;

        // adj(M) = C^T, so inverse = adj/det
        Minv.a[0][0] = c00 * inv_det;
        Minv.a[0][1] = c10 * inv_det;
        Minv.a[0][2] = c20 * inv_det;

        Minv.a[1][0] = c01 * inv_det;
        Minv.a[1][1] = c11 * inv_det;
        Minv.a[1][2] = c21 * inv_det;

        Minv.a[2][0] = c02 * inv_det;
        Minv.a[2][1] = c12 * inv_det;
        Minv.a[2][2] = c22 * inv_det;

        return true;
    }

    CalibrationResult run_calibration(const CalibrationOptions &opt)
    {
        CalibrationResult res;

        auto positions = read_position(opt.position_path, res.error);

        if (!res.error.empty())
        {
            res.ok = false;
            return res;
        }
        if (positions.size() != 8)
        {
            res.ok = false;
            res.error = "expected 8 positions, got " + std::to_string(positions.size());
            return res;
        }

        // Read data 1
        double max_abs_mag_raw_all{0.0};
        auto calib_pass1 = sla::read_imu_csv_streaming(opt.input_path,
                                                       [&](const std::array<double, 4> &row)
                                                       {
                                                           const double ax_raw = row[1], ay_raw = row[2], az_raw = row[3];
                                                           const double mag_raw = std::sqrt(ax_raw * ax_raw + ay_raw * ay_raw + az_raw * az_raw);
                                                           max_abs_mag_raw_all = std::max(max_abs_mag_raw_all, std::abs(mag_raw - opt.gravity));
                                                       });

        if (!calib_pass1.ok)
        {
            res.ok = false;
            res.error = calib_pass1.error;
            return res;
        }

        const int N = static_cast<int>(calib_pass1.counts.parsed_lines); // 80000
        const int npos = static_cast<int>(positions.size());             // 8
        const int L = N / npos;

        if (L <= 0)
        {
            res.ok = false;
            res.error = "L <= 0 (N=" + std::to_string(N) + ", npos=" + std::to_string(npos) + ")";
            return res;
        }

        const int steady_start = static_cast<int>(opt.steady_start_frac * L);
        const int steady_end = static_cast<int>(opt.steady_end_frac * L);

        // Read data 2: means per block (steady only)
        std::array<double, 8> sum_ax{}, sum_ay{}, sum_az{};
        std::array<int, 8> cnt{};
        std::array<double, 8> ax_mean{}, ay_mean{}, az_mean{};

        int row_count2{0};

        auto calib_pass2 = sla::read_imu_csv_streaming(opt.input_path,
                                                       [&](const std::array<double, 4> &row)
                                                       {
                                                           const int block = row_count2 / L;  // (0...7)
                                                           const int offset = row_count2 % L; //  (0...L-1)
                                                           row_count2++;

                                                           if (block >= npos)
                                                               return;

                                                           if (steady_start <= offset && offset < steady_end)
                                                           {
                                                               sum_ax[block] += row[1];
                                                               sum_ay[block] += row[2];
                                                               sum_az[block] += row[3];
                                                               cnt[block]++;
                                                           }
                                                       });

        if (!calib_pass2.ok)
        {
            res.ok = false;
            res.error = calib_pass2.error;
            return res;
        }

        for (int i = 0; i < npos; i++)
        {

            if (cnt[i] == 0)
            {
                res.ok = false;
                res.error = "block " + std::to_string(i) + " has cnt=0";
                return res;
            }

            ax_mean[i] = sum_ax[i] / cnt[i];
            ay_mean[i] = sum_ay[i] / cnt[i];
            az_mean[i] = sum_az[i] / cnt[i];
        }

        std::array<Vec3, 8> a_true{};
        for (int i = 0; i < npos; ++i)
        {
            a_true[i] = gravity_true(opt.gravity, positions[i].inner, positions[i].outer);
        }

        std::vector<std::array<double, 12>> A;
        std::vector<double> Y;

        build_system_Ax_eq_Y(a_true, ax_mean, ay_mean, az_mean, npos, A, Y);

        std::array<std::array<double, 12>, 12> AtA;
        std::array<double, 12> Aty;
        compute_normal_equations(A, Y, AtA, Aty);

        std::array<double, 12> x;
        std::string solve_error;
        if (!solve_gauss_12(AtA, Aty, x, solve_error))
        {
            res.ok = false;
            res.error = solve_error;
            return res;
        }

        Mat3 M;
        Vec3 b;
        unpack_params(x, M, b);

        for (int i = 0; i < npos; ++i)
        {
            const Vec3 t = a_true[i];
            const Vec3 meas = {ax_mean[i], ay_mean[i], az_mean[i]};

            const Vec3 Mt = mat3_mul_vec3(M, t);
            const Vec3 pred = {Mt.x + b.x, Mt.y + b.y, Mt.z + b.z};

            const Vec3 err = {meas.x - pred.x, meas.y - pred.y, meas.z - pred.z};

            fmt::println("fit i={} err=({:.6f},{:.6f},{:.6f})",
                         i, err.x, err.y, err.z);
        }

        Mat3 Minv;
        std::string inv_error;
        if (!invert_mat3(M, Minv, inv_error))
        {
            res.ok = false;
            res.error = inv_error;
            return res;
        }

        nlohmann::ordered_json points = nlohmann::ordered_json::array();
        for (int i = 0; i < npos; ++i)
        {
            const Vec3 ref = a_true[i];
            const Vec3 raw_mean = {ax_mean[i], ay_mean[i], az_mean[i]};

            // corr_mean = Minv * (raw_mean - b)
            const Vec3 raw_minus_b = vec3_sub(raw_mean, b);
            const Vec3 corr_mean = mat3_mul_vec3(Minv, raw_minus_b);

            // res_raw = raw_mean - ref
            const Vec3 res_raw = vec3_sub(raw_mean, ref);

            // res_corr = corr_mean - ref
            const Vec3 res_corr = vec3_sub(corr_mean, ref);

            nlohmann::ordered_json point = {
                {"position", i + 1},
                {"ref", {{"x", ref.x}, {"y", ref.y}, {"z", ref.z}}},
                {"raw_mean", {{"x", raw_mean.x}, {"y", raw_mean.y}, {"z", raw_mean.z}}},
                {"corr_mean", {{"x", corr_mean.x}, {"y", corr_mean.y}, {"z", corr_mean.z}}},
                {"res_raw", {{"x", res_raw.x}, {"y", res_raw.y}, {"z", res_raw.z}}},
                {"res_corr", {{"x", res_corr.x}, {"y", res_corr.y}, {"z", res_corr.z}}}};

            points.push_back(point);
        }

        nlohmann::ordered_json j;
        j["meta"] = {
            {"gravity", opt.gravity},
            {"L", L},
            {"steady_start", opt.steady_start_frac},
            {"steady_end", opt.steady_end_frac},
            {"npos", npos}};

        j["coeffs"] = {
            {"M", {
                {M.a[0][0], M.a[0][1], M.a[0][2]}, 
                {M.a[1][0], M.a[1][1], M.a[1][2]}, 
                {M.a[2][0], M.a[2][1], M.a[2][2]}
            }},
            {"b", {
                {"x", b.x}, 
                {"y", b.y}, 
                {"z", b.z}
            }},
            {"C", {
                {Minv.a[0][0], Minv.a[0][1], Minv.a[0][2]}, 
                {Minv.a[1][0], Minv.a[1][1], Minv.a[1][2]}, 
                {Minv.a[2][0], Minv.a[2][1], Minv.a[2][2]}
            }}
        };

        j["points"] = points;

        std::filesystem::path report_path = std::filesystem::path(opt.output_path);
        report_path.replace_extension(".json");

        std::ofstream file(report_path.string());
        file << j.dump(4);
        file.close();

        sla::CsvWriter calib_writer;
        if (!calib_writer.open(opt.output_path))
        {
            res.ok = false;
            res.error = "can't open output file: " + opt.output_path.string();
            return res;
        }
        calib_writer.write_header(sla::EXPECTED_HEADER);

        sla::WelfordStats mag_corr_stats;
        double max_abs_mag_raw_minus_g_steady{0.0};
        double max_abs_mag_corr_minus_g_steady{0.0};

        int row_count3{0};

        auto calib_pass3 = sla::read_imu_csv_streaming(opt.input_path,
                                                       [&](const std::array<double, 4> &row)
                                                       {
                                                           const int block = row_count3 / L;  // (0...7)
                                                           const int offset = row_count3 % L; //  (0...L-1)
                                                           row_count3++;

                                                           // raw accel
                                                           const Vec3 raw{row[1], row[2], row[3]};

                                                           // a_corr = Minv * (raw - b)
                                                           const Vec3 raw_minus_b = vec3_sub(raw, b);
                                                           const Vec3 corr = mat3_mul_vec3(Minv, raw_minus_b);

                                                           const double mag_raw = std::sqrt(raw.x * raw.x + raw.y * raw.y + raw.z * raw.z);
                                                           const double mag_corr = std::sqrt(corr.x * corr.x + corr.y * corr.y + corr.z * corr.z);

                                                           // always write corrected rows
                                                           std::array<double, 4> out = row;
                                                           out[1] = corr.x;
                                                           out[2] = corr.y;
                                                           out[3] = corr.z;
                                                           calib_writer.write_row(out);

                                                           // steady-only metrics (valid blocks only)
                                                           if (block < npos && steady_start <= offset && offset < steady_end)
                                                           {
                                                               mag_corr_stats.update(mag_corr);

                                                               max_abs_mag_raw_minus_g_steady = std::max(max_abs_mag_raw_minus_g_steady,
                                                                                                         std::abs(mag_raw - opt.gravity));

                                                               max_abs_mag_corr_minus_g_steady = std::max(max_abs_mag_corr_minus_g_steady,
                                                                                                          std::abs(mag_corr - opt.gravity));
                                                           }
                                                       });

        calib_writer.close();

        if (!calib_pass3.ok)
        {
            res.ok = false;
            res.error = calib_pass3.error;
            return res;
        }

        if (mag_corr_stats.count() == 0)
        {
            res.ok = false;
            res.error = "no steady samples in pass3 (check steady window / L)";
            return res;
        }

        res.max_abs_mag_raw_all = max_abs_mag_raw_all;
        res.parsed_lines = static_cast<std::size_t>(N);
        res.npos = npos;
        res.L = L;
        res.steady_start = steady_start;
        res.steady_end = steady_end;
        res.M = M;
        res.b = b;
        res.C = Minv; // correction matrix
        res.d = b;    // correction bias
        res.max_abs_mag_raw_steady = max_abs_mag_raw_minus_g_steady;
        res.max_abs_mag_corr_steady = max_abs_mag_corr_minus_g_steady;

        res.mag_corr_stats = to_stats(mag_corr_stats);

        fmt::println("Raw(steady)  max(|mag-g|) = {:.6f}", max_abs_mag_raw_minus_g_steady);
        fmt::println("Corr(steady) max(|mag-g|) = {:.6f}", max_abs_mag_corr_minus_g_steady);
        fmt::println("Corr mag stats(steady): mean={:.6f}, std={:.6f}, min={:.6f}, max={:.6f}",
                     mag_corr_stats.mean(), mag_corr_stats.stddev(),
                     mag_corr_stats.min(), mag_corr_stats.max());

        return res;
    }

}
