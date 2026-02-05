#pragma once

#include <array>
#include <filesystem>
#include <fstream>
#include <string_view>

namespace sla {

//  data/imu_dirty.csv -> data/imu_dirty_clean.csv
std::filesystem::path make_clean_path(const std::filesystem::path &input);

std::filesystem::path make_calib_path(const std::filesystem::path &input);

// Class that encapsulates the entire clean CSV record
class CsvWriter {
public:
    CsvWriter() = default;

    // Open file for writing
    bool open(const std::filesystem::path &out_path);

    // Is the file open
    bool is_open() const { return out_.is_open(); }

    // Path to the open file (to display a message to the user)
    const std::filesystem::path& path() const { return path_; }

    // Write down the header
    void write_header(const std::array<std::string_view, 4> &header);

    // Write one line of values (we expect == header.size)
    void write_row(const std::array<double, 4> &v);

    // Close file
    void close();

private:

    // Type: output file stream
    // Purpose: stores an open file
    std::ofstream out_;

    // Type: path to file
    // Purpose: remembers where we write
    std::filesystem::path path_;
};

}
