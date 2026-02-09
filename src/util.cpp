#include "sla/util.hpp"


namespace sla {

std::string_view trim(std::string_view s)
{
    size_t start = s.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string_view::npos)
        return {};
    size_t end = s.find_last_not_of(" \t\n\r\f\v");

    return s.substr(start, end - start + 1);
}

}