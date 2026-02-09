#include "sla/welford_stats.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

TEST_CASE("WelfordStats known dataset 1,2,3,4")
{
    sla::WelfordStats w;
    w.update(1);
    w.update(2);
    w.update(3);
    w.update(4);

    CHECK(w.count() == 4);
    CHECK(w.mean() == Catch::Approx(2.5));
    CHECK(w.variance() == Catch::Approx(5.0 / 3.0));
    CHECK(w.stddev() == Catch::Approx(std::sqrt(5.0 / 3.0)));
}