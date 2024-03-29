#include <array>

#include "acmacs-base/fmt.hh"
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
        fmt::print(stderr, "ERROR: {}\n", err);
        exit_code = 1;
    }
    return exit_code;
}

// ----------------------------------------------------------------------

using parse_passage_result_t = decltype(acmacs::virus::parse_passage(std::string_view{}, acmacs::virus::passage_only::no));

struct TestData
{
    std::string raw_passage;
    parse_passage_result_t expected;
};

void test_builtin()
{
    using namespace acmacs::virus;

    const std::array data{

        TestData{"C2", parse_passage_result_t{Passage{"MDCK2"}, ""}},
        TestData{"CX", parse_passage_result_t{Passage{"MDCK?"}, ""}},
        TestData{"C", parse_passage_result_t{Passage{"MDCK?"}, ""}},
        TestData{"MDCK1", parse_passage_result_t{Passage{"MDCK1"}, ""}},
        TestData{"MDCK1 ", parse_passage_result_t{Passage{"MDCK1"}, ""}},
        TestData{"MCDK1", parse_passage_result_t{Passage{"MDCK1"}, ""}},
        TestData{"MDCK 1", parse_passage_result_t{Passage{"MDCK1"}, ""}},
        TestData{"MDCK-1", parse_passage_result_t{Passage{"MDCK1"}, ""}},
        TestData{"MDCKX", parse_passage_result_t{Passage{"MDCK?"}, ""}},
        TestData{"MDCK?", parse_passage_result_t{Passage{"MDCK?"}, ""}},
        TestData{"Passage-MDCK1", parse_passage_result_t{Passage{"MDCK1"}, ""}}, // Crick
        TestData{"SIAT1", parse_passage_result_t{Passage{"SIAT1"}, ""}},
        TestData{"SIATX", parse_passage_result_t{Passage{"SIAT?"}, ""}},
        TestData{"SIAT?", parse_passage_result_t{Passage{"SIAT?"}, ""}},
        TestData{"P1 SIAT", parse_passage_result_t{Passage{"SIAT1"}, ""}}, // MELB
        TestData{"P1SIAT", parse_passage_result_t{Passage{"SIAT1"}, ""}}, // MELB
        TestData{"P1 MDCK", parse_passage_result_t{Passage{"MDCK1"}, ""}}, // MELB
        TestData{"P1MDCK", parse_passage_result_t{Passage{"MDCK1"}, ""}}, // MELB
        TestData{"S2", parse_passage_result_t{Passage{"SIAT2"}, ""}},
        TestData{"SX", parse_passage_result_t{Passage{"SIAT?"}, ""}},
        TestData{"QMC1", parse_passage_result_t{Passage{"QMC1"}, ""}},
        TestData{"QMCX", parse_passage_result_t{Passage{"QMC?"}, ""}},
        TestData{"QMC?", parse_passage_result_t{Passage{"QMC?"}, ""}},

        TestData{"E3", parse_passage_result_t{Passage{"E3"}, ""}},
        TestData{"EX", parse_passage_result_t{Passage{"E?"}, ""}},
        TestData{"E?", parse_passage_result_t{Passage{"E?"}, ""}},
        TestData{"E", parse_passage_result_t{Passage{"E?"}, ""}},
        TestData{"EXMDCKX", parse_passage_result_t{Passage{"E?MDCK?"}, ""}},

        TestData{"X", parse_passage_result_t{Passage{"X?"}, ""}},
        TestData{"X?", parse_passage_result_t{Passage{"X?"}, ""}},
        TestData{"X3", parse_passage_result_t{Passage{"X3"}, ""}},

        TestData{"OR", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"ORIGINAL", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"ORIGINAL P0", parse_passage_result_t{Passage{"OR"}, ""}}, // gisaid
        TestData{"ORIGINAL; HOSPITALIZED", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"CS", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"CS-ORI", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"ORIGINAL SAMPLE", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"CLINICAL SPECIMEN", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"CLINICAL SAMPLE", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"DIRECT SEQUENCING", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"LABORATORY ISOLATE", parse_passage_result_t{Passage{"OR"}, ""}},

        TestData{"M?", parse_passage_result_t{Passage{"MK?"}, ""}},
        TestData{"M3", parse_passage_result_t{Passage{"MK3"}, ""}},
        TestData{"MX", parse_passage_result_t{Passage{"MK?"}, ""}},
        TestData{"MK?", parse_passage_result_t{Passage{"MK?"}, ""}},
        TestData{"MK3", parse_passage_result_t{Passage{"MK3"}, ""}},
        TestData{"MKX", parse_passage_result_t{Passage{"MK?"}, ""}},
        TestData{"MEK3", parse_passage_result_t{Passage{"MEK3"}, ""}},
        TestData{"MEKX", parse_passage_result_t{Passage{"MEK?"}, ""}},
        TestData{"MEK?", parse_passage_result_t{Passage{"MEK?"}, ""}},

        TestData{"C1/C1", parse_passage_result_t{Passage{"MDCK1/MDCK1"}, ""}},
        TestData{"C1,C1", parse_passage_result_t{Passage{"MDCK1/MDCK1"}, ""}},
        TestData{"C1/S1", parse_passage_result_t{Passage{"MDCK1/SIAT1"}, ""}},
        TestData{"SIAT, SIAT1", parse_passage_result_t{Passage{"SIAT?/SIAT1"}, ""}},
        TestData{"MDCK-SIAT, MDCK1", parse_passage_result_t{Passage{"SIAT?/MDCK1"}, ""}},
        TestData{"MDCK-2, MDCK1", parse_passage_result_t{Passage{"MDCK2/MDCK1"}, ""}},
        TestData{"MDCK-SIAT1 2 +SIAT1", parse_passage_result_t{Passage{"SIAT2/SIAT1"}, ""}}, // NIID
        TestData{"MDCKX-Siat 1", parse_passage_result_t{Passage{"SIAT1"}, ""}},              // NIID
        TestData{"X/C1", parse_passage_result_t{Passage{"X?/MDCK1"}, ""}},
        TestData{"P1/SIAT1", parse_passage_result_t{Passage{"X1/SIAT1"}, ""}}, // Crick
        TestData{"PX/SIAT1", parse_passage_result_t{Passage{"X?/SIAT1"}, ""}}, // Crick
        TestData{"MDCKX+1/MDCK1", parse_passage_result_t{Passage{"MDCK?/MDCK1/MDCK1"}, ""}},
        TestData{"MDCK1+2", parse_passage_result_t{Passage{"MDCK1/MDCK2"}, ""}},
        TestData{"AX-4 2 +SIAT1", parse_passage_result_t{Passage{"A2/SIAT1"}, ""}},
        TestData{"A2/SIAT1", parse_passage_result_t{Passage{"A2/SIAT1"}, ""}},
        TestData{"CACO-2 2 +SIAT1", parse_passage_result_t{Passage{"CACO2/SIAT1"}, ""}},
        TestData{"CACO2/SIAT1", parse_passage_result_t{Passage{"CACO2/SIAT1"}, ""}},
        TestData{"E5/E2Spf8", parse_passage_result_t{Passage{"E5/E2SPF8"}, ""}},
        TestData{"E3/D7", parse_passage_result_t{Passage{"E3/D7"}, ""}},
        TestData{"E3/D7, E1", parse_passage_result_t{Passage{"E3/D7/E1"}, ""}},
        TestData{"MDCK1-ORI", parse_passage_result_t{Passage{"MDCK1"}, ""}},
        TestData{"MDCK-MIX1/SIAT1", parse_passage_result_t{Passage{"MDCK-MIX1/SIAT1"}, ""}},
        TestData{"PASSAGE DETAILS: OR", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"PASSAGE DETAILS: ORIGINAL SPECIMEN", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"PASSAGE DETAILS: N/A", parse_passage_result_t{Passage{"OR"}, ""}},
        TestData{"CXS1", parse_passage_result_t{Passage{"MDCK?SIAT1"}, ""}},
        TestData{"CXSX", parse_passage_result_t{Passage{"MDCK?SIAT?"}, ""}},
        TestData{"X, MDCK1", parse_passage_result_t{Passage{"X?/MDCK1"}, ""}},

        TestData{"CL2", parse_passage_result_t{Passage{""}, "CL2"}},
        TestData{"EGGPLANT", parse_passage_result_t{Passage{""}, "EGGPLANT"}},
        TestData{"NEW", parse_passage_result_t{Passage{""}, "NEW"}},
        TestData{"NEE", parse_passage_result_t{Passage{""}, "NEE"}},
        TestData{"NEC", parse_passage_result_t{Passage{""}, "NEC"}},
    };

    const auto field_mistmatch_output = [](auto&& res, auto&& exp) {
        if (res == exp)
            return fmt::format("\"{}\"", res);
        else
            return fmt::format("! \"{}\"  vs. expected \"{}\"", res, exp);
    };

    size_t errors = 0;
    for (const auto& entry : data) {
        try {
            const auto result = parse_passage(entry.raw_passage, passage_only::no);
            if (result != entry.expected) {
                fmt::print(stderr, "SRC: \"{}\"\nPAS: {}\nEXT: {}\n\n", entry.raw_passage, field_mistmatch_output(std::get<Passage>(result), std::get<Passage>(entry.expected)),
                           field_mistmatch_output(std::get<std::string>(result), std::get<std::string>(entry.expected)));
                ++errors;
            }
        }
        catch (std::exception& err) {
            fmt::print(stderr, "SRC: {}\nERR: {}", entry.raw_passage, err);
            throw;
        }
    }

    if (errors)
        throw std::runtime_error(fmt::format("test_builtin: {} errors found", errors));

} // test_builtin

// ----------------------------------------------------------------------

void test_from_command_line(int argc, const char* const* argv)
{
    using namespace acmacs::virus;

    for (int arg = 1; arg < argc; ++arg) {
        const auto result = parse_passage(argv[arg], passage_only::no);
        fmt::print(stderr, "SRC: \"{}\"\nPAS: \"{}\"\nEXT: \"{}\"\n", argv[arg], *std::get<Passage>(result), std::get<std::string>(result));
    }

} // test_from_command_line

// ----------------------------------------------------------------------
