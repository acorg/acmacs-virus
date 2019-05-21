#include <regex>
#include <array>
#include <map>
#include <functional>
#include <cctype>

#include "acmacs-base/string-split.hh"
#include "acmacs-virus/passage.hh"

// ----------------------------------------------------------------------

constexpr const char* re_egg = R"#(((E|SPF(CE)?|SPE)(\?|[0-9][0-9]?)|EGG))#";
constexpr const char* re_cell = R"#((MDCK|SIAT|QMC|MK|CKC|CEK|CACO|LLC|LLK|PRMK|MEK|C|SPFCK)(\?|[0-9][0-9]?))#";
constexpr const char* re_nimr_isolate = R"#(( (ISOLATE|CLONE) [0-9\-]+)*)#"; // NIMR isolate and/or clone, NIMR H1pdm has CLONE 38-32
constexpr const char* re_niid_plus_number = R"#(( *\+[1-9])?)#"; // NIID has +1 at the end of passage
constexpr const char* re_passage_date = R"#(( \([12][0129][0-9][0-9]-[01][0-9]-[0-3][0-9]\))?)#"; // passage date

// ----------------------------------------------------------------------

bool acmacs::virus::Passage::is_egg() const
{
#include "acmacs-base/global-constructors-push.hh"
        static std::regex egg_passage{std::string(re_egg) + re_nimr_isolate +  re_niid_plus_number + re_passage_date}; // NIMR has "EGG 10-6" in h3-neut
#include "acmacs-base/diagnostics-pop.hh"
        return std::regex_search(value_, egg_passage);

} // acmacs::virus::Passage::is_egg

// ----------------------------------------------------------------------

bool acmacs::virus::Passage::is_cell() const
{
#include "acmacs-base/global-constructors-push.hh"
        static std::regex cell_passage{std::string(re_cell) + re_nimr_isolate +  re_niid_plus_number + re_passage_date};
#include "acmacs-base/diagnostics-pop.hh"
        return std::regex_search(value_, cell_passage);

} // acmacs::virus::Passage::is_cell

// ----------------------------------------------------------------------

std::string acmacs::virus::Passage::without_date() const
{
    if (value_.size() > 13 && value_[value_.size() - 1] == ')' && value_[value_.size() - 12] == '(' && value_[value_.size() - 13] == ' ' && value_[value_.size() - 4] == '-' &&
        value_[value_.size() - 7] == '-')
        return std::string(value_, 0, value_.size() - 13);
    else
        return value_;

} // acmacs::virus::Passage::without_date

// ----------------------------------------------------------------------

acmacs::virus::parse_passage_t acmacs::virus::parse_passage(std::string_view source)
{
    struct parsing_failed : public std::exception { using std::exception::exception; };
    using source_iter_t = decltype(source.cbegin());
    using callback_t = std::function<source_iter_t(source_iter_t first, source_iter_t last)>; // returns new first value, throws parsing_failed

    std::vector<std::string> parts;
    std::string last_passage_type;
    const auto parts_push = [&parts,&last_passage_type](const char* p1, std::string p2={}, source_iter_t result={}) -> source_iter_t {
        parts.push_back(p1);
        last_passage_type = p1;
        if (!p2.empty())
            parts.push_back(p2);
        return result;
    };

#include "acmacs-base/global-constructors-push.hh"
    // MDCK C
    static const std::regex re_c_mdck_x("^[\\s\\-]*[X\\?]?", std::regex::icase);
    static const std::regex re_c_mdck_n("^[\\s\\-]*(\\d+)", std::regex::icase);
    static const std::regex re_m_mdck_x("^(?:DCK|CDK)[\\s\\-]*[X\\?`]?", std::regex::icase);
    static const std::regex re_m_mdck_n("^(?:DCK|CDK)[\\s\\-]*(\\d+)", std::regex::icase);
    static const std::regex re_m_mdck_siat_x("^DCK-SIAT[\\s\\-]*[X\\?]?", std::regex::icase);
    static const std::regex re_m_mdck_siat_n("^DCK-SIAT[\\s\\-]*(\\d+)", std::regex::icase);
    static const std::regex re_m_mdck_siat1_n("^DCK-SIAT1[\\s\\-](\\d+)", std::regex::icase);

    // SIAT S
    static const std::regex re_s_siat_x("^(?:IAT)?[\\s\\-]*[X\\?]?", std::regex::icase);
    static const std::regex re_s_siat_n("^(?:IAT)?[\\s\\-]*(\\d+)", std::regex::icase);

    // QMC Seqirus (Novartis) qualified MDCK cells. Previously the cell line called "NC"
    static const std::regex re_q_qmc_x("^MC[\\s\\-]*[X\\?]?", std::regex::icase);
    static const std::regex re_q_qmc_n("^MC[\\s\\-]*(\\d+)", std::regex::icase);

    // E EGG
    static const std::regex re_e_egg_x("^(?:GG)?[\\s\\-]*[X\\?]?", std::regex::icase);
    static const std::regex re_e_egg_n("^[\\s\\-]*(\\d+)", std::regex::icase);

    // MK M - Monkey Kidney Cell line
    static const std::regex re_m_mk_x("^K?[\\s\\-]*[X\\?]?", std::regex::icase);
    static const std::regex re_m_mk_n("^K?[\\s\\-]*(\\d+)", std::regex::icase);

    // OR CS CLINICAL ORIGINAL SPECIMEN/SAMPLE
    static const std::regex re_c_clinical("^(?:S(?:-ORI)?|LINI?CAL[\\sA-Z]*)", std::regex::icase);
    static const std::regex re_o_original("^R(?:IGINAL)?[\\s\\-_\\(\\)A-Z]*", std::regex::icase);
    static const std::regex re_l_lung("^UNG[\\s\\-_A-Z]*", std::regex::icase); // NIMR
    static const std::regex re_n_nose("^(?:OSE|ASO|ASA)[\\s\\-_A-Z]*", std::regex::icase); // NIMR
    static const std::regex re_t_throat("^HROAT SWAB", std::regex::icase); // NIMR
    static const std::regex re_s_swab("^WAB", std::regex::icase);
    static const std::regex re_p_pm("^M LUNG", std::regex::icase); // NIMR
    static const std::regex re_paren_from("^FROM[\\sA-Z]+\\)", std::regex::icase);

    // R R-MIX - R-mix tissure culture
    static const std::regex re_r_n("^(?:-?M[I1]?X)?[\\s\\-]*(\\d+)", std::regex::icase);
    static const std::regex re_r_x("^(?:-?M[I1]?X)?[\\s\\-]*[X\\?]?(?!\\w)", std::regex::icase);
    static const std::regex re_rii_n("^II[\\s\\-]*(\\d+)", std::regex::icase);
    static const std::regex re_rii_x("^II[X\\?]?(?!\\w)", std::regex::icase);

    // X
    static const std::regex re_x_n("^(\\d+)", std::regex::icase);
    static const std::regex re_p_n("^(\\d+)", std::regex::icase);

    // Passage-
    static const std::regex re_passage("^ASSAGE-?", std::regex::icase);

    static const std::regex re_digits("^(\\d+)");

    static const std::map<char, callback_t> normalize_data{
        {'C', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_c_mdck_n))
                parts_push("MDCK", match[1].str());
            else if (std::regex_search(first, last, match, re_c_clinical))
                parts_push("OR");
            else if (std::regex_search(first, last, match, re_c_mdck_x))
                parts_push("MDCK", "?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'E', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_e_egg_n))
                parts_push("E", match[1].str());
            else if (std::regex_search(first, last, match, re_e_egg_x))
                parts_push("E", "?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'L', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_l_lung))
                parts_push("OR");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'M', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            // std::cerr << "  M: [" << std::string(first, last) << "]\n";
            std::cmatch match;
            if (std::regex_search(first, last, match, re_m_mdck_n))
                parts_push("MDCK", match[1].str());
            else if (std::regex_search(first, last, match, re_m_mdck_siat1_n))
                parts_push("SIAT", match[1].str());
            else if (std::regex_search(first, last, match, re_m_mdck_siat_n))
                parts_push("SIAT", match[1].str());
            else if (std::regex_search(first, last, match, re_m_mdck_siat_x))
                parts_push("SIAT", "?");
            else if (std::regex_search(first, last, match, re_m_mk_n))
                parts_push("MK", match[1].str());
            else if (std::regex_search(first, last, match, re_m_mdck_x))
                parts_push("MDCK", "?");
            else if (std::regex_search(first, last, match, re_m_mk_x))
                parts_push("MK", "?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'N', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_n_nose))
                parts_push("OR");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'O', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_o_original))
                parts_push("OR");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'P', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_p_n))
                return parts_push("X", match[1].str(), match[0].second);
            else if (std::regex_search(first, last, match, re_passage))
                return match[0].second; // ignore PASSAGE-
            else if (std::regex_search(first, last, match, re_p_pm))
                return parts_push("OR", {}, match[0].second);
            else if (first != last && (*first == 'X' || *first == 'x'))
                return parts_push("X", "?", first + 1);
            else
                throw parsing_failed{};
        }},
        {'Q', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_q_qmc_n))
                parts_push("QMC", match[1].str());
            else if (std::regex_search(first, last, match, re_q_qmc_x))
                parts_push("QMC", "?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'R', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_r_n))
                parts_push("R", match[1].str());
            else if (std::regex_search(first, last, match, re_rii_n))
                parts_push("RII", match[1].str());
            else if (std::regex_search(first, last, match, re_rii_x))
                parts_push("RII", "?");
            else if (std::regex_search(first, last, match, re_r_x))
                parts_push("R", "?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'S', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_s_siat_n))
                parts_push("SIAT", match[1].str());
            else if (std::regex_search(first, last, match, re_s_swab))
                parts_push("OR");
            else if (std::regex_search(first, last, match, re_s_siat_x))
                parts_push("SIAT", "?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'T', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_t_throat))
                parts_push("OR");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'X', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            if (first == last)
                return parts_push("X", "?", first);
            else if (*first == '?')
                return parts_push("X", "?", first + 1);
            else if (std::cmatch match; std::regex_search(first, last, match, re_x_n))
                return parts_push("X", match[1].str(), match[0].second);
            else
                return parts_push("X", "?", first);
        }},
        {' ', [](source_iter_t first, source_iter_t /*last*/) -> source_iter_t {
            return first;
        }},
        {'/', [&parts_push](source_iter_t first, source_iter_t /*last*/) -> source_iter_t {
            return parts_push("/", {}, first);
        }},
        {'\\', [&parts_push](source_iter_t first, source_iter_t last) -> source_iter_t {
            parts_push("/");
            while (first != last && *first == '\\')
                ++first;
            return first;
        }},
        {',', [&parts_push](source_iter_t first, source_iter_t /*last*/) -> source_iter_t {
            return parts_push("/", {}, first);
        }},
        {'+', [&parts,&parts_push,&last_passage_type](source_iter_t first, source_iter_t last) -> source_iter_t {
            if (std::cmatch match; !last_passage_type.empty() && last_passage_type != "/" && std::regex_search(first, last, match, re_digits)) {
                parts.push_back("/");
                return parts_push(last_passage_type.data(), match[1].str(), match[0].second);
            }
            else
                return parts_push("/", {}, first);
        }},
        {'(', [&parts](source_iter_t first, source_iter_t last) -> source_iter_t {
            if (std::cmatch match; !parts.empty() && std::regex_search(first, last, match, re_paren_from))
                return match[0].second; // ignore
            else
                throw parsing_failed{};
        }},
    };

#include "acmacs-base/diagnostics-pop.hh"

    try {
        for (auto first = source.begin(); first != source.end();) {
            // std::cerr << "  PART: [" << std::string(first, source.end()) << "]\n";
            if (const auto entry = normalize_data.find(static_cast<char>(std::toupper(*first))); entry != normalize_data.end())
                first = entry->second(first + 1, source.end());
            else
                throw parsing_failed{};
        }
        return {Passage{::string::join("", parts)}, {}};
    }
    catch (parsing_failed&) {
        return {{}, std::string{source}}; // unrecognized
    }

} // acmacs::virus::parse_passage

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
