// report.hpp
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "stats.hpp"
#include "time_axis.hpp"   // тут має бути TimeAxisReport/TimeAxisIssues

namespace sla {

// Рядок-попередження (j["warnings"])
struct Warning
{
    std::string message;               // "invalid value", "incorrect number of columns ..." тощо
    std::size_t line{};                // номер рядка у файлі (1-based)
    std::optional<std::size_t> column; // Може бути число або "нічого"
    std::optional<std::string> value;  // Може бути текст або "нічого"
};

// Лічильники (j["counts"])
struct Counts
{
    std::size_t data_lines{};     // у тебе це = parsed_lines (або parsed+bad+header, залежно від домовленості)
    std::size_t header_lines{};
    std::size_t parsed_lines{};
    std::size_t total_lines{};
    std::size_t empty_lines{};
    std::size_t comment_lines{};
    std::size_t bad_lines{};
};

// Статистика каналів IMU (j["statistics"]["ax"] ...)
struct ImuStatistics
{
    Stats ax{};
    Stats ay{};
    Stats az{};
    Stats gx{};
    Stats gy{};
    Stats gz{};
};

// Головний результат аналізу (весь json "j")
struct Report
{
    std::string input;               // ім’я файлу (без шляху)
    Counts counts{};
    std::vector<Warning> warnings{};
    TimeAxisReport time_axis{};      // з time_axis.hpp
    ImuStatistics statistics{};
};

}
