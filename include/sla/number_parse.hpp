#pragma once

#include <string>
#include <string_view>
#include <array>
#include <cstddef>


namespace sla{


bool is_simple_decimal(std::string_view s);

bool parse_simple_double(std::string_view s, double &out);

bool parse_row_to_array_sv(
    const std::array<std::string_view, 4> &tokens,
    std::array<double, 4> &out,
    std::size_t &bad_idx);

}