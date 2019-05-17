#include <regex>
#include <array>

#include "acmacs-base/string.hh"
#include "acmacs-base/date.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name.hh"

// ----------------------------------------------------------------------

#define SRE_HOST "(?:\\s*([A-Z \\-_]+)\\s*/)?"
#define SRE_LOC "([A-Z \\-\\.,'&_]{2,})"  // no digits!, otherwise possibility to omit / after LOC does not work
#define SRE_ISOLATION "\\s*0*([^/]+)\\s*" // mutiple / at the end (found in gisaid)

constexpr const char* sre_flu_name_general_AB =
        "\\b([AB])/"                    // type \1
        SRE_HOST                        // host \2
        SRE_LOC "/"                     // location \3
        SRE_ISOLATION  "/+"             // isolation \4 - without leading 0, mutiple / at the end (found in gisaid)
        "\\s*(\\d+)"                    // year \5 - any number of digits
        ;

constexpr const char* sre_flu_name_general_AB_no_slash_after_location =
        "\\b([AB])/"                    // type \1
        SRE_HOST                        // host \2
        SRE_LOC                         // location \3, no slash!
        SRE_ISOLATION  "/+"             // isolation \4 - without leading 0, mutiple / at the end (found in gisaid)
        "\\s*(\\d+)"                    // year \5 - any number of digits
        ;

constexpr const char* sre_flu_name_general_AB_numeric_isolation_split = // A/Zambia/13/177/2013
        "\\b([AB])/"                    // type \1
        SRE_HOST                        // host \2
        SRE_LOC "/"                     // location \3
        "\\s*0*(\\d+)\\s*/"             // first isolation \4, ignore leading 0
        "\\s*(\\d+)\\s*/"               // second isolation \5
        "\\s*(\\d+)"                    // year \6 - any number of digits
        ;

constexpr const char* sre_flu_name_general_AB_location_split = // A/Lyon/CHU/R18.50.47/2018
        "\\b([AB])/"                    // type \1
        SRE_HOST                        // host \2
        SRE_LOC "/"                     // location \3
        SRE_LOC "/"                     // second location \4
        SRE_ISOLATION  "/+"             // isolation \5 - without leading 0, mutiple / at the end (found in gisaid)
        "\\s*(\\d+)"                    // year \6 - any number of digits
        ;

constexpr const char* sre_flu_name_general_A_subtype =
        "\\b(A\\(H[1-9][0-9]?(?:N[1-9][0-9]?)?\\))/" // A(H3N2) \1
        SRE_HOST                                     // host \2
        SRE_LOC "/"                                  // location \3
        SRE_ISOLATION "/+"                           // isolation \4 - without leading 0, mutiple / at the end (found in gisaid)
        "\\s*(\\d+)"                                 // year \5 - any number of digits
        ;


constexpr const char* sre_extra_keywords = "\\b(?:NEW)\\b";
constexpr const char* sre_extra_symbols = "^[\\(\\)_\\s]+$";

static std::string fix_location(std::string source, acmacs::virus::parse_name_f flags);
static std::string fix_year(std::string source);

// ----------------------------------------------------------------------

std::tuple<acmacs::virus::virus_name_t, acmacs::virus::Reassortant, acmacs::virus::Passage, std::string> acmacs::virus::parse_name(std::string_view source, parse_name_f flags)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::regex re_flu_name_general_AB{sre_flu_name_general_AB};
    static const std::regex re_flu_name_general_AB_no_slash_after_location{sre_flu_name_general_AB_no_slash_after_location};
    static const std::regex re_flu_name_general_AB_numeric_isolation_split{sre_flu_name_general_AB_numeric_isolation_split};
    static const std::regex re_flu_name_general_AB_location_split{sre_flu_name_general_AB_location_split};
    static const std::regex re_flu_name_general_A_subtype{sre_flu_name_general_A_subtype};
    static const std::regex re_extra_keywords{sre_extra_keywords};
    static const std::regex re_extra_symbols{sre_extra_symbols};
#include "acmacs-base/diagnostics-pop.hh"

    const auto make_extra = [](const std::smatch& match) { return ::string::join(" ", {::string::strip(match.prefix().str()), ::string::strip(match.suffix().str())}); };

    virus_name_t name{""};
    std::string extra;

    const std::string source_u = ::string::upper(source);
    if (std::smatch match_general_AB_numeric_isolation_split; std::regex_search(source_u, match_general_AB_numeric_isolation_split, re_flu_name_general_AB_numeric_isolation_split)) {
        const std::array fields{match_general_AB_numeric_isolation_split[1].str(), match_general_AB_numeric_isolation_split[2].str(),
                                fix_location(match_general_AB_numeric_isolation_split[3].str(), flags & parse_name_f::lookup_location),
                                ::string::concat(match_general_AB_numeric_isolation_split[4].str(), '-', match_general_AB_numeric_isolation_split[5].str()),
                                fix_year(match_general_AB_numeric_isolation_split[6].str())};
        name = virus_name_t(::string::join("/", fields));
        extra = make_extra(match_general_AB_numeric_isolation_split);
    }
    else if (std::smatch match_general_AB_location_split; std::regex_search(source_u, match_general_AB_location_split, re_flu_name_general_AB_location_split)) {
        const std::array fields{match_general_AB_location_split[1].str(), match_general_AB_location_split[2].str(),
                                fix_location(::string::concat(match_general_AB_location_split[3].str(), ' ', match_general_AB_location_split[4].str()), flags & parse_name_f::lookup_location),
                                match_general_AB_location_split[5].str(),
                                fix_year(match_general_AB_location_split[6].str())};
        name = virus_name_t(::string::join("/", fields));
        extra = make_extra(match_general_AB_location_split);
    }
    else if (std::smatch match_general_AB;
        std::regex_search(source_u, match_general_AB, re_flu_name_general_AB) || std::regex_search(source_u, match_general_AB, re_flu_name_general_AB_no_slash_after_location)) {
        const std::array fields{match_general_AB[1].str(), match_general_AB[2].str(), fix_location(match_general_AB[3].str(), flags & parse_name_f::lookup_location), match_general_AB[4].str(),
                                fix_year(match_general_AB[5].str())};
        name = virus_name_t(::string::join("/", fields));
        extra = make_extra(match_general_AB);
    }
    else if (std::smatch match_general_A_subtype; std::regex_search(source_u, match_general_A_subtype, re_flu_name_general_A_subtype)) {
        const std::array fields{match_general_A_subtype[1].str(), match_general_A_subtype[2].str(), fix_location(match_general_A_subtype[3].str(), flags & parse_name_f::lookup_location),
                                match_general_A_subtype[4].str(), fix_year(match_general_A_subtype[5].str())};
        name = virus_name_t(::string::join("/", fields));
        extra = make_extra(match_general_A_subtype);
    }
    else
        throw Error("cannot parse: " + std::string(source));

    Reassortant reassortant;
    if (!extra.empty())
        std::tie(reassortant, extra) = parse_reassortant(extra);

    Passage passage;

    if (!extra.empty()) {
        std::smatch match_extra_keywords;
        while (std::regex_search(extra, match_extra_keywords, re_extra_keywords))
            extra = make_extra(match_extra_keywords);
    }

    if (!extra.empty() && std::regex_match(extra, re_extra_symbols))
        extra.clear();

    return {name, reassortant, passage, extra};

} // acmacs::virus::parse_name

// ----------------------------------------------------------------------

std::string fix_location(std::string source, acmacs::virus::parse_name_f flags)
{
    if (flags != acmacs::virus::parse_name_f::lookup_location)
        return source;
    return get_locdb().find(::string::strip(source)).name;

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
        // std::cerr << "WARNING: ambigous year in virus name: " << year << '\n';
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
