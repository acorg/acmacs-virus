#include "acmacs-virus/passage.hh"

// ----------------------------------------------------------------------

int main(int argc, const char* const* argv)
{
    using namespace acmacs::virus;

    int exit_code = 0;
    try {
        if (argc > 1) {
            for (int arg = 1; arg < argc; ++arg) {
                const auto result = parse_passage(argv[arg], passage_only::no);
                fmt::print(stderr, "\"{}\":\n  \"{}\"\n  EXT: \"{}\"\n", argv[arg], *std::get<Passage>(result), std::get<std::string>(result));
            }
        }
        else
            throw std::runtime_error{fmt::format("Usage: {} <passage> ...")};
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
