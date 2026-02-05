#include "cli.hpp"

#include <fmt/core.h>

namespace sla::cli {

void print_usage(std::string_view prog)
{
    fmt::println(
        "Usage:\n"
        "  {} --input <file> [--clean | --calib]\n"
        "\nOptions:\n"
        "  --input <file>   Input CSV file\n"
        "  --clean          Write cleaned CSV next to input with _clean suffix\n"
        "  --calib Calibration mode (coeffs + errors + export for plots)\n"
        "  -h, --help       Show this help\n",
        prog.empty() ? "program" : prog
    );
}

ParseResult parse_args(int argc, char* argv[])
{
    Options opt;

    // Go through all arguments (start with 1, because 0 is the name of the program)
    for (int i = 1; i < argc; ++i)
    {
        std::string_view arg = argv[i] ? std::string_view(argv[i]) : std::string_view{};

        if (arg == "--help" || arg == "-h")
        {
            opt.show_help = true;
        }
        else if (arg == "--input")
        {
            if (i + 1 >= argc || !argv[i + 1])
                return Error{"missing value after --input"};

            opt.input_file = argv[++i];
        }
        else if (arg == "--clean")
        {
            if (opt.cmd != Command::None)
                return Error{"only one command allowed"};

            opt.cmd = Command::Clean;
        }
        else if (arg == "--calib")
        {
            if (opt.cmd != Command::None)
                return Error{"only one command allowed"};
            
            opt.cmd = Command::Calib;
        }
        else
        {
            return Error{fmt::format("unknown argument: {}", arg)};
        }
    }

    if (!opt.show_help && opt.input_file.empty())
        return Error{"missing required option: --input <file>"};

    return opt;
}

}
