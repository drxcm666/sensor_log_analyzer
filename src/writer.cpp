#include "writer.hpp"

#include <string>
#include <iomanip>
#include <limits>


namespace sla{

std::filesystem::path make_calib_path(const std::filesystem::path &input)
{
    const auto parent = input.parent_path();
    const auto stem = input.stem().string();
    const auto ext = input.extension().string();

    std::string out_name = stem + "_calib" + ext;
    return parent / out_name;
}

std::filesystem::path make_clean_path(const std::filesystem::path &input)
{
    // input: data/imu_dirty.csv

    const auto parent = input.parent_path();          // "data"
    const auto stem = input.stem().string();          // "imu_dirty"
    const auto ext = input.extension().string();      // ".csv"

    std::string out_name = stem + "_clean" + ext;     // "imu_dirty_clean.csv"
    return parent / out_name;                         // "data/imu_dirty_clean.csv"
}

bool CsvWriter::open(const std::filesystem::path &out_path)
{
    path_ = out_path;

    // std::ios::out — mode: output (writing)
    // | — bitwise “or” (flag union)
    // std::ios::trunc — mode: truncate (delete old content)
    out_.open(path_, std::ios::out | std::ios::trunc);

    if (!out_.is_open()) return false;
    
    out_ << std::setprecision(std::numeric_limits<double>::max_digits10);

    return true;
}

void CsvWriter::write_header(const std::array<std::string_view, 4> &header)
{
    for (size_t i = 0; i < header.size(); i++)
    {
        out_ << header[i];
        if (i + 1 < header.size())  out_ << ',';
    }

    out_ << '\n';
}

void CsvWriter::write_row(const std::array<double, 4> &v)
{
    out_ << v[0] << ',' << v[1] << ',' << v[2] << ',' << v[3] << '\n';
    
}

void CsvWriter::close()
{
    if (out_.is_open())
        out_.close();
}


}