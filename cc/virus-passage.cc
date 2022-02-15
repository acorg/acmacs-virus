#include "acmacs-base/argv.hh"
#include "acmacs-virus/passage.hh"
#include "acmacs-virus/log.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of log enablers"}};

    argument<str_array> passages{*this, arg_name{"passage"}};
};

int main(int argc, const char* const argv[])
{
    using namespace acmacs::virus;

    int exit_code = 0;
    try {
        Options opt(argc, argv);
        acmacs::log::enable(opt.verbose);
        for (const auto& passage : opt.passages) {
            const auto result = parse_passage(passage, passage_only::no);
            fmt::print(stderr, "\"{}\":\n  \"{}\"\n  EXT: \"{}\"\n", passage, *std::get<Passage>(result), std::get<std::string>(result));
        }
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
