#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include "report.hpp"

namespace sla {

// Without inline: Everyone who reads this line makes a copy for themselves
inline constexpr std::array<std::string_view, 7> EXPECTED_HEADER{
    "t_ms", "ax", "ay", "az", "gx", "gy", "gz"
};

struct CsvData
{
    std::vector<double> t_ms;
    std::vector<double> ax;
    std::vector<double> ay;
    std::vector<double> az;
    std::vector<double> gx;
    std::vector<double> gy;
    std::vector<double> gz;
};

struct CsvResult
{
    bool ok{true};
    std::string error;                  // якщо ok=false
    std::filesystem::path input_path;
    std::string input_name;             // тільки ім'я файлу

    bool header_found{false};

    Counts counts;
    CsvData data;
    std::vector<Warning> warnings;
};

// Читає IMU CSV формату: t_ms,ax,ay,az,gx,gy,gz (7 колонок)
// Пропускає порожні рядки та коментарі (#...)
// Заголовок розпізнає як точний збіг EXPECTED_HEADER
// Рядки з невірною кількістю колонок або з нечисловими значеннями -> bad_lines + warnings
CsvResult read_imu_csv(const std::filesystem::path& path);

}
