#include <regex>
#include <array>
#include <map>
#include <functional>
#include <cctype>

#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-strip.hh"
#include "acmacs-base/date.hh"
#include "acmacs-virus/passage.hh"
#include "acmacs-virus/log.hh"

// ----------------------------------------------------------------------

constexpr const char* re_egg = R"#(((E|D|SPF(CE)?|SPE)(\?|[0-9][0-9]?)|EGG))#";
constexpr const char* re_cell = R"#((MDCK|SIAT|QMC|HCK|MK|MEK|CKC|CEK|CACO|LLC|LLK|PRMK|MEK|C|SPFCK|R|RII)(\?|[0-9][0-9]?))#";
constexpr const char* re_crick_am_al = R"#((\(AM\d/?AL\d\)(C\d+(-\d+)?)?)?)#"; // Crick E8(Am3Al5)c11-10 in H3 PRN
constexpr const char* re_crick_isolate = R"#(( (ISOLATE|CLONE) [0-9\-]+)*)#"; // CRICK isolate and/or clone, CRICK H1pdm has CLONE 38-32
constexpr const char* re_niid_plus_number = R"#(( *\+[1-9])?)#"; // NIID has +1 at the end of passage
constexpr const char* re_passage_date = R"#(( \([12][0129][0-9][0-9]-[01][0-9]-[0-3][0-9]\))?)#"; // passage date

// ----------------------------------------------------------------------

bool acmacs::virus::Passage::is_egg() const
{
#include "acmacs-base/global-constructors-push.hh"
        static std::regex egg_passage{std::string(re_egg) + re_crick_am_al + re_crick_isolate +  re_niid_plus_number + re_passage_date}; // CRICK has "EGG 10-6" in h3-neut
#include "acmacs-base/diagnostics-pop.hh"
        return std::regex_search(get(), egg_passage);

} // acmacs::virus::Passage::is_egg

// ----------------------------------------------------------------------

bool acmacs::virus::Passage::is_cell() const
{
#include "acmacs-base/global-constructors-push.hh"
        static std::regex cell_passage{std::string(re_cell) + re_crick_isolate +  re_niid_plus_number + re_passage_date};
#include "acmacs-base/diagnostics-pop.hh"
        return std::regex_search(get(), cell_passage);

} // acmacs::virus::Passage::is_cell

// ----------------------------------------------------------------------

std::string acmacs::virus::Passage::without_date() const
{
    if (size() > 13 && get()[size() - 1] == ')' && get()[size() - 12] == '(' && get()[size() - 13] == ' ' && get()[size() - 4] == '-' &&
        get()[size() - 7] == '-')
        return std::string(get(), 0, size() - 13);
    else
        return get();

} // acmacs::virus::Passage::without_date

// ----------------------------------------------------------------------

using source_iter_t = decltype(std::string_view{}.cbegin());

struct processing_data_t
{
    std::vector<std::string> parts;
    std::string last_passage_type;
    std::string extra;
};

struct parsing_failed : public std::exception { using std::exception::exception; };

using callback_t = source_iter_t (*)(processing_data_t& data, source_iter_t first, source_iter_t last); // returns new first value, throws parsing_failed

static inline source_iter_t push_lab_separator(processing_data_t& data, char orig_symbol, source_iter_t result={})
{
    if (data.parts.empty())
        data.extra.append(1, orig_symbol);
    else
        data.parts.push_back("/");
    return result;
}

static inline source_iter_t parts_push_i(processing_data_t& data, const char* p1, const std::string& p2={}, source_iter_t result={})
{
    if (data.last_passage_type == p1 && !data.parts.empty() && !data.parts.back().empty() && data.parts.back().back() != '/')
        data.parts.push_back("/");
    data.parts.push_back(p1);
    data.last_passage_type = p1;
    if (!p2.empty())
        data.parts.push_back(p2);
    return result;
}

static inline void add_to_extra(processing_data_t& data, char orig_symbol, std::string_view p2)
{
    data.extra.append(1, ' ');
    data.extra.append(1, orig_symbol);
    data.extra.append(p2);
}

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"

// MDCK C
static const std::regex re_c_c_x("^(?:[X\\?]|ELL[\\s\\-]*(?:PASSAGED?)?)", acmacs::regex::icase);
static const std::regex re_c_c_n("^[\\s\\-]*(\\d+)(?![\\.])", acmacs::regex::icase); // no . afterwards to support C1.3 annotation (CDC)
static const std::regex re_c_c_n_mdck("^(\\d+)\\s*\\(MDCK\\)", acmacs::regex::icase); // gisaid
static const std::regex re_c_canis_mdck("^ANIS\\s+LUPUS\\s+FAMILIARIS\\s+MDCK\\s+CELLS", acmacs::regex::icase); // no . afterwards to support C1.3 annotation (CDC)
static const std::regex re_m_mdck_x("^(?:DCK|CDK|DKC)[\\s\\-]*(?:[X\\?`]|PASSAGED?|CELLS)?", acmacs::regex::icase);
static const std::regex re_m_mdck_n("^(?:M*DCK|CDK|DKC)[\\s\\-#/]*(\\d+)", acmacs::regex::icase); // MMDCK, MMMDCK, MDCK#2 - in gisaid
static const std::regex re_m_mdck_siat_x("^DCKX?-SIAT[\\s\\-]*[X\\?]?", acmacs::regex::icase);
static const std::regex re_m_mdck_siat_n("^DCKX?-SIAT[\\s\\-]*(\\d+)", acmacs::regex::icase);
static const std::regex re_m_mdck_siat1_n("^DCK-SIAT1[\\s\\-](\\d+)", acmacs::regex::icase);
static const std::regex re_m_mdck_mix_n("^DCK-MIX(\\d+)", acmacs::regex::icase);
static const std::regex re_2_2nd_pass_mdck("^ND\\s+PASS\\s+MDCK", acmacs::regex::icase);
// P1 MDCK (MELB)
static const std::regex re_p_mdck("^(\\d+)\\s*MDCK(?!\\d)", acmacs::regex::icase);

// SIAT S
static const std::regex re_s_s_x("^[X\\?]", acmacs::regex::icase);
static const std::regex re_s_s_n("^(\\d+)", acmacs::regex::icase); // CDC H1
static const std::regex re_s_siat_x("^IAT?[\\s\\-]*[X\\?]?", acmacs::regex::icase);
static const std::regex re_s_siat_n("^(?:IAT)?[\\s\\-]*(\\d+)", acmacs::regex::icase);
static const std::regex re_s_siat1_passage_n("^IAT1/\\s*PASSAGE?(\\d+)", acmacs::regex::icase);
// P1 SIAT (MELB)
static const std::regex re_p_siat("^(\\d+)\\s*SIAT(?!\\d)", acmacs::regex::icase);

// QMC Seqirus (Novartis) qualified MDCK cells. Previously the cell line called "NC"
static const std::regex re_q_qmc_x("^MC[\\s\\-]*[X\\?]?", acmacs::regex::icase);
static const std::regex re_q_qmc_n("^MC[\\s\\-]*(\\d+)", acmacs::regex::icase);
static const std::regex re_n_nc_n("^C[\\s\\-]*(\\d+)", acmacs::regex::icase);

// E EGG
static const std::regex re_e_e_x("^[\\s\\-]*[X\\?]", acmacs::regex::icase); // may followed by letters, e.g. EXMDCKX (MELB)
static const std::regex re_e_egg_x("^GG[\\s\\-]*(?:PASSAGED?|GROWN)?[X\\?]?(?!\\w)", acmacs::regex::icase);
static const std::regex re_e_am_al(R"#(^(\d+)\s*\((AM\d)/?(AL\d)\)(C\d+(?:-\d+)?)?)#", acmacs::regex::icase); // Crick PRN 2018 tables: "E8(Am3Al5)c11-10" "E6 (Am3/Al3)"
static const std::regex re_e_egg_n("^(?:GG(?:[\\s\\-]+PASSAGED?)?)?[\\s\\-]*(\\d+)(?!\\d*-\\d+)", acmacs::regex::icase); // does not match EGG 10-4 where 10-4 is concentration
static const std::regex re_s_spfe_n("^PFC?E[\\s\\-]*(\\d+)", acmacs::regex::icase);
static const std::regex re_s_spfe_x("^PFC?E[X\\?]", acmacs::regex::icase);
static const std::regex re_s_spfe("^PFC?E$", acmacs::regex::icase);

// HCK - humanized MDCK cell line for the efficient isolation and propagation of human influenza viruses https://www.researchgate.net/publication/332744615_A_humanized_MDCK_cell_line_for_the_efficient_isolation_and_propagation_of_human_influenza_viruses
static const std::regex re_h_hck_n("^CK?[\\s\\-]*(\\d+)", acmacs::regex::icase);
static const std::regex re_h_hck_x("^CK?[\\s\\-]*[X\\?]?", acmacs::regex::icase);

// LOT - not a passage
static const std::regex re_l_lot(R"(^(OT)\s*([A-Z]+\d+))", acmacs::regex::icase); // CDC H1pdm -> not a passage

// MK M - Monkey Kidney Cell line
static const std::regex re_m_mk_x("^K?[\\s\\-]*[X\\?]", acmacs::regex::icase);
static const std::regex re_m_mk_n("^K?[\\s\\-]*(\\d+)", acmacs::regex::icase);
static const std::regex re_p_pmk_n("^MK?[\\s\\-]*(\\d+);?", acmacs::regex::icase); // gisaid
static const std::regex re_p_prmk_n("^RMK?[\\s\\-]*(\\d+)", acmacs::regex::icase); // Primary Rhesus Monkey Kidney Cell line

// MEK - Monkey Epithelial Kidney Cell line
static const std::regex re_m_mek_x("^EK?[\\s\\-]*[X\\?]", acmacs::regex::icase);
static const std::regex re_m_mek_n("^EK?[\\s\\-]*(\\d+)", acmacs::regex::icase);

// OR CS CLINICAL ORIGINAL SPECIMEN/SAMPLE
static const std::regex re_c_clinical("^(?:S(?:-ORI|\\(ORIGINAL\\))?|LINI?CAL[\\sA-Z]*(?:\\((?:TRACHEA|NASAL)[\\sA-Z]+\\))?)", acmacs::regex::icase);
static const std::regex re_o_original("^(?:R|O?[RT]IGINAL)[;\\s\\-_\\(\\)A-Z0]*", acmacs::regex::icase);
static const std::regex re_o_opnp("^P&NP\\s*$", acmacs::regex::icase); // CDC:Congo/2015
static const std::regex re_l_lung("^(?:UNG|AB)[\\s\\-\\w]*", acmacs::regex::icase);             // CRICK
static const std::regex re_n_nose("^(?:OSE|ASO|ASA)[\\s\\-_A-Z]*", acmacs::regex::icase); // CRICK
static const std::regex re_t_throat("^HROAT SWAB", acmacs::regex::icase);                 // CRICK
static const std::regex re_s_swab("^(?:WAB|PECIMEN)", acmacs::regex::icase);
static const std::regex re_p_pm_lung("^M LUNG", acmacs::regex::icase); // CRICK
static const std::regex re_b_or("^RONCH[\\s\\-_\\(\\)A-Z]*", acmacs::regex::icase);
static const std::regex re_paren_from("^FROM[\\sA-Z]+\\)", acmacs::regex::icase);
static const std::regex re_d_direct("^IRECT[\\sA-Z\\-]*$", acmacs::regex::icase); // Public Health Agency of Sweden
static const std::regex re_n_not_passaged("^OT? PASSAGED?\\s*$", acmacs::regex::icase); // University of Michigan
static const std::regex re_a_autopsy("^UTOPSY[\\s\\-_\\(\\)A-Z]*$", acmacs::regex::icase);
static const std::regex re_n_na("^(?:/A|A|A\\s+EXTRACT|ONE)\\s*$", acmacs::regex::icase);
static const std::regex re_n_no_pass("^O\\s+PASS(?:AGE?)?$", acmacs::regex::icase);
static const std::regex re_i_initial("^NITIAL\\s*$", acmacs::regex::icase);

// R R-MIX - R-mix tissure culture
static const std::regex re_r_n("^(?:-?M[I1]?X)?[\\s\\-]*(\\d+)", acmacs::regex::icase);
static const std::regex re_r_x("^(?:-?M[I1]?X)?[\\s\\-]*[X\\?]?(?!\\w)", acmacs::regex::icase);
static const std::regex re_rii_n("^II[\\s\\-]*(\\d+)", acmacs::regex::icase);
static const std::regex re_rii_x("^II[X\\?]?(?!\\w)", acmacs::regex::icase);

// AX4-PB2 cell line by Vetmed (Eileen A. Maher), used by NIID H3 FRA in 2018 as "AX-4 2"
static const std::regex re_a_ax4_n("^(?:X-?4\\s+)?(\\d+)", acmacs::regex::icase);

//  Human Caucasian Colon Adenocarcinoma Cell line: CACO2 2 or CACO2
static const std::regex re_c_caco_n("^ACO(?:-2\\s+)?(\\d+)", acmacs::regex::icase);

// Specific Pathogen Free Egg, CDC H3 2018
static const std::regex re_s_spf_n("^PF(\\d+)", acmacs::regex::icase);

// D (egg?)
static const std::regex re_d_d_n("^(\\d+)(?![\\.])", acmacs::regex::icase); // no . afterwards to support D1.3 annotation (CDC)

// Specific Pathogen Free Chicken Kidney Cell line
static const std::regex re_s_spfck_n("^PFCK(\\d+)", acmacs::regex::icase);

// X
static const std::regex re_x_n("^(\\d+)", acmacs::regex::icase);
static const std::regex re_p_n("^-?\\s*(\\d+)", acmacs::regex::icase);

// 0
static const std::regex re_0_original("^(?:R|O?[RT]IGINAL)[;\\s\\-_\\(\\)A-Z0]*", acmacs::regex::icase);

// (AM1AL3) - NIID and Crick Egg passage suffix
static const std::regex re_parent_amal(R"((AM\d+)[/,]?(AL\d+)\))", acmacs::regex::icase);

// ignore/remove
static const std::regex re_c_ignore("^LONE-[A-Z\\d]+", acmacs::regex::icase); // clone-C12 in gisaid from Netehralnds
static const std::regex re_p_ignore("^ASSAGE[:\\-\\s]?(?:DETAILS:)?", acmacs::regex::icase);
static const std::regex re_dash_ori("^\\s*ORI\\s*$", acmacs::regex::icase);

static const std::regex re_digits("^(\\d+)", acmacs::regex::icase);
static const std::regex re_paren_date(R"(^(\d{4}-\d\d-\d\d|\d{1,2}/\d{1,2}/\d{2,4})\)(?:\s*[A-Z]{2}\b)?)", acmacs::regex::icase); // CDC passage sometimes has location abbreviation after date

#include "acmacs-base/diagnostics-pop.hh"

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"

static const std::map<char, callback_t> normalize_data{
    {'A',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         if (std::cmatch match; std::regex_search(first, last, match, re_a_ax4_n))
             return parts_push_i(data, "A", match[1].str(), match[0].second);
         else if (std::regex_search(first, last, match, re_a_autopsy))
             return parts_push_i(data, "OR", {}, match[0].second);
         else
             throw parsing_failed{};
     }},
    {'B',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         if (std::cmatch match; std::regex_search(first, last, match, re_b_or))
             return parts_push_i(data, "OR", {}, match[0].second);
         else
             throw parsing_failed{};
     }},
    {'C',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (first == last)     // just C
             return parts_push_i(data, "MDCK", "?", first);
         if (std::regex_search(first, last, match, re_c_ignore))
             add_to_extra(data, 'C', match.str(0)); // CLONE-xxx is extra
         else if (std::regex_search(first, last, match, re_c_c_n_mdck) || std::regex_search(first, last, match, re_c_c_n))
             parts_push_i(data, "MDCK", match[1].str());
         else if (std::regex_search(first, last, match, re_c_clinical))
             parts_push_i(data, "OR");
         else if (std::regex_search(first, last, match, re_c_caco_n))
             parts_push_i(data, "CACO", match[1].str());
         else if (std::regex_search(first, last, match, re_c_c_x) || std::regex_search(first, last, match, re_c_canis_mdck))
             parts_push_i(data, "MDCK", "?");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'D',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         if (std::cmatch match; std::regex_search(first, last, match, re_d_d_n))
             return parts_push_i(data, "D", match[1].str(), match[0].second);
         else if (std::regex_search(first, last, match, re_d_direct))
             return parts_push_i(data, "OR", {}, match[0].second);
         else
             throw parsing_failed{};
     }},
    {'E',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (first == last)     // just E
             return parts_push_i(data, "E", "?", first);
         if (std::regex_search(first, last, match, re_e_am_al))
             parts_push_i(data, "E", match.format("$1($2$3)$4"));
         else if (std::regex_search(first, last, match, re_e_egg_n))
             parts_push_i(data, "E", match[1].str());
         else if (std::regex_search(first, last, match, re_e_egg_x) || std::regex_search(first, last, match, re_e_e_x))
             parts_push_i(data, "E", "?");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'H',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_h_hck_n))
             parts_push_i(data, "HCK", match[1].str());
         else if (std::regex_search(first, last, match, re_h_hck_x))
             parts_push_i(data, "HCK", "?");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'I',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         if (std::cmatch match; std::regex_search(first, last, match, re_i_initial))
             return parts_push_i(data, "OR", {}, match[0].second);
         else
             throw parsing_failed{};
     }},
    {'L',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_l_lung))
             parts_push_i(data, "OR", {});
         else if (std::regex_search(first, last, match, re_l_lot))
             add_to_extra(data, 'L', match.format("$1 $2")); // "LOT A1" not a passage
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'M',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_m_mdck_n))
             parts_push_i(data, "MDCK", match[1].str());
         else if (std::regex_search(first, last, match, re_m_mdck_siat1_n))
             parts_push_i(data, "SIAT", match[1].str());
         else if (std::regex_search(first, last, match, re_m_mdck_siat_n))
             parts_push_i(data, "SIAT", match[1].str());
         else if (std::regex_search(first, last, match, re_m_mdck_siat_x))
             parts_push_i(data, "SIAT", "?");
         else if (std::regex_search(first, last, match, re_m_mdck_mix_n))
             parts_push_i(data, "MDCK-MIX", match[1].str());
         else if (std::regex_search(first, last, match, re_m_mk_n))
             parts_push_i(data, "MK", match[1].str());
         else if (std::regex_search(first, last, match, re_m_mek_n))
             parts_push_i(data, "MEK", match[1].str());
         else if (std::regex_search(first, last, match, re_m_mdck_x))
             parts_push_i(data, "MDCK", "?");
         else if (std::regex_search(first, last, match, re_m_mk_x))
             parts_push_i(data, "MK", "?");
         else if (std::regex_search(first, last, match, re_m_mek_x))
             parts_push_i(data, "MEK", "?");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'N',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_n_nose) || std::regex_search(first, last, match, re_n_not_passaged) || std::regex_search(first, last, match, re_n_na) || std::regex_search(first, last, match, re_n_no_pass))
             parts_push_i(data, "OR");
         else if (std::regex_search(first, last, match, re_n_nc_n))
             parts_push_i(data, "QMC", match[1].str());
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'O',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_o_original) || std::regex_search(first, last, match, re_o_opnp))
             parts_push_i(data, "OR");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'P',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_p_mdck))
             return parts_push_i(data, "MDCK", match[1].str(), match[0].second);
         else if (std::regex_search(first, last, match, re_p_siat))
             return parts_push_i(data, "SIAT", match[1].str(), match[0].second);
         else if (std::regex_search(first, last, match, re_p_n))
             return parts_push_i(data, "X", match[1].str(), match[0].second);
         else if (std::regex_search(first, last, match, re_p_ignore))
             return match[0].second; // ignore PASSAGE-
         else if (std::regex_search(first, last, match, re_p_pmk_n))
             return parts_push_i(data, "PMK", {}, match[0].second);
         else if (std::regex_search(first, last, match, re_p_prmk_n))
             return parts_push_i(data, "PRMK", {}, match[0].second);
         else if (std::regex_search(first, last, match, re_p_pm_lung))
             return parts_push_i(data, "OR", {}, match[0].second);
         else if (first != last && (*first == 'X' || *first == 'x'))
             return parts_push_i(data, "X", "?", first + 1);
         else
             throw parsing_failed{};
     }},
    {'Q',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_q_qmc_n))
             parts_push_i(data, "QMC", match[1].str());
         else if (std::regex_search(first, last, match, re_q_qmc_x))
             parts_push_i(data, "QMC", "?");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'R',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_r_n))
             parts_push_i(data, "R", match[1].str());
         else if (std::regex_search(first, last, match, re_rii_n))
             parts_push_i(data, "RII", match[1].str());
         else if (std::regex_search(first, last, match, re_rii_x))
             parts_push_i(data, "RII", "?");
         else if (std::regex_search(first, last, match, re_r_x))
             parts_push_i(data, "R", "?");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'S',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_s_siat1_passage_n) || std::regex_search(first, last, match, re_s_siat_n) || std::regex_search(first, last, match, re_s_s_n))
             parts_push_i(data, "SIAT", match[1].str());
         else if (std::regex_search(first, last, match, re_s_spfe_n))
             parts_push_i(data, "SPFE", match[1].str());
         else if (std::regex_search(first, last, match, re_s_spfe_x) || std::regex_search(first, last, match, re_s_spfe))
             parts_push_i(data, "SPFE", "?");
         else if (std::regex_search(first, last, match, re_s_spf_n))
             parts_push_i(data, "SPF", match[1].str());
         else if (std::regex_search(first, last, match, re_s_spfck_n))
             parts_push_i(data, "SPFCK", match[1].str());
         else if (std::regex_search(first, last, match, re_s_swab))
             parts_push_i(data, "OR");
         else if (std::regex_search(first, last, match, re_s_s_x) || std::regex_search(first, last, match, re_s_siat_x))
             parts_push_i(data, "SIAT", "?");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'T',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_t_throat))
             parts_push_i(data, "OR");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'X',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         // signle X is a passage only if there is no extra before it
         if (first == last) {
             if (data.extra.empty())
                 return parts_push_i(data, "X", "?", first);
             else
                 throw parsing_failed{};
         }
         else if (*first == '?' || *first == 'X')
             return parts_push_i(data, "X", "?", first + 1);
         else if (std::cmatch match; std::regex_search(first, last, match, re_x_n))
             return parts_push_i(data, "X", match[1].str(), match[0].second);
         else if (*first == '/' || *first == ',')
             return parts_push_i(data, "X", "?", first);
         else
             throw parsing_failed{};
     }},
    {'0',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_0_original))
             parts_push_i(data, "OR");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {'2',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if (std::regex_search(first, last, match, re_2_2nd_pass_mdck))
             parts_push_i(data, "MDCK", "2");
         else
             throw parsing_failed{};
         return match[0].second;
     }},
    {' ', [](processing_data_t&, source_iter_t first, source_iter_t /*last*/) -> source_iter_t { return first; }},
    {'/', [](processing_data_t& data, source_iter_t first, source_iter_t /*last*/) -> source_iter_t { return push_lab_separator(data, '/', first); }},
    {'\\',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         parts_push_i(data, "/");
         while (first != last && *first == '\\')
             ++first;
         return first;
     }},
    {',', [](processing_data_t& data, source_iter_t first, source_iter_t /*last*/) -> source_iter_t { return push_lab_separator(data, ',', first); }},
    {'.', [](processing_data_t& data, source_iter_t first, source_iter_t /*last*/) -> source_iter_t { return push_lab_separator(data, '.', first); }},
    {'-',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         if (std::cmatch match; !data.parts.empty() && std::regex_search(first, last, match, re_dash_ori))
             return match[0].second; // ignore
         else
             throw parsing_failed{};
     }},
    {'+',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         if (std::cmatch match; !data.last_passage_type.empty() && data.last_passage_type != "/" && std::regex_search(first, last, match, re_digits)) {
             push_lab_separator(data, '+');
             return parts_push_i(data, data.last_passage_type.data(), match[1].str(), match[0].second);
         }
         else
             return push_lab_separator(data, '+', first);
     }},
    {'(',
     [](processing_data_t& data, source_iter_t first, source_iter_t last) -> source_iter_t {
         std::cmatch match;
         if ( data.last_passage_type == "E" && std::regex_search(first, last, match, re_parent_amal)) {
             data.parts.push_back(match.format("($1$2)"));
         }
         else if (!data.parts.empty() && std::regex_search(first, last, match, re_paren_from)) {
             // empty, ignore
         }
         else if (!data.parts.empty() && std::regex_search(first, last, match, re_paren_date)) {
             data.parts.push_back(fmt::format(" ({})", date::from_string(match[1].str(), date::allow_incomplete::no, date::throw_on_error::yes, date::month_first::yes))); // passage date is CDC property -> month-first
             data.last_passage_type.clear();
         }
         else
             throw parsing_failed{};
         return match[0].second;
     }},
};

#include "acmacs-base/diagnostics-pop.hh"

// ----------------------------------------------------------------------

acmacs::virus::parse_passage_t acmacs::virus::parse_passage(std::string_view source, passage_only po)
{
    processing_data_t data;

    AD_LOG(acmacs::log::passage_parsing, "src: \"{}\"", source);
    AD_LOG_INDENT;

    for (auto first = source.begin(); first != source.end();) {
        if (*first != ' ') {
            bool skip = false;
            if (const auto entry = normalize_data.find(static_cast<char>(std::toupper(*first))); entry != normalize_data.end()) {
                try {
                    first = entry->second(data, first + 1, source.end());
                }
                catch (parsing_failed&) {
                    skip = true;
                }
            }
            else {
                skip = true;
            }
            if (skip) {
                if (po == passage_only::yes)
                    return parse_passage_t{{}, std::string{source}}; // parsing failed;

                if (data.parts.empty()) { // passage not yet started
                    if (std::isalnum(*first)) {
                            // put word into extra
                        const auto end = std::find_if(first + 1, source.end(), [](char chr) { return !std::isalnum(chr); });
                        data.extra.append(first, end);
                        first = end;
                    }
                    else {
                        data.extra.append(1, *first);
                        ++first;
                    }
                }
                else { // some parts of passage found
                    if (!data.extra.empty())
                        data.extra.append(1, ' ');
                    data.extra.append(first, source.end());
                    break;
                }
            }
        }
        else {
            if (!data.extra.empty())
                data.extra.append(1, ' ');
            ++first;
        }
        AD_LOG(acmacs::log::passage_parsing, "src:\"{}\" passage:{} last_passage_type:{} extra:\"{}\"", std::string_view(&*first, static_cast<size_t>(source.end() - first)), data.parts, data.last_passage_type, data.extra);
    }

    auto extra = ::string::upper(acmacs::string::strip(data.extra));

    using namespace acmacs::regex;
#include "acmacs-base/global-constructors-push.hh"
    static const std::array remove_redundant_extra{
        look_replace_t{std::regex("\\b(?:AND ORIGINAL ISOLATES|(?:chicken|quail|mouse\\s+)?ADAPTED)\\b", acmacs::regex::icase), {"$` $'"}},
    };
#include "acmacs-base/diagnostics-pop.hh"
    if (const auto extra_fixed = scan_replace(extra, remove_redundant_extra); extra_fixed.has_value())
        extra = ::string::collapse_spaces(acmacs::string::strip(extra_fixed->back()));

    Passage result{::string::upper(string::join(acmacs::string::join_concat, data.parts))};
    AD_LOG(acmacs::log::passage_parsing, "resulting passage:\"{}\"  extra:\"{}\"", result, extra);
    return {std::move(result), extra};

} // acmacs::virus::parse_passage

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
