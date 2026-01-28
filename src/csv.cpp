#include "csv.hpp"

#include "util.hpp"         // trim(std::string_view)
#include "csv_split.hpp"    // split_by_comma(std::string_view) -> std::vector<std::string>
#include "number_parse.hpp" // convert_all_or_bad_syntax(std::vector<std::string>) -> variant<vector<double>, size_t>

#include <fstream>
#include <string>
#include <string_view>
#include <algorithm>


namespace sla {


static bool is_expected_header(const std::vector<std::string> &tokens)
{
    if (tokens.size() != EXPECTED_HEADER.size()) return false;

    for (size_t i = 0; i < EXPECTED_HEADER.size(); i++)
    {
        if (tokens[i] != EXPECTED_HEADER[i]) return false;
    }
    
    return true;
}

CsvResult read_imu_csv(const std::filesystem::path& path)
{
    CsvResult r;
    r.input_path = path;
    r.input_name = path.filename().string();

    // Open the file for reading 
    std::ifstream file(path);
    if (!file)
    {
        r.ok = false;
        r.error = "Error, can't open file: " + path.string();
        return r;
    }
    
    constexpr std::size_t EXPECTED_COLUMNS = 7;

    std::string line;

    // Читаємо рядок за рядком
    while (std::getline(file, line))
    {
        r.counts.total_lines++;

        std::string_view trimmed = trim(line);

        // empty
        if (trimmed.empty())
        {
            r.counts.empty_lines++;
            continue;
        }

        // comment
        if (trimmed.front() == '#')
        {
            r.counts.comment_lines++;
            continue;
        }
        
        // split
        auto tokens = split_by_comma(trimmed);

        // columns count check
        if (tokens.size() != EXPECTED_COLUMNS)
        {
            r.counts.bad_lines++;
            r.warnings.push_back(Warning{
                "incorrect number of columns (expected 7, got " + std::to_string(tokens.size()) + ")",
                r.counts.total_lines,
                std::nullopt,
                std::nullopt
            });
            continue;
        }
        
        // header check
        if (!r.header_found && is_expected_header(tokens))
        {
            r.header_found = true;
            r.counts.header_lines++;
            continue;
        }
        
        // parse numbers: either all 7 ok, or return index of first bad token
        auto parsed = convert_all_or_bad_syntax(tokens);

        if (std::holds_alternative<std::vector<double>>(parsed))
        {
            const auto &v = std::get<std::vector<double>>(parsed);

            r.data.t_ms.push_back(v[0]);
            r.data.ax.push_back(v[1]);
            r.data.ay.push_back(v[2]);
            r.data.az.push_back(v[3]);
            r.data.gx.push_back(v[4]);
            r.data.gy.push_back(v[5]);
            r.data.gz.push_back(v[6]);

            r.counts.parsed_lines++;
        }
        else
        {
            // Індекс помилкового токена
            std::size_t bad_idx = std::get<std::size_t>(parsed);
            r.counts.bad_lines++;

            Warning w;
            w.line = r.counts.total_lines;
            w.message = "invalid value";
            w.column = bad_idx + 1;
            w.value = std::optional<std::string>(tokens[bad_idx]);

            // Moves the entire structure “w” into the vector, rather than copying it
            r.warnings.push_back(std::move(w));
        }
    }
    
    return r;
}


}