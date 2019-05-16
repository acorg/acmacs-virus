#include <regex>

#include "acmacs-base/string.hh"
#include "acmacs-virus/virus-name.hh"

constexpr const char* re_flu_name_s =
        "([AB])((?:/H[1-9][0-9]?(?:N[1-9][0-9]?)?)|(?:\\(H[1-9][0-9]?(?:N[1-9][0-9]?)?\\))|(?:H[1-9][0-9]?(?:N[1-9][0-9]?)?))?/" // type subtype
        "(?:([^/]+)/)?"      // host
        "([^/]{2,})/"        // location/
        "0*([^/]+)/"         // isolation /
        "\\d+"   // year
        ;

// ----------------------------------------------------------------------

std::tuple<acmacs::virus_name::virus_name_t, acmacs::virus::Reassortant, acmacs::virus::Passage, std::string> acmacs::virus_name::parse_name(std::string_view source)
{
#include "acmacs-base/global-constructors-push.hh"
    static std::regex re_flu_name{re_flu_name_s};
#include "acmacs-base/diagnostics-pop.hh"

    acmacs::virus_name::virus_name_t name{""};
    acmacs::virus::Reassortant reassortant;
    acmacs::virus::Passage passage;
    std::string extra;

    const std::string source_u = ::string::upper(source);
    if (std::smatch match; std::regex_search(source_u, match, re_flu_name)) {
        name = acmacs::virus_name::virus_name_t(match[0].str());
        extra = ::string::join(" ", {::string::strip(match.prefix().str()), ::string::strip(match.suffix().str())});
    }
    else
        throw Error(std::string(source));

    return {name, reassortant, passage, extra};

} // acmacs::virus_name::parse_name

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
