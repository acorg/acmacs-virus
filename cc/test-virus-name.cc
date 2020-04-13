#include <array>
#include <tuple>

#include "acmacs-base/debug.hh"
#include "acmacs-virus/virus-name-normalize.hh"

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

// using parse_name_result_t = decltype(acmacs::virus::parse_name(std::string_view{}));

// inline bool operator!=(const parse_name_result_t& lh, const parse_name_result_t& rh)
// {
//     return ! (lh.name == rh.name && lh.host == rh.host && lh.reassortant == rh.reassortant && lh.passage == rh.passage && lh.extra == rh.extra);
// }

struct to_compare_t
{
    acmacs::virus::type_subtype_t subtype;
    acmacs::virus::host_t host;
    std::string location;
    std::string isolation;
    std::string year;
    acmacs::virus::Reassortant reassortant;
    acmacs::virus::Passage passage;
    std::string extra;
};

inline auto operator==(const acmacs::virus::name::parsed_fields_t& parsed, const to_compare_t& expected)
{
    return parsed.subtype == expected.subtype && parsed.host == expected.host && parsed.location == expected.location && parsed.isolation == expected.isolation && parsed.year == expected.year &&
           parsed.reassortant == expected.reassortant && parsed.passage == expected.passage && parsed.extra == expected.extra;
}

template <> struct fmt::formatter<to_compare_t> : public fmt::formatter<acmacs::fmt_default_formatter>
{
    template <typename FormatContext> auto format(const to_compare_t& fields, FormatContext& ctx)
    {
        return format_to(ctx.out(), "{{\"{}\" \"{}\" \"{}\" \"{}\" \"{}\" \"{}\"}}", fields.subtype, fields.host, fields.location, fields.isolation, fields.year, fields.reassortant);
    }
};

struct TestData
{
    std::string raw_name;
    to_compare_t expected;
};

void test_builtin()
{
    using type_subtype_t = acmacs::virus::type_subtype_t;
    using host_t = acmacs::virus::host_t;
    using Reassortant = acmacs::virus::Reassortant;
    using Passage = acmacs::virus::Passage;

    const type_subtype_t A{"A"};
    const type_subtype_t B{"B"};
    const host_t H;
    const Reassortant R;
    const Passage P;
    const std::string E;

    const std::array data{
        TestData{"A/duck/Guangdong/4.30 DGCPLB014-O/2017",          to_compare_t{A,                         host_t{"DUCK"}, "GUANGDONG", "4.30 DGCPLB014-O", "2017", R, P, E}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/2016",                 to_compare_t{A,                         H,              "SINGAPORE", "INFIMH-16-0019",   "2016", R, P, E}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/16",                   to_compare_t{A,                         H,              "SINGAPORE", "INFIMH-16-0019",   "2016", R, P, E}},
        TestData{"A/ SINGAPORE/INFIMH-16-0019/16",                  to_compare_t{A,                         H,              "SINGAPORE", "INFIMH-16-0019",   "2016", R, P, E}},
        TestData{"A/SINGAPORE /INFIMH-16-0019/16",                  to_compare_t{A,                         H,              "SINGAPORE", "INFIMH-16-0019",   "2016", R, P, E}},
        TestData{"A(H3N2)/SINGAPORE/INFIMH-16-0019/2016",           to_compare_t{type_subtype_t{"A(H3N2)"}, H,              "SINGAPORE", "INFIMH-16-0019",   "2016", R, P, E}},
        TestData{"AH3N2/SINGAPORE/INFIMH-16-0019/2016",             to_compare_t{type_subtype_t{"A(H3N2)"}, H,              "SINGAPORE", "INFIMH-16-0019",   "2016", R, P, E}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/2016 CL2  X-307A",     to_compare_t{A,                         H,              "SINGAPORE", "INFIMH-16-0019",   "2016", Reassortant{"NYMC-307A"}, P, "CL2"}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/2016 NEW CL2  X-307A", to_compare_t{A,                         H,              "SINGAPORE", "INFIMH-16-0019",   "2016", Reassortant{"NYMC-307A"}, P, "CL2"}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/2016 CL2 NEW X-307A",  to_compare_t{A,                         H,              "SINGAPORE", "INFIMH-16-0019",   "2016", Reassortant{"NYMC-307A"}, P, "CL2"}},
        TestData{"A/SINGAPORE/INFIMH-16-0019/2016 CL2  X-307A NEW", to_compare_t{A,                         H,              "SINGAPORE", "INFIMH-16-0019",   "2016", Reassortant{"NYMC-307A"}, P, "CL2"}},
        TestData{"AH3N2/SINGAPORE/INFIMH-16-0019/2016 MDCK1",       to_compare_t{type_subtype_t{"A(H3N2)"}, H,              "SINGAPORE", "INFIMH-16-0019",   "2016", R, Passage{"MDCK1"}, E}},
        TestData{"A/Snowy Sheathbill/Antarctica/2899/2014",         to_compare_t{A,                         host_t{"SNOWY SHEATHBILL"}, "ANTARCTICA", "2899", "2014", R, P, E}},
        TestData{"A/wigeon/Italy/6127-23/2007",                     to_compare_t{A,                         host_t{"WIGEON"}, "ITALY", "6127-23", "2007", R, P, E}},
        TestData{"B/Via?A Del Mar/73490/2017",                      to_compare_t{B,                         H,                "VINA DEL MAR", "73490", "2017", R, P, E}},
        TestData{"B/Cameroon11V-12080 GVFI/2011",                   to_compare_t{B,                         H,                "CAMEROON", "11V-12080 GVFI", "2011", R, P, E}},
        TestData{"A/Mali 071 Ci/2015",                              to_compare_t{A,                         H,                "MALI", "71 CI", "2015", R, P, E}},
        TestData{"A/Zambia/13/174/2013",                            to_compare_t{A,                         H,                "ZAMBIA", "13-174", "2013", R, P, E}},
        TestData{"A/Lyon/CHU18.54.48/2018",                         to_compare_t{A,                         H,                "LYON CHU", "18.54.48", "2018", R, P, E}},
        TestData{"A/Lyon/CHU/R18.54.48/2018",                       to_compare_t{A,                         H,                "LYON CHU", "R18.54.48", "2018", R, P, E}},
        TestData{"A/Algeria/G0281/16/2016",                         to_compare_t{A,                         H,                "ALGERIA", "G0281-16", "2016", R, P, E}},
        TestData{"A/chicken/Ghana/7/2015",                          to_compare_t{A,                         host_t{"CHICKEN"}, "GHANA", "7", "2015", R, P, E}},
        TestData{"IVR-153 (A/CALIFORNIA/07/2009)",                  to_compare_t{A,                         H,                "CALIFORNIA", "7", "2009", Reassortant{"IVR-153"}, P, E}},
        TestData{"A/Brisbane/01/2018  NYMC-X-311 (18/160)",         to_compare_t{A,                         H,                "BRISBANE", "1", "2018", Reassortant{"NYMC-311"}, P, E}}, // "(18/160)" removed by check_nibsc_extra
        TestData{"A/Antananarivo/1067/2016 CBER-11B C1.3",          to_compare_t{A,                         H,                "ANTANANARIVO", "1067", "2016", Reassortant{"CBER-11B"}, P, "C1.3"}}, // CDC
        TestData{"A/Montana/50/2016 CBER-07 D2.3",                  to_compare_t{A,                         H,                "MONTANA", "50", "2016", Reassortant{"CBER-07"}, P, "D2.3"}}, // CDC
        TestData{"A/duck/Guangdong/02.11 DGQTXC195-P/2015(Mixed)",  to_compare_t{A,                         host_t{"DUCK"},   "GUANGDONG", "2.11 DGQTXC195-P", "2015", R, P, E}}, // (MIXED) removed
        TestData{"A/duck/Guangdong/02.11 DGQTXC195-P/2015(H5N1)",   to_compare_t{type_subtype_t{"A(H5N1)"}, host_t{"DUCK"},   "GUANGDONG", "2.11 DGQTXC195-P", "2015", R, P, E}},
        TestData{"A/swine/Chachoengsao/2003",                       to_compare_t{A,                         host_t{"SWINE"},   "CHACHOENGSAO", "UNKNOWN", "2003", R, P, E}},

        // nbci -- genbank
        TestData{"A/Anas platyrhynchos/Belgium/17330 2/2013",       to_compare_t{A, host_t{"ANAS PLATYRHYNCHOS"}, "BELGIUM", "17330 2", "2013", R, P, E}},
        // TestData{"A/mallard/Balkhash/6304_HA/2014",                 to_compare_t{A, host_t{"MALLARD"}, "BALKHASH", "6304", "2014"}, R, P, E}},
        TestData{"A/mallard/Balkhash/6304_HA/2014",                 to_compare_t{A, host_t{"MALLARD"}, "BALKHASH", "6304", "2014", R, P, E}}, // _HA is seqgment reference in ncbi
        // TestData{"A/SWINE/NE/55024/2018",                           to_compare_t{A, host_t{"SWINE"},   "NE", "55024", "2018", R, P, E}},
        TestData{"A/chicken/Iran221/2001",                          to_compare_t{A, host_t{"CHICKEN"}, "IRAN", "221", "2001", R, P, E}},
        TestData{"A/BiliranTB5/0423/2015",                          to_compare_t{A, H,                 "BILIRAN",  "TB5-0423", "2015", R, P, E}},
        TestData{"A/chicken/Yunnan/Kunming/2007",                   to_compare_t{A, host_t{"CHICKEN"}, "YUNNAN KUNMING", "UNKNOWN", "2007", R, P, E}},

        //----
        //TestData{"",          to_compare_t{name_t{"", R, Passage{}, ""}},

        // TestData{"A/H3N2/SINGAPORE/INFIMH-16-0019/2016",            to_compare_t{A, "(H3N2)/SINGAPORE/INFIMH-16-0019/2016", R, Passage{}, ""}},
        // TestData{"SINGAPORE/INFIMH-16-0019/2016",                   to_compare_t{name_t{"SINGAPORE/INFIMH-16-0019/2016", R, Passage{}, ""}},
        // TestData{"SINGAPORE/INFIMH-16-0019/16",                     to_compare_t{name_t{"SINGAPORE/INFIMH-16-0019/2016", R, Passage{}, ""}},

        // TestData{"A-Rostov-CRIE-1-2018",                            to_compare_t{A, "ROSTOV/CRIE-1/2018", R, Passage{}, ""}}, // CRIE (Russia)
    };

    size_t errors = 0;
    for (const auto& entry : data) {
        try {
            const auto result = acmacs::virus::name::parse(entry.raw_name);
            if (result != entry.expected) {
                AD_ERROR("{} <-- \"{}\"  expected: \"{}\"", result, entry.raw_name, entry.expected);
                ++errors;
            }
            else if (!result.messages.empty()) {
                AD_INFO("{}", result);
            }
        }
        catch (std::exception& err) {
            AD_ERROR("SRC: {}: {}", entry.raw_name, err);
            throw;
        }
    }

    if (errors)
        throw std::runtime_error(acmacs::string::concat("test_builtin: ", errors, " errors found"));

} // test_builtin

// ----------------------------------------------------------------------

void test_from_command_line(int argc, const char* const* argv)
{
    for (int arg = 1; arg < argc; ++arg) {
        try {
            const auto result = acmacs::virus::name::parse(argv[arg]);
            fmt::print("\"{}\" --> {}\n", argv[arg], result);
        }
        catch (std::exception& err) {
            AD_ERROR("{}", err);
        }
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
