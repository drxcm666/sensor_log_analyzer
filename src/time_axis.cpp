#include "time_axis.hpp"

#include <cmath>

namespace sla
{

    TimeAxisReport make_time_axis_report_streaming(const TimestampStream &stream)
    {
        TimeAxisReport rep{};

        // Pass 1: dt stats
        sla::WelfordStats dt_stats;
        bool have_last = false;
        double last = 0.0;

        stream([&](double t)
        {
            if (have_last)
            {
                const double dt = t - last;
                if (dt > 0.0)
                    dt_stats.update(dt);
            }
            last = t;
            have_last = true;
        });

        if (dt_stats.count() > 0)
        {
            rep.dt_ms.count = static_cast<int>(dt_stats.count());
            rep.dt_ms.min = dt_stats.min();
            rep.dt_ms.max = dt_stats.max();
            rep.dt_ms.mean = dt_stats.mean();
            rep.dt_ms.std = dt_stats.stddev();
        }

        constexpr double EPS = 1e-9;
        rep.dt_available = (rep.dt_ms.count > 0) && (rep.dt_ms.mean > EPS);
        rep.sampling_hz_est = rep.dt_available ? (1000.0 / rep.dt_ms.mean) : 0.0;

        // Pass 2: anomalies with final expected_dt
        TimeAxisIssues anomalies{};
        const double expected_dt = rep.dt_available ? rep.dt_ms.mean : 0.0;

        bool have_last2 = false;
        double last2 = 0.0;

        stream([&](double t)
        {
            if (have_last2)
            {
                const double dt = t - last2;
                
                if (dt < -EPS)
                anomalies.non_increasing++;
                else if (std::abs(dt) <= EPS)
                    anomalies.duplicates++;

                if (expected_dt > EPS && dt > EPS && dt > 2 * expected_dt)
                    anomalies.gaps++;

            }
            last2 = t;
            have_last2 = true;
        });

        rep.anomalies = anomalies;
        return rep;
    }

}