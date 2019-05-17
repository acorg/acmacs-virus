#include <regex>

#include "acmacs-virus/reassortant.hh"

// ----------------------------------------------------------------------

constexpr const char* sre_nymc = "\\b(?:NYMC\\sX|B?X|NYMC)-?(\\d+[A-F]?)\\b";
constexpr const char* sre_nib = "\\bNIB(?:SC)?-?(\\d+[A-F]?)\\b";
constexpr const char* sre_cber = "\\b(?:CBER|BVR)-?(\\d+[A-F]?)\\b";
constexpr const char* sre_rest = "\\b(IVR)-?(\\d+[A-F]?)\\b";

// ----------------------------------------------------------------------

std::tuple<acmacs::virus::Reassortant, std::string> acmacs::virus::parse_reassortant(std::string_view source)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::regex re_nymc{sre_nymc, std::regex::icase};
    static const std::regex re_nib{sre_nib, std::regex::icase};
    static const std::regex re_cber{sre_cber, std::regex::icase};
    static const std::regex re_rest{sre_rest, std::regex::icase};
#include "acmacs-base/diagnostics-pop.hh"

    Reassortant reassortant;
    std::string extra;

    if (std::cmatch match_nymc; std::regex_search(std::begin(source), std::end(source), match_nymc, re_nymc)) {
        reassortant = Reassortant{"NYMC-" + ::string::upper(match_nymc[1].str())};
        extra = ::string::join(" ", {::string::strip(match_nymc.prefix().str()), ::string::strip(match_nymc.suffix().str())});
    }
    else if (std::cmatch match_nib; std::regex_search(std::begin(source), std::end(source), match_nib, re_nib)) {
        reassortant = Reassortant{"NIB-" + ::string::upper(match_nib[1].str())};
        extra = ::string::join(" ", {::string::strip(match_nib.prefix().str()), ::string::strip(match_nib.suffix().str())});
    }
    else if (std::cmatch match_cber; std::regex_search(std::begin(source), std::end(source), match_cber, re_cber)) {
        reassortant = Reassortant{"CBER-" + ::string::upper(match_cber[1].str())};
        extra = ::string::join(" ", {::string::strip(match_cber.prefix().str()), ::string::strip(match_cber.suffix().str())});
    }
    else if (std::cmatch match_rest; std::regex_search(std::begin(source), std::end(source), match_rest, re_rest)) {
        reassortant = Reassortant{::string::upper(::string::concat(match_rest[1].str(), '-', match_rest[2].str()))};
        extra = ::string::join(" ", {::string::strip(match_rest.prefix().str()), ::string::strip(match_rest.suffix().str())});
    }
    else
        extra = source;

    return {reassortant, extra};

} // acmacs::virus::parse_reassortant

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
