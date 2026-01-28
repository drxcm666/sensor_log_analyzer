#pragma once
#include <vector>
#include <cmath>
#include <cstddef>
#include "stats.hpp"


namespace sla{


struct TimeAxisIssues
{
    size_t non_increasing{};
    size_t duplicates{};
    size_t gaps{};
};

struct TimeAxisReport
{
    bool dt_available{};
    Stats dt_ms{};
    double sampling_hz_est{};
    TimeAxisIssues anomalies{};
};

// Calculates only positive dt between adjacent t[i]-t[i-1]
std::vector<double> compute_positive_dts(const std::vector<double> &t);

// Analysis of the time axis by expected dt (often mean dt)
TimeAxisIssues time_axis_analysis(const std::vector<double> &t, double expected_dt);

// dt_values -> dt_stats -> anomalies -> sampling_hz_est
TimeAxisReport make_time_axis_report(const std::vector<double> &timestamps);


}