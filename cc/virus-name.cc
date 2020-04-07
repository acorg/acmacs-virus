#include "acmacs-base/argv.hh"
#include "acmacs-base/read-file.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-virus/virus-name-normalize.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str> from_file{*this, 'f', "from", desc{"read names from file (one per line)"}};
    argument<str_array> names{*this, arg_name{"name"}};
};

int main(int argc, const char* const* argv)
{
    int exit_code = 0;
    try {
        Options opt(argc, argv);
        if (opt.from_file) {
            const std::string lines = acmacs::file::read(opt.from_file);
            size_t lines_read{0}, succeeded{0}, failed{0};
            for (const auto& line : acmacs::string::split(lines, "\n")) {
                ++lines_read;
                const auto [fields, messages] = acmacs::virus::name::parse(line);
                if (!messages.empty()) {
                    ++failed;
                    // AD_WARNING("{}", messages);
                }
                else
                    ++succeeded;
                // fmt::print("{} -> {}\n", line, fields);
            }
            fmt::print("Lines: {:6d}\nGood:  {:6d}\nBad:   {:6d}\n", lines_read, succeeded, failed);
        }
        else if (!opt.names.empty()) {
            for (const auto& src : opt.names) {
                const auto [fields, messages] = acmacs::virus::name::parse(src);
                if (!messages.empty())
                    AD_WARNING("{}", messages);
                fmt::print("{} -> {}\n", src, fields);
            }
        }
        else
            throw std::runtime_error{fmt::format("Usage: {} [-h] [-f <filename>] [<name> ...]", argv[0])};
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
