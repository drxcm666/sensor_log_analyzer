#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "welford_stats.hpp"
#include "time_axis.hpp"

namespace sla {

// (j["warnings"])
struct Warning
{
    std::string message;               // "invalid value", "incorrect number of columns ..." ...
    std::size_t line{};
    std::optional<std::size_t> column; // Can be text or “nothing”
    std::optional<std::string> value;  // Can be text or “nothing”
};

// (j["counts"])
struct Counts
{
    std::size_t data_lines{};
    std::size_t header_lines{};
    std::size_t parsed_lines{};
    std::size_t total_lines{};
    std::size_t empty_lines{};
    std::size_t comment_lines{};
    std::size_t bad_lines{};
};

// (j["statistics"]["ax"] ...)
struct ImuStatistics
{
    Stats ax{};
    Stats ay{};
    Stats az{};
};

// Main result of the analysis (all json “j”)
struct Report
{
    std::string input;               // file name (without path)
    Counts counts{};
    std::vector<Warning> warnings{};
    TimeAxisReport time_axis{};
    ImuStatistics statistics{};
    std::size_t warnings_dropped{};
};

}
