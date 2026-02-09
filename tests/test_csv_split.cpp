#include "sla/csv_split.hpp"

#include <catch2/catch_test_macros.hpp>
#include <array>
#include <string_view>
#include <cstddef>


TEST_CASE("split_csv Ok with exactly 4 columns")
{
    std::string line = "1,2,3,4";
    std::array<std::string_view, 4> out;
    std::size_t cols{0};

    auto st = sla::split_csv(line, out, cols);

    CHECK(st == sla::SplitStatus::Ok);
    CHECK(cols == 4);
    CHECK(out[0] == "1");
    CHECK(out[3] == "4");
}

TEST_CASE("split_csv TooFew / TooMany")
{
    {
        std::string line = "1,2,3";
        std::array<std::string_view, 4> out{};
        std::size_t cols = 0;

        auto st = sla::split_csv(line, out, cols);
        CHECK(st == sla::SplitStatus::TooFew);
        CHECK(cols == 3);
    }

    {
        std::string line = "1,2,3,4,5";
        std::array<std::string_view, 4> out{};
        std::size_t cols = 0;

        auto st = sla::split_csv(line, out, cols);
        CHECK(st == sla::SplitStatus::TooMany);
        CHECK(cols == 5);
        CHECK(out[0] == "1");
        CHECK(out[3] == "4");
    }
}