#include "sla/number_parse.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("parse_simple_double parses valid numbers")
{
    double x{0.0};

    REQUIRE(sla::parse_simple_double("1e3", x));
    CHECK(x == Catch::Approx(1000.0));

    REQUIRE(sla::parse_simple_double("1.0e-2", x));
    CHECK(x == Catch::Approx(0.01));
}

TEST_CASE("parse_simple_double rejects invalid numbers")
{
    double x = 0.0;
    CHECK_FALSE(sla::parse_simple_double("nan", x));
    CHECK_FALSE(sla::parse_simple_double("1.", x));
    CHECK_FALSE(sla::parse_simple_double(".5", x));
}