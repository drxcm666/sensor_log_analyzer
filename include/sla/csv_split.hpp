#pragma once

#include <string_view>
#include <array>
#include <cstddef>


namespace sla{

enum class SplitStatus{Ok, TooFew, TooMany};

SplitStatus split_csv (
    std::string_view s, 
    std::array<std::string_view, 4> &out, 
    std::size_t &actual_columns); 

}