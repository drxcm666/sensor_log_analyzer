#pragma once

#include <cmath>
#include <cstddef>
#include <functional>

#include "welford_stats.hpp"


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

// function that takes a single timestamp
using TimestampVisitor = std::function<void(double)>;

// “function-stream”: “You give me ‘visit’, and I will call visit(t) for every t that I find.”
using TimestampStream = std::function<void(const TimestampVisitor&)>;

TimeAxisReport make_time_axis_report_streaming(const TimestampStream &stream);


}