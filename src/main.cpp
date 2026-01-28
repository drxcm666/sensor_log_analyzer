#include <fmt/core.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <variant>
#include <array>

#include "stats.hpp"
#include "time_axis.hpp"
#include "report.hpp"
#include "csv.hpp"
#include "clean_writer.hpp"
#include "report_json.hpp"
#include "cli.hpp"


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


    /*
        csv_result
    ├── ok = true
    ├── error = ""
    ├── input_path = "d:/data/imu_dirty.csv"
    ├── input_name = "imu_dirty.csv"
    ├── header_found = true
    │
    ├── counts
    │   ├── total_lines = 1005
    │   ├── empty_lines = 2
    │   ├── comment_lines = 1
    │   ├── header_lines = 1
    │   ├── parsed_lines = 998
    │   └── bad_lines = 3
    │
    ├── data
    │   ├── t_ms = [1000.0, 1010.5, 1020.3, ...] (998 елементів)
    │   ├── ax = [0.12, 0.15, 0.11, ...]         (998 елементів)
    │   ├── ay = [-0.05, -0.03, -0.04, ...]      (998 елементів)
    │   ├── az = [9.78, 9.81, 9.79, ...]         (998 елементів)
    │   ├── gx = [0.01, 0.02, 0.01, ...]         (998 елементів)
    │   ├── gy = [-0.01, 0.00, -0.01, ...]       (998 елементів)
    │   └── gz = [0.00, 0.01, 0.00, ...]         (998 елементів)
    │
    └── warnings = [
            {message="invalid value", line=15, column=3, value="abc"},
            {message="incorrect number of columns", line=42, ...},
            {message="invalid value", line=89, column=5, value="NaN"}
        ]
    */
    auto csv_result = sla::read_imu_csv(opt.input_file);

    if (!csv_result.ok)
    {
        fmt::println(stderr, "Error: {}", csv_result.error);
        return 1;
    }

    if (opt.cmd == sla::cli::Command::Clean)
    {
        auto clean_path = sla::make_clean_path(csv_result.input_path);

        sla::CleanWriter writter;

        if (!writter.open(clean_path))
        {
            fmt::println(stderr, "Error: can't open file for writing: {}", clean_path.string());
            return 1;
        }

        writter.write_header(sla::EXPECTED_HEADER);

        for (size_t i = 0; i < csv_result.data.t_ms.size(); i++)
        {
            std::vector<double> row = {
                csv_result.data.t_ms[i],
                csv_result.data.ax[i],
                csv_result.data.ay[i],
                csv_result.data.az[i],
                csv_result.data.gx[i],
                csv_result.data.gy[i],
                csv_result.data.gz[i]
            };

            writter.write_row(row);
        }
        
        writter.close();
        fmt::println("Clean CSV written to: {}", clean_path.string());
    }
        
    sla::Report report;
    report.input = csv_result.input_name;
    report.counts = csv_result.counts;
    report.warnings = csv_result.warnings;

    report.time_axis = sla::make_time_axis_report(csv_result.data.t_ms);

    report.statistics.ax = sla::calculate_stats(csv_result.data.ax);
    report.statistics.ay = sla::calculate_stats(csv_result.data.ay);
    report.statistics.az = sla::calculate_stats(csv_result.data.az);
    report.statistics.gx = sla::calculate_stats(csv_result.data.gx);
    report.statistics.gy = sla::calculate_stats(csv_result.data.gy);
    report.statistics.gz = sla::calculate_stats(csv_result.data.gz);
    
    auto json_path = sla::default_report_json_path(csv_result.input_path);

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
