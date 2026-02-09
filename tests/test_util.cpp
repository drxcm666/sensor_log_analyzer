#include "sla/util.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("trim removes leading/trailing whitespace")
{
    CHECK(sla::trim(" hello ") == "hello");
    CHECK(sla::trim("\thello\t") == "hello");
    CHECK(sla::trim("hello") == "hello");
}

TEST_CASE("trim handles empty and all-whitespace")
{
    CHECK(sla::trim("") == "");
    CHECK(sla::trim("   ") == "");
    CHECK(sla::trim("\t\n\r") == "");
}