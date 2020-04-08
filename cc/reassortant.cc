#include <array>

#include "acmacs-base/regex.hh"
#include "acmacs-base/string.hh"
#include "acmacs-virus/reassortant.hh"

// ----------------------------------------------------------------------

std::tuple<acmacs::virus::Reassortant, std::string> acmacs::virus::parse_reassortant(std::string_view source)
{
    using namespace acmacs::regex;

#include "acmacs-base/global-constructors-push.hh"

#define BOL "^"
#define LOOKAHEAD_NOT_PAREN_SPACE "(?=[\\(\\s])"
#define AB_REASSORTANT "[AB]/REASSORTANT/"

#define PREFIX "(?:" BOL "|" BOL AB_REASSORTANT "|" LOOKAHEAD_NOT_PAREN_SPACE ")"

#define NYMC "(?:NYMC[\\s\\-]B?X|B?X|NYMC)[\\-\\s]?(\\d+[A-Z\\d\\-]*)\\b"
#define CBER "(?:CBER|BVR)[_\\-\\s]?(\\d+[A-Z]*)\\b"
#define IDCDC "(?:PR8[\\- ]*IDCDC[\\- ]*|I[DB]CDC-)?RG[\\- ]*([\\dA-Z\\.]+)"
#define NIB "NIB(?:SC|RG)?[\\-\\s]?([\\dA-Z]+)\\b"
#define IVR "(IVR)[\\-\\s]*(\\d+[A-Z]*)\\b"

    static const std::array normalize_data{
        look_replace2_t{std::regex(PREFIX NYMC, std::regex::icase), "NYMC-$1", "$` $'"},

        look_replace2_t{std::regex(PREFIX IDCDC, std::regex::icase), "RG-$1", "$` $'"},

        look_replace2_t{std::regex(PREFIX NIB, std::regex::icase), "NIB-$1", "$` $'"},

        look_replace2_t{std::regex(PREFIX CBER, std::regex::icase), "CBER-$1", "$` $'"},

        look_replace2_t{std::regex(PREFIX IVR, std::regex::icase), "$1-$2", "$` $'"},

        // CDC-LV is annotation, it is extra in the c2 excel parser // look_replace2_t{std::regex("\\b(CDC)-?(LV\\d+[AB]?)\\b", std::regex::icase), "$1-$2"}, "$` $'",
        look_replace2_t{std::regex(LOOKAHEAD_NOT_PAREN_SPACE "X[\\s\\-]+PR8", std::regex::icase), "REASSORTANT-PR8", "$` $'"},
        look_replace2_t{std::regex(LOOKAHEAD_NOT_PAREN_SPACE "REASSORTANT-([A-Z0-9\\-\\(\\)_/:]+)", std::regex::icase), "REASSORTANT-$1", "$` $'"}, // manually fixed gisaid stuff
    };
#include "acmacs-base/diagnostics-pop.hh"

    const auto [reassortant, rest] = scan_replace2(source, normalize_data);
    if (reassortant.empty() && rest.empty()) // nothing found
        return {Reassortant{}, std::string{source}};
    else
        return {Reassortant{reassortant}, string::collapse_spaces(string::strip(rest))};

} // acmacs::virus::parse_reassortant

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
