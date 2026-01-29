#include "csv_split.hpp"
#include "util.hpp"


namespace sla{


/*
 * Splits a string by comma delimiters and returns a vector of trimmed strings.
 *
 * Process:
 * - Iterates through the input string looking for comma characters
 * - Extracts substrings between commas using find()
 * - Trims whitespace from each substring using trim()
 * - Stores each part (including empty ones after trimming) in the output vector
 *
 * Example: "apple, banana , cherry" -> ["apple", "banana", "cherry"]
 *
 * @param s Input string_view to split
 * @return Vector of trimmed strings
 */
std::vector<std::string> split_by_comma(std::string_view s)
{
    std::vector<std::string> out;
    size_t start = 0;

    while (start <= s.size())
    {
        size_t pos = s.find(',', start);
        if (pos == std::string_view::npos)
            pos = s.size();

        std::string_view part = trim(s.substr(start, pos - start));
        out.emplace_back(part);    // constructs a string directly

        start = pos + 1;
    }

    return out;
}


}