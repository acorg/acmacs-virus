#include <regex>
#include <array>

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

std::tuple<acmacs::virus::Passage, std::string> acmacs::virus::parse_passage(std::string_view source)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::regex re_normal("^\\s*(E|C|MDCK|S|SIAT)[\\s\\-]*(\\d+|X|\\?)\\s*", std::regex::icase);
    static const std::regex re_passage_type("^\\s*(E|C|MDCK|S|SIAT)()(?![\\w\\-])", std::regex::icase);
    static const std::regex re_mdck_siat("^\\s*MDCK-(SIAT)(\\d*)", std::regex::icase);
    static const std::regex re_mdck_siat_1("^\\s*MDCK-(SIAT)1\\s+(\\d*)\\b", std::regex::icase);
    static const std::regex re_x("^\\s*(X)\\s*(\\d+|\\?)?\\s*", std::regex::icase);
    static const std::regex re_original("^\\s*(OR)(?:IGINAL)?(?:\\s+SPECIMEN)?()\\s*", std::regex::icase);
    static const std::regex re_lab_separator("^\\s*[/,\\+]\\s*", std::regex::icase);
#include "acmacs-base/diagnostics-pop.hh"

    std::vector<std::string> parts;
    for (auto first = source.begin(); first != source.end(); ) {
        // std::cerr << "  PART: [" << std::string(first, source.end()) << "]\n";
        if (std::cmatch match; std::regex_search(first, source.end(), match, re_normal)
            || std::regex_search(first, source.end(), match, re_x)
            || std::regex_search(first, source.end(), match, re_original)
            || std::regex_search(first, source.end(), match, re_passage_type)
            || std::regex_search(first, source.end(), match, re_mdck_siat_1) // before re_mdck_siat!
            || std::regex_search(first, source.end(), match, re_mdck_siat)
            ) {
            std::string number = match[2].str();
            if (number == "X" || number.empty())
                number.assign(1, '?');
            if (match[1].length() == 1) {
                switch (*match[1].first) {
                  case 'E':
                  case 'e':
                      parts.push_back("E" + number);
                      break;
                  case 'C':
                  case 'c':
                      parts.push_back("MDCK" + number);
                      break;
                  case 'S':
                  case 's':
                      parts.push_back("SIAT" + number);
                      break;
                  case 'X':
                  case 'x':
                      parts.push_back("X" + number);
                      break;
                  default:
                      return {{}, std::string{source}}; // unrecognized
                }
            }
            else if (match[1].str() == "OR")
                parts.push_back("OR");
            else
                parts.push_back(match[1].str() + number);
            first = match[0].second;
        }
        else
            return {{}, std::string{source}};
        if (first != source.end()) {
            if (std::cmatch match; std::regex_search(first, source.end(), match, re_lab_separator)) {
                parts.push_back("/");
                first = match[0].second;
            }
        }
    }
    return {Passage{::string::join("", parts)}, {}};

} // acmacs::virus::parse_passage

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
