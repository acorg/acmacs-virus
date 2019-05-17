#include <regex>
#include <array>

#include "acmacs-base/string.hh"
#include "acmacs-base/date.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name.hh"

// ----------------------------------------------------------------------

constexpr const char* sre_flu_name_general_AB =
        "\\b([AB])/"                    // type \1
        "(?:\\s*([A-Z \\-_]+)\\s*/)?"   // host \2
        "\\s*([A-Z \\-_]{2,})\\s*/"     // location \3
        "\\s*0*([^/]+)\\s*/"            // isolation \4 - without leading 0
        "\\s*(\\d+)"                    // year \5 - any number of digits
        ;

constexpr const char* sre_flu_name_general_A_subtype =
        "\\b(A\\(H[1-9][0-9]?(?:N[1-9][0-9]?)?\\))/" // A(H3N2) \1
        "(?:\\s*([A-Z \\-_]+)\\s*/)?"                // host \2
        "\\s*([A-Z \\-_]{2,})\\s*/"                  // location \3
        "\\s*0*([^/]+)\\s*/"                         // isolation \4 - without leading 0
        "\\s*(\\d+)"                                 // year \5 - any number of digits
        ;

static std::string fix_location(std::string source, acmacs::virus::parse_name_f flags);
static std::string fix_year(std::string source);

// ----------------------------------------------------------------------

std::tuple<acmacs::virus::virus_name_t, acmacs::virus::Reassortant, acmacs::virus::Passage, std::string> acmacs::virus::parse_name(std::string_view source, parse_name_f flags)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::regex re_flu_name_general_AB{sre_flu_name_general_AB};
    static const std::regex re_flu_name_general_A_subtype{sre_flu_name_general_A_subtype};
#include "acmacs-base/diagnostics-pop.hh"

    virus_name_t name{""};
    std::string extra;

    const std::string source_u = ::string::upper(source);
    if (std::smatch match_general_AB; std::regex_search(source_u, match_general_AB, re_flu_name_general_AB)) {
        const std::array fields{match_general_AB[1].str(), match_general_AB[2].str(), fix_location(match_general_AB[3].str(), flags & parse_name_f::lookup_location), match_general_AB[4].str(),
                                fix_year(match_general_AB[5].str())};
        name = virus_name_t(::string::join("/", fields));
        extra = ::string::join(" ", {::string::strip(match_general_AB.prefix().str()), ::string::strip(match_general_AB.suffix().str())});
    }
    else if (std::smatch match_general_A_subtype; std::regex_search(source_u, match_general_A_subtype, re_flu_name_general_A_subtype)) {
        const std::array fields{match_general_A_subtype[1].str(), match_general_A_subtype[2].str(), fix_location(match_general_A_subtype[3].str(), flags & parse_name_f::lookup_location),
                                match_general_A_subtype[4].str(), fix_year(match_general_A_subtype[5].str())};
        name = virus_name_t(::string::join("/", fields));
        extra = ::string::join(" ", {::string::strip(match_general_A_subtype.prefix().str()), ::string::strip(match_general_A_subtype.suffix().str())});
    }
    else
        throw Error("cannot parse: " + std::string(source));

    Reassortant reassortant;
    if (!extra.empty())
        std::tie(reassortant, extra) = parse_reassortant(extra);

    Passage passage;

    return {name, reassortant, passage, extra};

} // acmacs::virus::parse_name

// ----------------------------------------------------------------------

std::string fix_location(std::string source, acmacs::virus::parse_name_f flags)
{
    if (flags != acmacs::virus::parse_name_f::lookup_location)
        return source;
    return get_locdb().find(source).name;

} // fix_location

// ----------------------------------------------------------------------

std::string fix_year(std::string source)
{
#include "acmacs-base/global-constructors-push.hh"
    static const auto current_year = static_cast<size_t>(Date(Date::Today).year());
    static const auto current_year_2 = current_year % 100;
#include "acmacs-base/diagnostics-pop.hh"

    if (const auto year = std::stoul(source); year < 18) {
        return "20" + std::to_string(year);
    }
    else if (year <= current_year_2) {
        std::cerr << "WARNING: ambigous year in virus name: " << year << '\n';
        return "20" + std::to_string(year);
    }
    else if (year < 100) {
        return "19" + std::to_string(year);
    }
    else if (year < 1900 || year > current_year) {
        throw acmacs::virus::Error(::string::concat("invalid year in the virus name: ", source));
    }
    else
        return std::to_string(year);

} // fix_year

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
