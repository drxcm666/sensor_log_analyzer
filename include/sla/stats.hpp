#pragma once
#include <vector>


namespace sla{


struct Stats
{
    int count{};
    double min{};
    double max{};
    double mean{};
    double std{};
};

Stats calculate_stats(const std::vector<double> &values);

}