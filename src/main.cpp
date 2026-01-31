#include "welford_stats.hpp"
#include "clean_writer.hpp"
#include "report_json.hpp"
#include "time_axis.hpp"
#include "report.hpp"
#include "cli.hpp"
#include "csv.hpp"

#include <iostream>
#include <fmt/core.h>
#include <string>
#include <filesystem>
#include <variant>
#include <array>
#include <cmath>


int main(int argc, char *argv[])
{
    auto parse_result = sla::cli::parse_args(argc, argv);

    if (std::holds_alternative<sla::cli::Error>(parse_result))
    {
        auto err = std::get<sla::cli::Error>(parse_result);
        fmt::println(stderr, "Error: {}", err.message);
        sla::cli::print_usage(argv[0]);
        return 1;
    }

    auto opt = std::get<sla::cli::Options>(parse_result);

    if (opt.show_help)
    {
        sla::cli::print_usage(argv[0]);
        return 0;
    }

    const bool do_clean = (opt.cmd == sla::cli::Command::Clean);
    sla::CleanWriter writer;

    if (do_clean)
    {
        auto clean_path = sla::make_clean_path(opt.input_file);
        if (!writer.open(clean_path)) // Open file for writing
        {
            fmt::println(stderr, "Error: can't open file for writing: {}", clean_path.string());
            return 1;
        }

        writer.write_header(sla::EXPECTED_HEADER);
    }

    sla::WelfordStats ax, ay, az, gx, gy, gz;
    bool have_last_t = false;
    double last_t = 0.0;
    
    auto pass1 = sla::read_imu_csv_streaming(opt.input_file,
    // lambda
    [&](const std::array<double, 7> &row)
    {
        const double t = row[0];

        ax.update(row[1]);
        ay.update(row[2]);
        az.update(row[3]);
        gx.update(row[4]);
        gy.update(row[5]);
        gz.update(row[6]);

        if (do_clean)
        {
            writer.write_row(row);
        }
    });

    if (do_clean)
        writer.close();

    if (!pass1.ok)
    {
        fmt::println(stderr, "Error: {}", pass1.error);
        return 1;
    }
    
    // ???
    auto to_stats = [](const sla::WelfordStats &w)
    {
        sla::Stats s;
        if (w.count() == 0)
            return s;
        
        s.count = static_cast<int>(w.count());
        s.min = w.min();
        s.max = w.max();
        s.mean = w.mean();
        s.std = w.stddev();

        return s;
        
    };

    sla::Report report;
    report.input = pass1.input_name;
    report.counts = pass1.counts;
    report.warnings = pass1.warnings;

    report.time_axis = sla::make_time_axis_report_streaming(
        [&](const sla::TimestampVisitor &visit)
        {
            (void)sla::read_imu_csv_streaming(opt.input_file,
                [&](const std::array<double, 7> &row)
                {
                    visit(row[0]);
                });
        });

    report.statistics.ax = to_stats(ax);
    report.statistics.ay = to_stats(ay);
    report.statistics.az = to_stats(az);
    report.statistics.gx = to_stats(gx);
    report.statistics.gy = to_stats(gy);
    report.statistics.gz = to_stats(gz);
    
    auto json_path = sla::default_report_json_path(pass1.input_path);

    try
    {
        sla::write_report_json_file(report, json_path);
        fmt::println("Report written to: {}", json_path.string());
    }
    catch(const std::exception& e)
    {
        fmt::println(stderr, "Error writing JSON: {}", e.what());
        return 1;
    }


    fmt::println("\n=== Analysis Summary ===");
    fmt::println("Input file: {}", report.input);
    fmt::println("Total lines: {}", report.counts.total_lines);
    fmt::println("Parsed lines: {}", report.counts.parsed_lines);
    fmt::println("Bad lines: {}", report.counts.bad_lines);
    fmt::println("Warnings: {}", report.warnings.size());

    if (report.time_axis.dt_available)
    {
        fmt::println("\nSampling frequency: {:.2f} Hz", report.time_axis.sampling_hz_est);
        fmt::println("Time interval stats (ms):");
        fmt::println("  mean: {:.3f}", report.time_axis.dt_ms.mean);
        fmt::println("  std:  {:.3f}", report.time_axis.dt_ms.std);
    }

    return 0;
}
