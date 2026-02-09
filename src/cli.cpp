#include "sla/cli.hpp"

#include <fmt/core.h>

namespace sla::cli
{

    static bool is_command(std::string_view s)
    {
        return s == "analyze" || s == "clean" || s == "calib";
    }

    static Command parse_command(std::string_view s)
    {
        if (s == "analyze")
            return Command::None;
        if (s == "clean")
            return Command::Clean;
        if (s == "calib")
            return Command::Calib;

        return Command::None;
    }

    void print_usage(std::string_view prog)
    {
        const auto p = prog.empty() ? "sla" : prog;

        fmt::println(
            "Usage:\n"
            "  {0} analyze --input <file>\n"
            "  {0} clean   --input <file>\n"
            "  {0} calib   --input <file> [--position <file>]\n"
            "\n"
            "Options:\n"
            "  --input <file>      Input CSV file\n"
            "  --position <file>   (calib) Path to POSITION.txt (default: рядом з input)\n"
            "  -h, --help          Show this help\n",
            p);
    }

    ParseResult parse_args(int argc, char *argv[])
    {
        Options opt;

        bool cmd_set_by_subcommand = false;
        int i = 1;

        if (i < argc && argv[i])
        {
            std::string_view first = argv[i] ? std::string_view(argv[i]) : std::string_view{};

            // If the first token doesn't start with '-' and matches a known command -> treat as subcommand
            if (!first.empty() && first[0] != '-' && is_command(first))
            {
                opt.cmd = parse_command(first);
                cmd_set_by_subcommand = true;
                ++i;
            }
            else if (!first.empty() && first[0] != '-' && !is_command(first))
            {
                // A positional token that is not a command => error (keeps CLI strict)
                return Error{fmt::format("unknown command: {} (expected: analyze|clean|calib)", first)};
            }
        }

        // Parse options
        for (; i < argc; ++i)
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
            else if (arg == "--position")
            {
                if (i + 1 >= argc || !argv[i + 1])
                    return Error{"missing value after --position"};

                opt.position_file = argv[++i];
            }

            /*
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
            */

            else
            {
                return Error{fmt::format("unknown argument: {}", arg)};
            }
        }

        // Validation
        if (!opt.show_help && opt.input_file.empty())
            return Error{"missing required option: --input <file>"};

        if (!opt.position_file.empty() && opt.cmd != Command::Calib)
            return Error{"--position is only valid for 'calib' command"};

        return opt;
    }

}
