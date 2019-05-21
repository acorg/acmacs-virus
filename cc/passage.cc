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

#include "acmacs-base/global-constructors-push.hh"
    // MDCK C
    static const std::regex re_c_mdck_x("^[\\s\\-]*[X\\?]?", std::regex::icase);
    static const std::regex re_c_mdck_n("^[\\s\\-]*(\\d+)", std::regex::icase);
    static const std::regex re_m_mdck_x("^DCK[\\s\\-]*[X\\?]?", std::regex::icase);
    static const std::regex re_m_mdck_n("^DCK[\\s\\-]*(\\d+)", std::regex::icase);
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
    static const std::regex re_c_clinical("^(?:S(?:-ORI)?|LINICAL\\s*(?:SPECIMEN|SAMPLE))", std::regex::icase);
    static const std::regex re_o_original("^R(?:IGINAL)?\\s*(?:SPECIMEN|SAMPLE)?)", std::regex::icase);

    // X
    static const std::regex re_x_n("^(\\d+)", std::regex::icase);
    static const std::regex re_p_n("^(\\d+)", std::regex::icase);

    static const std::map<char, callback_t> normalize_data{
        {'C', [&parts](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_c_mdck_n))
                parts.push_back("MDCK" + match[1].str());
            else if (std::regex_search(first, last, match, re_c_clinical))
                parts.push_back("OR");
            else if (std::regex_search(first, last, match, re_c_mdck_x))
                parts.push_back("MDCK?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'E', [&parts](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_e_egg_n))
                parts.push_back("E" + match[1].str());
            else if (std::regex_search(first, last, match, re_e_egg_x))
                parts.push_back("E?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'M', [&parts](source_iter_t first, source_iter_t last) -> source_iter_t {
            // std::cerr << "  M: [" << std::string(first, last) << "]\n";
            std::cmatch match;
            if (std::regex_search(first, last, match, re_m_mdck_n))
                parts.push_back("MDCK" + match[1].str());
            else if (std::regex_search(first, last, match, re_m_mdck_siat1_n))
                parts.push_back("SIAT" + match[1].str());
            else if (std::regex_search(first, last, match, re_m_mdck_siat_n))
                parts.push_back("SIAT" + match[1].str());
            else if (std::regex_search(first, last, match, re_m_mdck_siat_x))
                parts.push_back("SIAT?");
            else if (std::regex_search(first, last, match, re_m_mk_n))
                parts.push_back("MK" + match[1].str());
            else if (std::regex_search(first, last, match, re_m_mdck_x))
                parts.push_back("MDCK?");
            else if (std::regex_search(first, last, match, re_m_mk_x))
                parts.push_back("MK?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'O', [&parts](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_o_original))
                parts.push_back("OR");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'P', [&parts](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_p_n)) {
                parts.push_back("X" + match[1].str());
                return match[0].second;
            }
            else if (first != last && (*first == 'X' || *first == 'x')) {
                parts.push_back("X?");
                return first + 1;
            }
            else
                throw parsing_failed{};
        }},
        {'Q', [&parts](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_q_qmc_n))
                parts.push_back("QMC" + match[1].str());
            else if (std::regex_search(first, last, match, re_q_qmc_x))
                parts.push_back("QMC?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'S', [&parts](source_iter_t first, source_iter_t last) -> source_iter_t {
            std::cmatch match;
            if (std::regex_search(first, last, match, re_s_siat_n))
                parts.push_back("SIAT" + match[1].str());
            else if (std::regex_search(first, last, match, re_s_siat_x))
                parts.push_back("SIAT?");
            else
                throw parsing_failed{};
            return match[0].second;
        }},
        {'X', [&parts](source_iter_t first, source_iter_t last) -> source_iter_t {
            if (first == last) {
                parts.push_back("X?");
                return first;
            }
            else if (*first == '?') {
                parts.push_back("X?");
                return first + 1;
            }
            else if (std::cmatch match; std::regex_search(first, last, match, re_x_n)) {
                parts.push_back("X" + match[1].str());
                return match[0].second;
            }
            else {
                parts.push_back("X?");
                return first;
            }
        }},
        {' ', [](source_iter_t first, source_iter_t /*last*/) -> source_iter_t {
            return first;
        }},
        {'/', [&parts](source_iter_t first, source_iter_t /*last*/) -> source_iter_t {
            parts.push_back("/");
            return first;
        }},
        {',', [&parts](source_iter_t first, source_iter_t /*last*/) -> source_iter_t {
            parts.push_back("/");
            return first;
        }},
        {'+', [&parts](source_iter_t first, source_iter_t /*last*/) -> source_iter_t {
            parts.push_back("/");
            return first;
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

    //     if (std::cmatch match; std::regex_search(first, source.end(), match, re_normal) || std::regex_search(first, source.end(), match, re_x) ||
    //                            std::regex_search(first, source.end(), match, re_original) || std::regex_search(first, source.end(), match, re_passage_type) ||
    //                            std::regex_search(first, source.end(), match, re_mdck_siat_1) // before re_mdck_siat!
    //                            || std::regex_search(first, source.end(), match, re_mdck_siat)) {
    //         std::string number = match[2].str();
    //         if (number == "X" || number.empty())
    //             number.assign(1, '?');
    //         if (match[1].length() == 1) {
    //             switch (*match[1].first) {
    //                 case 'E':
    //                 case 'e':
    //                     parts.push_back("E" + number);
    //                     break;
    //                 case 'C':
    //                 case 'c':
    //                     parts.push_back("MDCK" + number);
    //                     break;
    //                 case 'S':
    //                 case 's':
    //                     parts.push_back("SIAT" + number);
    //                     break;
    //                 case 'M':
    //                 case 'm':
    //                     parts.push_back("MK" + number);
    //                     break;
    //                 case 'X':
    //                 case 'x':
    //                     parts.push_back("X" + number);
    //                     break;
    //                 default:
    //                     return {{}, std::string{source}}; // unrecognized
    //             }
    //         }
    //         else if (match[1].str() == "OR")
    //             parts.push_back("OR");
    //         else
    //             parts.push_back(match[1].str() + number);
    //         first = match[0].second;
    //     }
    //     else
    //         return {{}, std::string{source}};
    //     if (first != source.end()) {
    //         if (std::cmatch match; std::regex_search(first, source.end(), match, re_lab_separator)) {
    //             parts.push_back("/");
    //             first = match[0].second;
    //         }
    //     }
    // }
    // return {Passage{::string::join("", parts)}, {}};

} // acmacs::virus::parse_passage

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
