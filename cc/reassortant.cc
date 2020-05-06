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

#define PR_BOL "^"
#define PR_LOOKAHEAD_NOT_PAREN_SPACE_DASH "(?=[^\\(\\s\\-])"
#define PR_LOOKAHEAD_NOT_PAREN_SPACE "(?=[^\\(\\s])"
#define PR_AB_REASSORTANT "[AB]/REASSORTANT/"
#define PR_HG_REASSORTANT "HIGHGROWTH\\s+REASSORTANT\\s+"
#define PR_AB "[AB]/"

#define PR_PREFIX_1 "(?:" PR_BOL "|" PR_BOL PR_AB_REASSORTANT "|" PR_BOL PR_AB "|" PR_HG_REASSORTANT "|" PR_LOOKAHEAD_NOT_PAREN_SPACE_DASH ")"
// #define PREFIX_2 "(?:" BOL "|" BOL AB_REASSORTANT "|" LOOKAHEAD_NOT_PAREN_SPACE ")"

#define PR_NYMC "(?:NYMC[\\s\\-]B?X|B?X|NYMC)[\\-\\s]?(\\d+[A-Z\\d\\-]*)\\b"
//#define PR_NYMCX "X-(\\d+[A-Z\\d\\-]*)"
#define PR_CBER "(?:CBER|BVR)[_\\-\\s]?(\\d+[A-Z]*)\\b" // Center for Biologics Evaluation and Research https://www.fda.gov/about-fda/fda-organization/center-biologics-evaluation-and-research-cber
#define PR_IDCDC "(?:PR8[\\- ]*IDCDC[\\- _]*|I[DB]CDC-)?RG[\\- ]*([\\dA-Z\\.]+)"
#define PR_NIB "NIB(?:SC|RG)?[\\-\\s]?([\\dA-Z]+)\\b"
#define PR_IVR "(IVR|CVR)[\\-\\s]*(\\d+[A-Z]*)\\b" // IVR-153 (A(H1N1)/California/7/2009) is by CSL, CVR - by CSL/Seqirus

    static const std::array normalize_data{
        look_replace_t{std::regex(PR_PREFIX_1 PR_NYMC, std::regex::icase), {"NYMC-$1", "$` $'"}},
        // look_replace_t{std::regex(PR_AB PR_NYMCX, std::regex::icase), {"NYMC-$1", "$'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_NIB, std::regex::icase), {"NIB-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_IDCDC, std::regex::icase), {"RG-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_CBER, std::regex::icase), {"CBER-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_IVR, std::regex::icase), {"$1-$2", "$` $'"}},

        // CDC-LV is annotation, it is extra in the c2 excel parser // look_replace_t{std::regex("\\b(CDC)-?(LV\\d+[AB]?)\\b", std::regex::icase), "$1-$2"}, "$` $'",
        look_replace_t{std::regex(PR_LOOKAHEAD_NOT_PAREN_SPACE "X[\\s\\-]+PR8", std::regex::icase), {"REASSORTANT-PR8", "$` $'"}},
        look_replace_t{std::regex(PR_LOOKAHEAD_NOT_PAREN_SPACE "REASSORTANT-([A-Z0-9\\-\\(\\)_/:]+)", std::regex::icase), {"REASSORTANT-$1", "$` $'"}}, // manually fixed gisaid stuff
    };
#include "acmacs-base/diagnostics-pop.hh"

    // AD_DEBUG("parse_reassortant \"{}\"", source);
    if (const auto reassortant_rest = scan_replace(source, normalize_data); reassortant_rest.has_value()) {
        // AD_DEBUG("reassortant separated \"{}\"  \"{}\"", reassortant_rest->front(), reassortant_rest->back());
        return {Reassortant{reassortant_rest->front()}, ::string::collapse_spaces(acmacs::string::strip(reassortant_rest->back()))};
    }
    else {
        // AD_DEBUG("no reassortant in \"{}\"", source);
        return {Reassortant{}, std::string{source}};
    }

} // acmacs::virus::parse_reassortant

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
