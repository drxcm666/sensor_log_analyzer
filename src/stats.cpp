#include "stats.hpp"
#include <cmath>


namespace sla{


/*
 * Calculates statistical measures for a collection of numerical values.
 *
 * How it works:
 * 1. Counts the total number of values in the input vector
 * 2. Finds minimum and maximum values in a single pass through the data
 * 3. Calculates the sum of all values to determine the arithmetic mean
 * 4. Computes the mean by dividing the sum by the count
 * 5. Calculates variance by summing squared differences from the mean
 * 6. Applies Bessel's correction (n-1 divisor) to get sample standard deviation
 * 7. Returns 0.0 for standard deviation if only one value exists
 *
 * Example: [1.0, 2.0, 3.0, 4.0, 5.0] → Stats{count=5, min=1.0, max=5.0, mean=3.0, std=1.58...}
 *
 * @param values A vector of double values to analyze
 * @return A Stats structure containing count, min, max, mean, and std
 *
 * @note Returns default Stats object if input vector is empty
 * @complexity Time: O(n) | Space: O(1) auxiliary space
 */
Stats calculate_stats(const std::vector<double> &values)
{
    Stats stats;
    if (values.empty())
        return stats;

    stats.count = values.size();

    double sum{0.0};
    double min_val = values[0];
    double max_val = values[0];

    for (double val : values)
    {
        sum += val;
        if (val < min_val)
            min_val = val;
        if (val > max_val)
            max_val = val;
    }

    stats.min = min_val;
    stats.max = max_val;
    stats.mean = sum / stats.count;

    double variance_sum{0.0};
    for (double val : values)
    {
        double diff = val - stats.mean;
        variance_sum += diff * diff;
    }
    if (values.size() > 1)
    {
        stats.std = std::sqrt(variance_sum / (values.size() - 1)); // need * Welford
    }
    else
    {
        stats.std = 0.0;
    }

    return stats;
}


}