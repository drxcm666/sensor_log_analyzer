#include "sla/time_axis.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <vector>

static sla::TimeAxisReport run_time_axis(const std::vector<double>& ts)
{
    return sla::make_time_axis_report_streaming(
        [&](const sla::TimestampVisitor& visit)
        {
            for (double t : ts) visit(t);
        }
    );
}

TEST_CASE("time_axis: regular dt")
{
    auto rep = run_time_axis({0.0, 10.0, 20.0, 30.0});
    CHECK(rep.dt_available);
    CHECK(rep.dt_ms.count == 3);
    CHECK(rep.dt_ms.mean == Catch::Approx(10.0));
    CHECK(rep.sampling_hz_est == Catch::Approx(100.0)); // 1000/10
    CHECK(rep.anomalies.duplicates == 0);
    CHECK(rep.anomalies.non_increasing == 0);
    CHECK(rep.anomalies.gaps == 0);
}

TEST_CASE("time_axis: duplicate dt=0")
{
    auto rep = run_time_axis({0.0, 10.0, 10.0, 20.0});
    CHECK(rep.dt_available);
    CHECK(rep.anomalies.duplicates == 1);
    CHECK(rep.anomalies.non_increasing == 0);
}

TEST_CASE("time_axis: non-increasing dt<0")
{
    auto rep = run_time_axis({0.0, 10.0, 5.0, 20.0});
    CHECK(rep.dt_available);
    CHECK(rep.anomalies.non_increasing == 1);
    CHECK(rep.anomalies.duplicates == 0);
}

TEST_CASE("time_axis: gap dt > 2*mean_dt")
{
    auto rep = run_time_axis({0.0, 10.0, 20.0, 61.0});
    CHECK(rep.dt_available);
    CHECK(rep.anomalies.gaps == 1);
}

TEST_CASE("time_axis: no positive dt -> dt_available false")
{
    auto rep = run_time_axis({0.0, 0.0, 0.0});
    CHECK(!rep.dt_available);
    CHECK(rep.dt_ms.count == 0);
    CHECK(rep.anomalies.duplicates == 2);
}