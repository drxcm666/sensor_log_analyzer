#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <variant>


namespace sla{


bool is_simple_decimal(std::string_view s);

bool parse_simple_double(std::string_view s, double &out);

std::variant<std::vector<double>, size_t>
convert_all_or_bad_syntax(const std::vector<std::string> &v);


}