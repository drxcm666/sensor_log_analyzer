#include "time_axis.hpp"
#include <cmath>

namespace sla
{

    std::vector<double> compute_positive_dts(const std::vector<double> &t)
    {
        std::vector<double> out;

        if (t.size() < 2)
            return out;

        out.reserve(t.size() - 1);

        for (size_t i = 1; i < t.size(); i++)
        {
            const double dt = t[i] - t[i - 1];

            if (dt > 0)
                out.push_back(dt);
        }

        return out;
    }

    TimeAxisIssues time_axis_analysis(const std::vector<double> &t, double expected_dt)
    {
        TimeAxisIssues r{};
        constexpr double EPSILON = 1e-9;  // Обчислюється при компіляції
        const bool expected_dt_ok = (expected_dt > EPSILON);

        for (size_t i = 1; i < t.size(); i++)
        {
            const double dt = t[i] - t[i - 1];

            if (dt < -EPSILON)
                r.non_increasing++;
            else if (std::abs(dt) <= EPSILON)
                r.duplicates++;

            if (expected_dt_ok && dt > EPSILON && dt > 2 * expected_dt)
                r.gaps++;
        }

        return r;
    }

    TimeAxisReport make_time_axis_report(const std::vector<double> &timestamps)
    {
        TimeAxisReport rep{};

        const auto dt_values = compute_positive_dts(timestamps);
        rep.dt_ms = calculate_stats(dt_values);

        constexpr double EPS = 1e-9;

        rep.dt_available = (rep.dt_ms.count > 0) && (rep.dt_ms.mean > EPS);

        const double expected_dt = rep.dt_available ? rep.dt_ms.mean : 0.0;
        rep.anomalies = time_axis_analysis(timestamps, expected_dt);

        rep.sampling_hz_est = rep.dt_available ? (1000.0 / rep.dt_ms.mean) : 0.0;

        return rep;
    }

}