#pragma once

#include "welford_stats.hpp"

#include <filesystem>
#include <string>


namespace sla {

struct Vec3 { double x{}, y{}, z{}; };

struct Mat3 { double a[3][3]{}; };

struct Position
{
    int inner{};
    int outer{};
};


struct CalibrationOptions
{
    std::filesystem::path input_path;
    std::filesystem::path position_path;
    std::filesystem::path output_path;

    double gravity{9.81054};
    double steady_start_frac{0.3};
    double steady_end_frac{0.7};
};


struct CalibrationResult
{
    bool ok{true};
    std::string error;

    std::size_t parsed_lines{};

    int npos{};
    int L{};                 // lines per position
    int steady_start{};      // index of the beginning of the steady window within the block
    int steady_end{};        // index of the end of the steady window within the block

    Mat3 M{};                // measurement model a_meas = M*a_true + b
    Vec3 b{};                // bias in the measurement model
    Mat3 C{};                // correction matrix = inv(M)
    Vec3 d{};                // correction bias = b

    double max_abs_mag_raw_all{};           // max(|mag_raw - g|) (all)
    double max_abs_mag_raw_steady{};        // per steady window
    double max_abs_mag_corr_steady{};       // per steady window (after correction)

    // Statistics |a_corr| in the steady window
    Stats mag_corr_stats{};
};


CalibrationResult run_calibration(const CalibrationOptions &opt);




}