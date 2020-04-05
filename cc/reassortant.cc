#include <array>

#include "acmacs-base/regex.hh"
#include "acmacs-base/string.hh"
#include "acmacs-virus/reassortant.hh"

// ----------------------------------------------------------------------

std::tuple<acmacs::virus::Reassortant, std::string> acmacs::virus::parse_reassortant(std::string_view source)
{
    using namespace acmacs::regex;

#include "acmacs-base/global-constructors-push.hh"
    static const std::array normalize_data{
        look_replace2_t{std::regex("\\b(?:NYMC[\\s\\-]B?X|B?X|NYMC)[\\-\\s]?(\\d+[A-Z]*)\\b", std::regex::icase), "NYMC-$1", "$` $'"},
        look_replace2_t{std::regex("\\b(?:PR8[\\- ]*IDCDC[\\- ]*|I[DB]CDC-)?RG[\\- ]*([\\dA-Z\\.]+)", std::regex::icase), "RG-$1", "$` $'"},
        look_replace2_t{std::regex("\\bNIB(?:SC|RG)?[\\-\\s]?([\\dA-Z]+)\\b", std::regex::icase), "NIB-$1", "$` $'"},
        look_replace2_t{std::regex("\\b(?:CBER|BVR)[\\-\\s]?(\\d+[A-Z]*)\\b", std::regex::icase), "CBER-$1", "$` $'"},
        // CDC-LV is annotation, it is extra in the c2 excel parser // look_replace2_t{std::regex("\\b(CDC)-?(LV\\d+[AB]?)\\b", std::regex::icase), "$1-$2"}, "$` $'",
        look_replace2_t{std::regex("\\bX[\\s\\-]+PR8", std::regex::icase), "REASSORTANT-PR8", "$` $'"},
        look_replace2_t{std::regex("\\bREASSORTANT-([A-Z0-9\\-\\(\\)_/:]+)", std::regex::icase), "REASSORTANT-$1", "$` $'"}, // manually fixed gisaid stuff

        look_replace2_t{std::regex("\\b(IVR)[\\-\\s]?(\\d+[A-Z]*)\\b", std::regex::icase), "$1-$2", "$` $'"},
    };
#include "acmacs-base/diagnostics-pop.hh"

    const auto [reassortant, rest] = scan_replace2(source, normalize_data);
    return {Reassortant{reassortant}, string::collapse_spaces(string::strip(rest))};

} // acmacs::virus::parse_reassortant

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
