#include <regex>

#include "acmacs-virus/reassortant.hh"

// ----------------------------------------------------------------------

constexpr const char* sre_nymc = "\\b(?:B?X|NYMC)-?(\\d+[A-F]?)\\b";
constexpr const char* sre_rest = "\\b(IVR|NIB)-?(\\d+[A-F]?)\\b";

// ----------------------------------------------------------------------

std::tuple<acmacs::virus::Reassortant, std::string> acmacs::virus::parse_reassortant(std::string_view source)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::regex re_nymc{sre_nymc, std::regex::icase};
    static const std::regex re_rest{sre_rest, std::regex::icase};
#include "acmacs-base/diagnostics-pop.hh"

    Reassortant reassortant;
    std::string extra;

    if (std::cmatch match_nymc; std::regex_search(std::begin(source), std::end(source), match_nymc, re_nymc)) {
        reassortant = Reassortant{"NYMC-" + ::string::upper(match_nymc[1].str())};
        extra = ::string::join(" ", {::string::strip(match_nymc.prefix().str()), ::string::strip(match_nymc.suffix().str())});
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
