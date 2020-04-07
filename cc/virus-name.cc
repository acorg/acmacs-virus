#include "acmacs-virus/virus-name-normalize.hh"

// ----------------------------------------------------------------------

int main(int argc, const char* const* argv)
{
    int exit_code = 0;
    try {
        if (argc > 1) {
            for (int arg = 1; arg < argc; ++arg) {
                const auto [fields, messages] = acmacs::virus::name::parse(argv[arg]);
                fmt::print(stderr, " {}", messages);
                fmt::print("{}\n", fields);
            }
        }
        else
            throw std::runtime_error{fmt::format("Usage: {} <name> ...", argv[0])};
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
