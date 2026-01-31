#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <functional>

#include "report.hpp"

namespace sla {

// Without inline: Everyone who reads this line makes a copy for themselves
inline constexpr std::array<std::string_view, 7> EXPECTED_HEADER{
    "t_ms", "ax", "ay", "az", "gx", "gy", "gz"
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
};

// using Meter = double;
//
// read_imu_csv — це людина, яка читає файл
// std::array<double,7> — це листок з 7 цифрами (один рядок)
// std::function — це інструкція, що робити з листком
//
// on_row({v[0], v[1], v[2], v[3], v[4], v[5], v[6]});
// Це читається так:
// “Я (read_imu_csv) зробив 7 чисел.
// Ось вони. Тримай. Виконай інструкцію on_row.”
//
// коли викликаєш read_imu_csv.
// Приклад: інструкція “надрукуй”
//
// (в іншому файлі)
// read_imu_csv("imu.csv",
//   [](const std::array<double,7>& row) {
//       std::cout << row[0] << "\n";
//   }
// );
//
// Тут ти дав інструкцію: друкувати перше число.
//
// Тут std::function зберігає НЕ числа, а оцю “дію друку” (лямбду).
// А числа (row) приходять кожного разу нові, коли читається новий рядок.
using CsvRowCallback = std::function<void(const std::array<double, 7>&)>;

CsvStreamResult read_imu_csv_streaming(
    const std::filesystem::path& path,
    const CsvRowCallback& on_row
);

}
