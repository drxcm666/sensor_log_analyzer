#pragma once

#include <cstddef>
#include <cmath>
#include <limits>


namespace sla{

struct Stats
{
    int count{};
    double min{};
    double max{};
    double mean{};
    double std{};
};


class WelfordStats{
private:
    std::size_t count_;
    double mean_;
    double M2_;
    double min_;
    double max_;


public:
    WelfordStats()
        : count_(0),
          mean_(0.0),
          M2_(0.0),
          min_(std::numeric_limits<double>::quiet_NaN()),
          max_(std::numeric_limits<double>::quiet_NaN()) {}

    void update(double value)
    {
        if (count_ == 0)
        {
            min_ = value;
            max_ = value;
        }
        else
        {
            if (value < min_) min_ = value;
            if (value > max_) max_ = value;
        }

        count_++;
        double delta = value - mean_;
        mean_ += delta / static_cast<double>(count_);
        double delta2 = value - mean_;
        M2_ += delta * delta2;
    }

    std::size_t count() const { return count_; }
    double mean() const { return mean_; }

    double min() const {
        return (count_ == 0) ? std::numeric_limits<double>::quiet_NaN() : min_;
    }

    double max() const {
        return (count_ == 0) ? std::numeric_limits<double>::quiet_NaN() : max_;
    }

    // sample variance (with Bessel correction)
    double variance () const{
        return (count_ > 1) ? M2_ / (count_ - 1) : 0.0;
    }

    double stddev() const { return std::sqrt(variance()); }

    void reset() 
    { 
        count_ = 0;
        mean_ = 0.0;
        M2_ = 0.0;
        min_ = std::numeric_limits<double>::quiet_NaN();
        max_ = std::numeric_limits<double>::quiet_NaN();
    }
};

}