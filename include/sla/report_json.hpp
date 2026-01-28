#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>

#include "report.hpp"

namespace sla {

// Creates a JSON object from the already compiled Report
// Does NOT write to a file, only forms the JSON structure
nlohmann::ordered_json report_to_json(const Report& r);

// Selects the path to the output .json based on input_path:
// data/imu_dirty.csv -> data/imu_dirty.json
std::filesystem::path default_report_json_path(const std::filesystem::path& input_path);

// Writes report_to_json(r) to the output_path file (with indentation)
// Throws std::runtime_error if the file cannot be opened
void write_report_json_file(const Report& r, const std::filesystem::path& output_path);

} 
