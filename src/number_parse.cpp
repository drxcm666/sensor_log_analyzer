#include "number_parse.hpp"

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>


namespace sla{


/*
 * Validates whether a string represents a valid decimal number in scientific notation.
 *
 * Format: [±]digits[.digits][e|E[±]digits]
 *
 * Rules:
 * - Must start with optional sign (+ or -)
 * - Must have at least one digit before decimal point
 * - If decimal point exists, must have at least one digit after it
 * - If exponent (e/E) exists, must have optional sign followed by at least one digit
 * - No other characters allowed
 *
 * Valid examples: "42", "-3.14", "1.0e-5", "+2.5E+3"
 * Invalid examples: ".", "e5", "1.", ".5", "1.0e"
 */
bool is_simple_decimal(std::string_view s)
{
    if (s.empty())
        return false;

    size_t i = 0;

    if (s[0] == '-' || s[0] == '+')
    {
        if (s.size() == 1)
            return false;
        ++i;
    }

    size_t digits_before{0};
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
    {
        ++digits_before;
        ++i;
    }

    if (digits_before == 0)
        return false;

    size_t digits_after{0};
    if (i < s.size() && s[i] == '.')
    {
        ++i;
        if (i == s.size())
            return false;

        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
        {
            ++digits_after;
            ++i;
        }

        if (digits_after == 0)
            return false;
    }

    if (i == s.size())
        return true;

    if (s[i] != 'e' && s[i] != 'E')
        return false;

    ++i;
    if (i == s.size())
        return false;

    if (s[i] == '-' || s[i] == '+')
    {
        if (s.size() == 1)
            return false;
        ++i;
    }

    size_t exp_digits{0};
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i])))
    {
        ++exp_digits;
        ++i;
    }

    if (exp_digits == 0)
        return false;

    return i == s.size();
}


/*
 * parse_simple_double - Safely converts a string to a double value
 *
 * Parameters:
 *   s   - string to parse
 *   out - reference to variable for storing the result
 *
 * Returns: true if parsing is successful, false on error
 *
 * Processing stages:
 * 1. is_simple_decimal(s) - checks if string contains only digits, decimal point, and sign
 * 2. std::string tmp(s) - copies the string so strtod can find the null terminator
 * 3. errno = 0 - resets the global error variable before calling strtod
 * 4. std::strtod(tmp.c_str(), &end) - converts string to double, end points to unparsed part
 * 5. end != tmp.c_str() + tmp.size() - verifies that the entire string was parsed
 * 6. errno == ERANGE - checks for overflow (number too large/small)
 * 7. std::isfinite(val) - checks if result is not Infinity or NaN
 * 8. out = val - if all checks pass, stores the result in the output variable
 */
bool parse_simple_double(std::string_view s, double &out)
{
    if (!is_simple_decimal(s))
        return false;

    std::string tmp(s); // need ‘\0’ at the end for strtod
    errno = 0;
    char *end = nullptr;

    double val = std::strtod(tmp.c_str(), &end);

    if (end != tmp.c_str() + tmp.size())
        return false;

    if (errno == ERANGE)
        return false;

    if (!std::isfinite(val))
        return false;

    out = val;

    return true;
}

bool parse_row_to_array(
    const std::vector<std::string> &v,
    std::array<double, 7> &out,
    std::size_t &bad_idx,
    std::size_t EXPECTED_COLUMNS
)
{
    if (v.size() != EXPECTED_COLUMNS)
    {
        bad_idx = 0;
        return false;
    }
    
    for (std::size_t i = 0; i < EXPECTED_COLUMNS; i++)
    {
        double x;
        if (!parse_simple_double(v[i], x))
        {
            bad_idx = i;
            return false;
        }
        out[i] = x;
    }

    return true;
}

}