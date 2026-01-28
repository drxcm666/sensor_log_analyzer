#include "util.hpp"


namespace sla {

    
/*
 * Removes leading and trailing whitespace from a string view.
 *
 * How it works:
 * 1. find_first_not_of() locates the index of the first character that is NOT whitespace
 * 2. If no such character exists (npos), the string is all whitespace, so return empty
 * 3. find_last_not_of() locates the index of the last character that is NOT whitespace
 * 4. substr() extracts the substring from start position with length (end - start + 1)
 *    The "+1" is needed because end is an index, and we want to include that character
 *
 * Example: "  hello world  " → "hello world"
 */
std::string_view trim(std::string_view s)
{
    size_t start = s.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string_view::npos)
        return {};
    size_t end = s.find_last_not_of(" \t\n\r\f\v");

    return s.substr(start, end - start + 1);
}

}