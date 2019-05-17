#include <iostream>
#include <array>

#include "acmacs-virus/virus-name.hh"

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

using parse_name_result_t = decltype(acmacs::virus::parse_name(std::string_view{}));

struct TestData
{
    std::string raw_name;
    parse_name_result_t expected;
};

void test_builtin()
{
    using namespace acmacs::virus;

    const std::array data{
        TestData{"A/SINGAPORE/INFIMH-16-0019/2016",                 parse_name_result_t{virus_name_t{"A/SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{""}, Passage{""}, ""}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/16",                   parse_name_result_t{virus_name_t{"A/SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{""}, Passage{""}, ""}},
        TestData{"A/ SINGAPORE/INFIMH-16-0019/16",                  parse_name_result_t{virus_name_t{"A/SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{""}, Passage{""}, ""}},
        TestData{"A/SINGAPORE /INFIMH-16-0019/16",                  parse_name_result_t{virus_name_t{"A/SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{""}, Passage{""}, ""}},
        TestData{"A(H3N2)/SINGAPORE/INFIMH-16-0019/2016",           parse_name_result_t{virus_name_t{"A(H3N2)/SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{""}, Passage{""}, ""}},
        // TestData{"A/H3N2/SINGAPORE/INFIMH-16-0019/2016",            parse_name_result_t{virus_name_t{"A(H3N2)/SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{""}, Passage{""}, ""}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/2016 CL2  X-307A",     parse_name_result_t{virus_name_t{"A/SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{"NYMC-307A"}, Passage{""}, "CL2"}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/2016 NEW CL2  X-307A", parse_name_result_t{virus_name_t{"A/SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{"NYMC-307A"}, Passage{""}, "CL2"}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/2016 CL2 NEW X-307A",  parse_name_result_t{virus_name_t{"A/SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{"NYMC-307A"}, Passage{""}, "CL2"}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/2016 CL2  X-307A NEW", parse_name_result_t{virus_name_t{"A/SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{"NYMC-307A"}, Passage{""}, "CL2"}},
        // TestData{"SINGAPORE/INFIMH-16-0019/2016",                   parse_name_result_t{virus_name_t{"SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{""}, Passage{""}, ""}},
        // TestData{"SINGAPORE/INFIMH-16-0019/16",                     parse_name_result_t{virus_name_t{"SINGAPORE/INFIMH-16-0019/2016"}, Reassortant{""}, Passage{""}, ""}},

        TestData{"IVR-153 (A/CALIFORNIA/07/2009)",                  parse_name_result_t{virus_name_t{"A/CALIFORNIA/7/2009"}, Reassortant{"IVR-153"}, Passage{""}, ""}},
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
            const auto result = parse_name(entry.raw_name);
            if (result != entry.expected) {
                std::cerr << "SRC: " << entry.raw_name << '\n'
                          << "NAM: " << field_mistmatch_output(std::get<virus_name_t>(result), std::get<virus_name_t>(entry.expected)) << '\n'
                          << "REA: " << field_mistmatch_output(std::get<Reassortant>(result), std::get<Reassortant>(entry.expected)) << '\n'
                          << "PAS: " << field_mistmatch_output(std::get<Passage>(result), std::get<Passage>(entry.expected)) << '\n'
                          << "EXT: " << field_mistmatch_output(std::get<std::string>(result), std::get<std::string>(entry.expected)) << '\n'
                          << '\n';
                ++errors;
            }
        }
        catch (std::exception& err) {
            std::cout << "SRC: " << entry.raw_name << '\n' << "ERR: " << err.what() << '\n';
            throw;
        }
    }

    if (errors)
        throw std::runtime_error(::string::concat("test_builtin: ", errors, " errors found"));

} // test_builtin

// ----------------------------------------------------------------------

void test_from_command_line(int argc, const char* const* argv)
{
        for (int arg = 1; arg < argc; ++arg) {
            virus_name::Name fields(argv[arg]);
            std::cout << "SRC: " << argv[arg] << '\n'
                      << "VT:  " << fields.virus_type << '\n'
                      << "HST: " << fields.host << '\n'
                      << "LOC: " << fields.location << '\n'
                      << "ISO: " << fields.isolation << '\n'
                      << "YEA: " << fields.year << '\n'
                      << "REA: " << fields.reassortant << '\n'
                      << "EXT: " << fields.extra << '\n'
                      << '\n';
        }

} // test_from_command_line

// ----------------------------------------------------------------------

// A/SINGAPORE/INFIMH-16-0019/2016 CL2  X-307A
// A/KANSAS/14/17 CDCLV24A -- reassortant
// A/KANSAS/14/2017 X-327
// A/KANSAS/14/17 CBER-22B

// BX-51B(B/MASSACHUSETTS/02/2012)
// BX-69(B/MARYLAND/15/2016)
// IVR-153 (A/CALIFORNIA/07/2009)
// IVR-155(A/VICTORIA/210/2009)
// NIB 79 (A/VICTORIA/361/2011
// NIB-103 (A/NORWAY/3806/2016)
// NIB-104(A/SINGAPORE/INFIMH-16-0019/2016)
// NYMC BX-69 (B/MARYLAND/15/2016)
// NYMC BX-69A (B/MARYLAND/15/2016)
// NYMC X-181
// NYMC X-265 (HY A/SOUTH AUSTRALIA/9/2015)
// NYMC X-265A (HY A/SOUTH AUSTRALIA/9/2015)
// RG A/CALIFORNIA/07/2009
// X-181 (A/CALIFORNIA/07/2009)
// X-203(A/MINNESOTA/11/2010)
// X-257A (A/NEW CALEDONIA/71/2014
// X-263B (A/HONG KONG/4801/2014)
// A/ABU DHABI/240/2018 CBER-25C
// A/ABU DHABI/240/2018 X-325
// A/ALASKA/232/2015 X-005
// A/ALASKA/232/2015 X-289
// A/ALMATY/2958/2013 NIB-85
// A/AMASYA/1478/2014 (22)
// A/ANTANANARIVO/1067/2016 CBER-11A B1.2
// A/ANTANANARIVO/1067/2016 CBER-11B C1.3
// A/ANTANANARIVO/1067/2016 CBER-11BC1.3
// A/AUSTRIA/1105514/2018 (1468070)
// A/BELGIUM/G0044 /2013
// A/BELGIUM/G651/2014 (VS0241)
// A/BRISBANE/1/2018 X-311
// A/BRISBANE/1/2018 X-311A
// A/BRISBANE/11/2010 X-197
// A/CHRISTCHURCH/16/2010 NIB-74XP
// A/GUANGDONG-LUOHU/1256/2009  X-185
// A/HAWAII/07/2009 RG-46V3
// A/HONG KONG/125/2017 IDCDC-RG56B PR/8
// A/HONG KONG/3101/2017 (108002)
// A/HONG KONG/4801/2014 X-263B
// A/HONG KONG/7127/2014 NIB-93
// A/ICELAND/10301/2015 (15-10301)
// A/ICELAND/32/2015 (15-02234)
// A/IDAHO/33/2016 CBER-6 B3.4
// A/IRAN/105051/2015 (9-A)
// A/KANSAS/14/2017 CBER-22B CDC19A
// A/KANSAS/14/2017 CBER-22C
// A/LISBOA/NICHLC145 17-18/2018
// A/MALI 071 CI/2015
// A/MALI 107 CI/2015
// A/MONTANA/50/2016 CBER-07 D2.3
// A/MONTANA/50/2016 CBER-7 D2.3
// A/NEW CALEDONIA/71/2014 IVR-178
// A/NEW YORK/61/2015 CDC-LV16A
// A/NORWAY/3806/2016 CBER-8 C-1.4
// A/NORWAY/3806/2016 CBER-8 C1.4
// A/PERTH/16/2009 V0152-149
// A/SINGAPORE/INFIMH-16-0019/2016 CL-2
// A/SINGAPORE/TT630 /2011
// A/SOUTH CAROLINA/02/2010 NYMC X-205A
// A/TEXAS/05/2009 PR8-IDCDC-RG15
// B/ARIZONA/10/2015 BX-63
// B/ARIZONA/10/2015 BX-63A
// B/BELGIUM/2015G0775 /2015
// B/BELGIUM/G634/2014 (VS0240)
// B/CAMEROON11V-12076 GVFI/2011
// B/COSTA RICA/4623 /2014
// B/GEORGIA/1197 (27G)/2009
// B/ICELAND/28/2015 (15-02067)
// B/IRAN/115032/2015 (4-C)
// B/TEXAS/02/2013 BX-53C

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
