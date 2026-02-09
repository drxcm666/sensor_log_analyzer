#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <array>
#include <functional>

#include "report.hpp"

namespace sla {

// Without inline: Everyone who reads this line makes a copy for themselves
inline constexpr std::array<std::string_view, 4> EXPECTED_HEADER{
    "t_ms", "ax", "ay", "az"
};

struct CsvStreamResult
{
    bool ok{true};
    std::string error;
    std::filesystem::path input_path;
    std::string input_name;

    bool header_found{false};

    Counts counts;
    std::vector<Warning> warnings;

    std::size_t warnings_dropped{};
};

using CsvRowCallback = std::function<void(const std::array<double, 4>&)>;

CsvStreamResult read_imu_csv_streaming(
    const std::filesystem::path &path,
    const CsvRowCallback &on_row
);

}
