#include "sla/number_parse.hpp"

#include <cctype>
#include <cmath>
#include <system_error>  
#include <fast_float/fast_float.h>


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
        ++i;

        if (i == s.size())
            return false;
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
 * Processing stages:
 * 1. is_simple_decimal(s) - validates allowed format (sign, digits, '.', exponent)
 * 2. begin/end pointers - use the string_view range directly (no allocations)
 * 3. fast_float::from_chars(begin, end, out, chars_format::general)
 *    - parses the number and returns {ptr, ec}
 * 4. ec == std::errc() - ensures no conversion error
 * 5. ptr == end - ensures the entire string was consumed
 * 6. std::isfinite(out) - rejects Inf/NaN
 */
bool parse_simple_double(std::string_view s, double &out)
{
    if (!is_simple_decimal(s))
        return false;

    const char *begin = s.data();
    const char *end = s.data() + s.size();

    auto result = fast_float::from_chars(begin, end, out, 
                                         fast_float::chars_format::general);

    if (result.ec != std::errc() || result.ptr != end)
        return false;
    
    if (std::isinf(out) || std::isnan(out))
        return false;
    
    return true;
}

bool parse_row_to_array_sv(
    const std::array<std::string_view, 4> &tokens,
    std::array<double, 4> &out,
    std::size_t &bad_idx)
{
    for (std::size_t i = 0; i < out.size(); i++)
    {
        double x;
        if (!parse_simple_double(tokens[i], x))
        {
            bad_idx = i;
            return false;
        }
        out[i] = x;
    }
    
    return true;
}

}