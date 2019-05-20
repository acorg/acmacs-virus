#include <iostream>
#include <array>

#include "acmacs-virus/passage.hh"

static void test_from_command_line(int argc, const char* const* argv);
static void test_builtin();

// ----------------------------------------------------------------------

int main(int argc, const char* const* argv)
{
    int exit_code = 0;
    try {
        if (argc > 1)
            test_from_command_line(argc, argv);
        else
            test_builtin();
    }
    catch (std::exception& err) {
        std::cerr << "ERROR: " << err.what() << '\n';
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

using parse_passage_result_t = decltype(acmacs::virus::parse_passage(std::string_view{}));

struct TestData
{
    std::string raw_passage;
    parse_passage_result_t expected;
};

void test_builtin()
{
    using namespace acmacs::virus;

    const std::array data{
        TestData{"MDCK1",                 parse_passage_result_t{Passage{"MDCK1"}, ""}},
        TestData{"MDCK1 ",                parse_passage_result_t{Passage{"MDCK1"}, ""}},
        TestData{"MDCK 1",                parse_passage_result_t{Passage{"MDCK1"}, ""}},
        TestData{"C2",                    parse_passage_result_t{Passage{"MDCK2"}, ""}},
        TestData{"MDCKX",                 parse_passage_result_t{Passage{"MDCK?"}, ""}},
        TestData{"MDCK?",                 parse_passage_result_t{Passage{"MDCK?"}, ""}},
        TestData{"CX",                    parse_passage_result_t{Passage{"MDCK?"}, ""}},
        TestData{"SIAT1",                 parse_passage_result_t{Passage{"SIAT1"}, ""}},
        TestData{"S2",                    parse_passage_result_t{Passage{"SIAT2"}, ""}},
        TestData{"SIATX",                 parse_passage_result_t{Passage{"SIAT?"}, ""}},
        TestData{"SIAT?",                 parse_passage_result_t{Passage{"SIAT?"}, ""}},
        TestData{"SX",                    parse_passage_result_t{Passage{"SIAT?"}, ""}},
        TestData{"E3",                    parse_passage_result_t{Passage{"E3"}, ""}},
        TestData{"EX",                    parse_passage_result_t{Passage{"E?"}, ""}},
        TestData{"E?",                    parse_passage_result_t{Passage{"E?"}, ""}},
        TestData{"X",                     parse_passage_result_t{Passage{"X?"}, ""}},
        TestData{"X?",                    parse_passage_result_t{Passage{"X?"}, ""}},
        TestData{"X3",                    parse_passage_result_t{Passage{"X3"}, ""}},
        TestData{"OR",                    parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"ORIGINAL",              parse_passage_result_t{Passage{"OR"}, ""}},

        TestData{"C1/C1",                 parse_passage_result_t{Passage{"MDCK1/MDCK1"}, ""}},
        TestData{"C1,C1",                 parse_passage_result_t{Passage{"MDCK1/MDCK1"}, ""}},
        TestData{"C1/S1",                 parse_passage_result_t{Passage{"MDCK1/SIAT1"}, ""}},
        TestData{"SIAT, SIAT1",           parse_passage_result_t{Passage{"SIAT?/SIAT1"}, ""}},
    };

    const auto field_mistmatch_output = [](auto&& res, auto&& exp) {
        if (res == exp)
            return ::string::concat('"', acmacs::to_string(res), '"');
        else
            return ::string::concat("! \"", acmacs::to_string(res), "\"  vs. expected \"", acmacs::to_string(exp), '"');
    };

    size_t errors = 0;
    for (const auto& entry : data) {
        try {
            const auto result = parse_passage(entry.raw_passage);
            if (result != entry.expected) {
                std::cerr << "SRC: \"" << entry.raw_passage << "\"\n"
                          << "PAS: " << field_mistmatch_output(std::get<Passage>(result), std::get<Passage>(entry.expected)) << '\n'
                          << "EXT: " << field_mistmatch_output(std::get<std::string>(result), std::get<std::string>(entry.expected)) << '\n';
                std::cerr << '\n';
                ++errors;
            }
        }
        catch (std::exception& err) {
            std::cout << "SRC: " << entry.raw_passage << '\n' << "ERR: " << err.what() << '\n';
            throw;
        }
    }

    if (errors)
        throw std::runtime_error(::string::concat("test_builtin: ", errors, " errors found"));

} // test_builtin

// ----------------------------------------------------------------------

void test_from_command_line(int argc, const char* const* argv)
{
    using namespace acmacs::virus;

    for (int arg = 1; arg < argc; ++arg) {
        const auto result = parse_passage(argv[arg]);
        std::cerr << "SRC: \"" << argv[arg] << "\"\n"
                  << "PAS: \"" << std::get<Passage>(result) << "\"\n"
                  << "EXT: \"" << std::get<std::string>(result) << "\"\n";
    }

} // test_from_command_line

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
