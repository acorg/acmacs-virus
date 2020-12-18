#include <array>

#include "acmacs-base/regex.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/string-strip.hh"
#include "acmacs-virus/reassortant.hh"
#include "acmacs-virus/log.hh"

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

#define PR_NUMBER     "[\\-\\s]?(\\d+[A-Z\\d\\-]*)\\b"
#define PR_NYMC       PR_PREFIX_1 "(?:NYMC[\\s\\-]B?X|BX|NYMC)" PR_NUMBER
#define PR_NYMCX_0    "X" PR_NUMBER
#define PR_NYMCX_1    "^" PR_NYMCX_0
#define PR_NYMCX_2    "([\\s_])" PR_NYMCX_0
#define PR_NYMCX_3    PR_PREFIX_1 PR_NYMCX_0
#define PR_CBER       "(?:CBER|BVR)" PR_NUMBER // Center for Biologics Evaluation and Research https://www.fda.gov/about-fda/fda-organization/center-biologics-evaluation-and-research-cber
#define PR_IDCDC      "(?:PR8[\\- ]*IDCDC[\\- _]*|I[DB]CDC-)?RG[\\- ]*([\\dA-Z\\.]+)"
#define PR_NIB        "NIB(?:SC|RG)?" PR_NUMBER
#define PR_IVR        "(IVR|CVR)" PR_NUMBER // IVR-153 (A(H1N1)/California/7/2009) is by CSL, CVR - by CSL/Seqirus
#define PR_MELB       "(PR8)[-_\\s]*(?:HY)?"             // MELB (Malet) reassortant spec, e.g. "A/DRY VALLEYS/1/2020_PR8-HY"

    static const std::array normalize_data{
        look_replace_t{std::regex(PR_NYMC, std::regex::icase), {"NYMC-$1", "$` $'"}},
        look_replace_t{std::regex(PR_NYMCX_1, std::regex::icase), {"NYMC-$1", "$` $'"}},
        look_replace_t{std::regex(PR_NYMCX_2, std::regex::icase), {"NYMC-$2", "$`$1 $'"}},
        look_replace_t{std::regex(PR_NYMCX_3, std::regex::icase), {"NYMC-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_NIB, std::regex::icase), {"NIB-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_IDCDC, std::regex::icase), {"RG-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_CBER, std::regex::icase), {"CBER-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_IVR, std::regex::icase), {"$1-$2", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_MELB, std::regex::icase), {"$1", "$` $'"}},

        // CDC-LV is annotation, it is extra in the c2 excel parser // look_replace_t{std::regex("\\b(CDC)-?(LV\\d+[AB]?)\\b", std::regex::icase), "$1-$2"}, "$` $'",
        look_replace_t{std::regex(PR_LOOKAHEAD_NOT_PAREN_SPACE "X[\\s\\-]+PR8", std::regex::icase), {"REASSORTANT-PR8", "$` $'"}},
        look_replace_t{std::regex(PR_LOOKAHEAD_NOT_PAREN_SPACE "REASSORTANT-([A-Z0-9\\-\\(\\)_/:]+)", std::regex::icase), {"REASSORTANT-$1", "$` $'"}}, // manually fixed gisaid stuff
    };
#include "acmacs-base/diagnostics-pop.hh"

    AD_LOG(acmacs::log::name_parsing, "reassortant source: \"{}\"", source);
    if (const auto reassortant_rest = scan_replace(source, normalize_data); reassortant_rest.has_value()) {
        AD_LOG(acmacs::log::name_parsing, "reassortant: \"{}\" extra:\"{}\" <-- \"{}\"", reassortant_rest->front(), reassortant_rest->back(), source);
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
