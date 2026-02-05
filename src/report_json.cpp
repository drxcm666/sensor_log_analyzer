#include "report_json.hpp"

#include <fstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace sla
{


// Stats input: count=100, min=0.5, max=9.8, mean=5.2, std=2.1
// JSON output: {"count": 100, "min": 0.5, "max": 9.8, "mean": 5.2, "std": 2.1}
static nlohmann::ordered_json stats_to_json(const Stats &s)
{
    return nlohmann::ordered_json{
        {"count", s.count},
        {"min", s.min},
        {"max", s.max},
        {"mean", s.mean},
        {"std", s.std}};
}

// Warning input: line=15, message="invalid number", column=3, value="abc"
// JSON output: {"line": 15, "message": "invalid number", "column": 3, "value": "abc"}
static nlohmann::ordered_json warning_to_json(const Warning &w)
{
    nlohmann::ordered_json j{
        {"line", w.line},
        {"message", w.message}};

    if (w.column.has_value())
        j["column"] = *w.column;

    if (w.value)
        j["value"] = *w.value;

    return j;
}

static nlohmann::ordered_json counts_to_json(const Counts &c)
{
    return nlohmann::ordered_json{
        {"total_lines", c.total_lines},
        {"empty_lines", c.empty_lines},
        {"comment_lines", c.comment_lines},
        {"header_lines", c.header_lines},
        {"parsed_lines", c.parsed_lines},
        {"bad_lines", c.bad_lines},
    };
}

static nlohmann::ordered_json time_axis_to_json(const TimeAxisReport &t)
{
    // Initialize an empty ordered JSON
    nlohmann::ordered_json j;

    j["dt_available"] = t.dt_available;

    if (t.dt_available)
    {
        j["dt_ms"] = stats_to_json(t.dt_ms);
        j["sampling_hz_est"] = t.sampling_hz_est;
    }
    else
    {
        j["dt_ms"] = nullptr;
        j["sampling_hz_est"] = nullptr;
    }

    j["anomalies"] = {
        {"non_increasing", t.anomalies.non_increasing},
        {"duplicates", t.anomalies.duplicates},
        {"gaps", t.anomalies.gaps},
    };

    return j;
}

static nlohmann::ordered_json statistics_to_json(const ImuStatistics &s)
{
    return nlohmann::ordered_json{
        {"ax", stats_to_json(s.ax)},
        {"ay", stats_to_json(s.ay)},
        {"az", stats_to_json(s.az)},
    };
}

// ---------------------------- public functions of the module  ----------------------------

nlohmann::ordered_json report_to_json(const Report &r)
{
    nlohmann::ordered_json j;

    j["input"] = r.input;

    j["counts"] = counts_to_json(r.counts);

    // Reserve space for warnings and add each of them
    j["warnings"] = nlohmann::ordered_json::array();

    // get_ref<...>() — gets a reference to the internal JSON array
    // .reserve(r.warnings.size()) — reserves space for all warnings
    j["warnings"].get_ref<nlohmann::ordered_json::array_t &>().reserve(r.warnings.size());
    for (const auto &w : r.warnings)
        j["warnings"].push_back(warning_to_json(w));

    j["time_axis"] = time_axis_to_json(r.time_axis);

    j["statistics"] = statistics_to_json(r.statistics);

    return j;
}


std::filesystem::path default_report_json_path(const std::filesystem::path &input_path)
{
    // Copy the input path and replace the extension with .json
    std::filesystem::path out = input_path;
    out.replace_extension(".json");
    return out;
}


void write_report_json_file(const Report &r, const std::filesystem::path &output_path)
{
    // Open file for writing
    std::ofstream f(output_path);

    if (!f)
        throw std::runtime_error("Failed to open file for writing: " + output_path.string());

    // Convert the report to JSON and write it with indentation
    nlohmann::ordered_json j = report_to_json(r);
    f << j.dump(4);
}

} 