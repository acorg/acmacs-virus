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

struct PassageNormalize
{
    std::regex look_for;
    const char* replace_fmt;
};

std::tuple<acmacs::virus::Passage, std::string> acmacs::virus::parse_passage(std::string_view source)
{
    return {Passage{source}, {}};

// #include "acmacs-base/global-constructors-push.hh"
//     static const std::array normalize_data{
//         PassageNormalize{std::regex("nonon", std::regex::icase), ""},
//     };
// #include "acmacs-base/diagnostics-pop.hh"

//     for (const auto& normalize_entry : normalize_data) {
//         if (std::cmatch match; std::regex_search(std::begin(source), std::end(source), match, normalize_entry.look_for))
//             return {Passage{match.format(normalize_entry.replace_fmt)}, ::string::join(" ", {::string::strip(match.prefix().str()), ::string::strip(match.suffix().str())})};
//     }
//     return {{}, std::string{source}};

} // acmacs::virus::parse_passage

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
