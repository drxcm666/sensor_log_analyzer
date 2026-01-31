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



// time_axis.hpp
// #pragma once
// #include <functional>
//
// namespace sla {
// using TimestampVisitor = std::function<void(double)>;
// using TimestampStream  = std::function<void(const TimestampVisitor&)>;
//
// double sum_timestamps(const TimestampStream& stream);
// -----------------------------
// time_axis.cpp
// #include "time_axis.hpp"
//
// namespace sla {
// double sum_timestamps(const TimestampStream& stream)
// {
//     double sum = 0.0;
//
//     stream([&](double t) {   // visitor
//         sum += t;            // обробка одного timestamp
//     });
//
//     return sum;
// }
// }
// ------------------------------
// main.cpp
// #include <iostream>
// #include <vector>
// #include "time_axis.hpp"
//
// int main()
// {
//     std::vector<double> t = {1.0, 2.0, 3.0};
//
//     sla::TimestampStream stream = [&](const sla::TimestampVisitor& visit) {
//         for (double x : t) visit(x);   // "віддаємо" x по одному
//     };
//
//     std::cout << sla::sum_timestamps(stream) << "\n"; // 6
// }

// function that takes a single timestamp
using TimestampVisitor = std::function<void(double)>;

// “function-stream”: “You give me ‘visit’, and I will call visit(t) for every t that I find.”
using TimestampStream = std::function<void(const TimestampVisitor)>;

TimeAxisReport make_time_axis_report_streaming(const TimestampStream &stream);


}