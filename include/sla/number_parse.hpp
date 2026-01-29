#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <variant>
#include <array>


namespace sla{


bool is_simple_decimal(std::string_view s);

bool parse_simple_double(std::string_view s, double &out);

// streaming-friendly parse (no vector<double>)
bool parse_row_to_array(
    const std::vector<std::string>& v,
    std::array<double, 7>& out,
    std::size_t& bad_idx,
    std::size_t EXPECTED_COLUMNS
);

}