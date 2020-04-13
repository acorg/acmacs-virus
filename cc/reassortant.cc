#include <array>

#include "acmacs-base/regex.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-strip.hh"
#include "acmacs-virus/reassortant.hh"

// ----------------------------------------------------------------------

std::tuple<acmacs::virus::Reassortant, std::string> acmacs::virus::parse_reassortant(std::string_view source)
{
    using namespace acmacs::regex;

#include "acmacs-base/global-constructors-push.hh"

#define BOL "^"
#define LOOKAHEAD_NOT_PAREN_SPACE_DASH "(?=[^\\(\\s\\-])"
#define LOOKAHEAD_NOT_PAREN_SPACE "(?=[^\\(\\s])"
#define AB_REASSORTANT "[AB]/REASSORTANT/"

#define PREFIX_1 "(?:" BOL "|" BOL AB_REASSORTANT "|" LOOKAHEAD_NOT_PAREN_SPACE_DASH ")"
// #define PREFIX_2 "(?:" BOL "|" BOL AB_REASSORTANT "|" LOOKAHEAD_NOT_PAREN_SPACE ")"

#define NYMC "(?:NYMC[\\s\\-]B?X|B?X|NYMC)[\\-\\s]?(\\d+[A-Z\\d\\-]*)\\b"
#define CBER "(?:CBER|BVR)[_\\-\\s]?(\\d+[A-Z]*)\\b"
#define IDCDC "(?:PR8[\\- ]*IDCDC[\\- ]*|I[DB]CDC-)?RG[\\- ]*([\\dA-Z\\.]+)"
#define NIB "NIB(?:SC|RG)?[\\-\\s]?([\\dA-Z]+)\\b"
#define IVR "(IVR)[\\-\\s]*(\\d+[A-Z]*)\\b"

    static const std::array normalize_data{
        look_replace_t{std::regex(PREFIX_1 NYMC, std::regex::icase), {"NYMC-$1", "$` $'"}},

        look_replace_t{std::regex(PREFIX_1 IDCDC, std::regex::icase), {"RG-$1", "$` $'"}},

        look_replace_t{std::regex(PREFIX_1 NIB, std::regex::icase), {"NIB-$1", "$` $'"}},

        look_replace_t{std::regex(PREFIX_1 CBER, std::regex::icase), {"CBER-$1", "$` $'"}},

        look_replace_t{std::regex(PREFIX_1 IVR, std::regex::icase), {"$1-$2", "$` $'"}},

        // CDC-LV is annotation, it is extra in the c2 excel parser // look_replace_t{std::regex("\\b(CDC)-?(LV\\d+[AB]?)\\b", std::regex::icase), "$1-$2"}, "$` $'",
        look_replace_t{std::regex(LOOKAHEAD_NOT_PAREN_SPACE "X[\\s\\-]+PR8", std::regex::icase), {"REASSORTANT-PR8", "$` $'"}},
        look_replace_t{std::regex(LOOKAHEAD_NOT_PAREN_SPACE "REASSORTANT-([A-Z0-9\\-\\(\\)_/:]+)", std::regex::icase), {"REASSORTANT-$1", "$` $'"}}, // manually fixed gisaid stuff
    };
#include "acmacs-base/diagnostics-pop.hh"

    // AD_DEBUG("parse_reassortant \"{}\"", source);
    if (const auto reassortant_rest = scan_replace(source, normalize_data); reassortant_rest.has_value())
        return {Reassortant{reassortant_rest->front()}, ::string::collapse_spaces(acmacs::string::strip(reassortant_rest->back()))};
    else
        return {Reassortant{}, std::string{source}};

} // acmacs::virus::parse_reassortant

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
