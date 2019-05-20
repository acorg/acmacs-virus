#include <regex>
#include <array>

#include "acmacs-virus/reassortant.hh"

// ----------------------------------------------------------------------

struct ReassortantNormalize
{
    std::regex look_for;
    const char* replace_fmt;
};

// ----------------------------------------------------------------------

std::tuple<acmacs::virus::Reassortant, std::string> acmacs::virus::parse_reassortant(std::string_view source)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::array replacements{
        ReassortantNormalize{std::regex("\\b(?:NYMC[\\s\\-]B?X|B?X|NYMC)[\\-\\s]?(\\d+[A-Z]*)\\b", std::regex::icase), "NYMC-$1"},
        ReassortantNormalize{std::regex("\\bNIB(?:SC)?[\\-\\s]?(\\d+[A-Z]*)\\b", std::regex::icase), "NIB-$1"},
        ReassortantNormalize{std::regex("\\b(?:CBER|BVR)[\\-\\s]?(\\d+[A-Z]*)\\b", std::regex::icase), "CBER-$1"},
        ReassortantNormalize{std::regex("\\b(CDC)-?(LV\\d+[AB]?)\\b", std::regex::icase), "$1-$2"},
        ReassortantNormalize{std::regex("\\b(?:PR8[\\- ]*IDCDC[\\- ]*)?RG[\\- ]*(\\d+)", std::regex::icase), "RG-$1"},

        ReassortantNormalize{std::regex("\\b(IVR)[\\-\\s]?(\\d+[A-Z]*)\\b", std::regex::icase), "$1-$2"},
    };
#include "acmacs-base/diagnostics-pop.hh"

    for (const auto& replacement : replacements) {
        if (std::cmatch match; std::regex_search(std::begin(source), std::end(source), match, replacement.look_for))
            return {Reassortant{match.format(replacement.replace_fmt)}, ::string::join(" ", {::string::strip(match.prefix().str()), ::string::strip(match.suffix().str())})};
    }
    return {{}, std::string{source}};

} // acmacs::virus::parse_reassortant

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
