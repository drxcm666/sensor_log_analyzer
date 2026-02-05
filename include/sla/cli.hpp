#pragma once

#include <string>
#include <string_view>
#include <variant>

namespace sla::cli {

// enum class is used to restrict the value to a fixed set of valid commands
// and provide type safety compared to bools, ints, or strings.
enum class Command
{
    None,   // ./program --input data.csv  # Command::None (analysis only)
    Clean,   // ./program --input data.csv --clean  # Command::Clean (analysis + record clean CSV)
    Calib
};

struct Options
{
    std::string input_file;
    Command cmd{Command::None};
    bool show_help{false};
};

struct Error
{
    std::string message;
};

using ParseResult = std::variant<Options, Error>;
ParseResult parse_args(int argc, char* argv[]);

void print_usage(std::string_view prog);

}
