#include "csv.hpp"

#include "util.hpp"         // trim(std::string_view)
#include "csv_split.hpp"    // split_csv(...) + SplitStatus
#include "number_parse.hpp" // parse_row_to_array_sv(...)

#include <fstream>
#include <string>
#include <string_view>


namespace sla {


static bool is_expected_header(const std::array<std::string_view, 4> &tokens)
{
    if (tokens.size() != EXPECTED_HEADER.size()) return false;

    for (std::size_t i = 0; i < EXPECTED_HEADER.size(); i++)
    {
        if (tokens[i] != EXPECTED_HEADER[i]) return false;
    }
    
    return true;
}



CsvStreamResult read_imu_csv_streaming(
    const std::filesystem::path& path,
    const CsvRowCallback& on_row
    // on_row — a “handler function” that is called for each valid row
    // “You (main) give me a function-handler, and when I get a valid string, I'll call it.”
    // implicitly: CsvRowCallback on_row = <lambda from main>;
)
{
    CsvStreamResult r;
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

    std::string line;

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
        
        std::array<std::string_view, 4> tokens;
        std::size_t actual_cols = 0;

        // split
        auto split_status = split_csv(trimmed, tokens, actual_cols);

        // columns count check
        if (split_status != sla::SplitStatus::Ok)
        {
            r.counts.bad_lines++;
            r.warnings.push_back(Warning{
                "incorrect number of columns (expected 4, got " + std::to_string(actual_cols) + ")",
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

        std::array<double, 4> row{};
        std::size_t bad_idx{0};

        if (parse_row_to_array_sv(tokens, row, bad_idx)) // here, “row” from main is filled with numbers
        {
            r.counts.parsed_lines++;

            // ??
            // using Meter = double;
            //
            // read_imu_csv — це людина, яка читає файл
            // std::array<double,4> — це листок з 4 цифрами (один рядок)
            // std::function — це інструкція, що робити з листком
            //
            // on_row({v[0], v[1], v[2], v[3]});
            // Це читається так:
            // “Я (read_imu_csv) зробив 7 чисел.
            // Ось вони. Тримай. Виконай інструкцію on_row.”
            //
            // коли викликаєш read_imu_csv.
            // Приклад: інструкція “надрукуй”
            //
            // (в іншому файлі)
            // read_imu_csv("imu.csv",
            //   [](const std::array<double,4>& row) {
            //       std::cout << row[0] << "\n";
            //   }
            // );
            //
            // Тут ти дав інструкцію: друкувати перше число.
            //
            // Тут std::function зберігає НЕ числа, а оцю “дію друку” (лямбду).
            // А числа (row) приходять кожного разу нові, коли читається новий рядок.
            //
            //--------------------------------------
            // if (on_row)
            // checks whether the string handler has been passed, and if so, calls it
            // sla::read_imu_csv_streaming(opt.input_file, [&](const std::array<double, 4>& row)
            if (on_row)
            {
                on_row(row); // = “execute the callback that was passed to me.”
            }
        }
        else
        {
            r.counts.bad_lines++;

            Warning w;
            w.line = r.counts.total_lines;
            w.message = "invalid value";
            w.column = bad_idx + 1;
            w.value = std::string(tokens[bad_idx]);

            r.warnings.push_back(std::move(w));
        }
    }

    return r;
}


}