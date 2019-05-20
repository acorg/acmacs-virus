#include <regex>
#include <array>

#include "acmacs-base/string.hh"
#include "acmacs-base/date.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name.hh"

// ----------------------------------------------------------------------

#define SRE_AB "([AB])"
#define SRE_HOST "(?:\\s*([A-Z \\-_]+)\\s*/)?"
// #define SRE_LOC "([A-Z \\-\\.,'&_]{2,})"  // no digits!, otherwise possibility to omit / after LOC does not work
#define SRE_LOC "([A-Z0-9 \\-\\.,'&_\\?]{2,})"
#define SRE_LOC_NO_DIGITS "([A-Z \\-\\.,'&_]{2,})"
#define SRE_ISOLATION "\\s*0*([^/]+)\\s*"
#define SRE_ISOLATION_WITH_LOC "\\s*([A-Z]{3,})([^/]+)\\s*"

// A/LYON/CHU18.54.48/2018
constexpr const char* sre_flu_name_general_AB_isolation_with_location =
        "\\b"
        SRE_AB "/"                      // type \1
        SRE_HOST                        // host \2
        SRE_LOC "/"                     // location \3
        SRE_ISOLATION_WITH_LOC  "/"     // isolation \4 + \5
        "\\s*(\\d+)"                    // year \5 - any number of digits
        // "(?!(?:\\d|/\\d))"              // neither digit nor /digit at the end
        "(?![\\d\\w\\-/])"              // neither digit nor letter nor / nor - nor _
        ;

// A/SINGAPORE/INFIMH-16-0019/2016
constexpr const char* sre_flu_name_general_AB =
        "\\b"
        SRE_AB "/"                      // type \1
        SRE_HOST                        // host \2
        SRE_LOC "/"                     // location \3
        SRE_ISOLATION  "/+"             // isolation \4  - without leading 0, mutiple / at the end (found in gisaid)
        "\\s*(\\d+)"                    // year \5 - any number of digits
        ;

constexpr const char* sre_flu_name_general_AB_no_isolation = // or no slash after location
        "\\b"
        SRE_AB "/"                      // type \1
        SRE_HOST                        // host \2
        SRE_LOC_NO_DIGITS               // location \3
        SRE_ISOLATION "/"               // isolation \4
        "\\s*(\\d+)"                    // year \5 - any number of digits
        ;

constexpr const char* sre_flu_name_general_A_subtype =
        "\\b(A\\(H[1-9][0-9]?(?:N[1-9][0-9]?)?\\))/" // A(H3N2) \1
        SRE_HOST                                     // host \2
        SRE_LOC "/"                                  // location \3
        SRE_ISOLATION "/+"                           // isolation \4 - without leading 0, mutiple / at the end (found in gisaid)
        "\\s*(\\d+)"                                 // year \5 - any number of digits
        ;


constexpr const char* sre_extra_passage = "^-?(E|MDCK|C|CELL|OR|EGG)$";
constexpr const char* sre_extra_keywords = "\\b(?:NEW)\\b";
constexpr const char* sre_extra_keywords_when_reassortant = "\\b(?:HY|[BCD]-?\\d\\.\\d)\\b";
constexpr const char* sre_extra_symbols = "^[\\(\\)_\\s]+$";

static std::string fix_location(std::string source, acmacs::virus::parse_name_f flags, std::vector<acmacs::virus::parse_result_t::message_t>* messages);
static std::string fix_year(std::string source, std::vector<acmacs::virus::parse_result_t::message_t>* messages);
static acmacs::virus::virus_name_t isolation_with_location(const std::smatch& match, acmacs::virus::parse_name_f flags, std::vector<acmacs::virus::parse_result_t::message_t>& messages);
static acmacs::virus::virus_name_t general(const std::smatch& match, acmacs::virus::parse_name_f flags, std::vector<acmacs::virus::parse_result_t::message_t>& messages);

// ----------------------------------------------------------------------

acmacs::virus::parse_result_t acmacs::virus::parse_name(std::string_view source, parse_name_f flags)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::regex re_flu_name_general_AB_isolation_with_location{sre_flu_name_general_AB_isolation_with_location};
    static const std::regex re_flu_name_general_AB{sre_flu_name_general_AB};
    static const std::regex re_flu_name_general_AB_no_isolation{sre_flu_name_general_AB_no_isolation};
    static const std::regex re_flu_name_general_A_subtype{sre_flu_name_general_A_subtype};
    static const std::regex re_extra_passage{sre_extra_passage};
    static const std::regex re_extra_keywords{sre_extra_keywords};
    static const std::regex re_extra_keywords_when_reassortant{sre_extra_keywords_when_reassortant};
    static const std::regex re_extra_symbols{sre_extra_symbols};
#include "acmacs-base/diagnostics-pop.hh"

    const auto make_extra = [](const std::smatch& match) { return ::string::join(" ", {::string::strip(match.prefix().str()), ::string::strip(match.suffix().str())}); };

    virus_name_t name{""};
    std::string extra;
    std::vector<acmacs::virus::parse_result_t::message_t> messages;

    const std::string source_u = ::string::upper(source);

    if (std::smatch match_general_AB_isolation_with_location; std::regex_search(source_u, match_general_AB_isolation_with_location, re_flu_name_general_AB_isolation_with_location)) {
        // std::cerr << "isoloc: " << source_u << '\n';
        name = isolation_with_location(match_general_AB_isolation_with_location, flags, messages);
        extra = make_extra(match_general_AB_isolation_with_location);
    }
    else if (std::smatch match_general_AB; std::regex_search(source_u, match_general_AB, re_flu_name_general_AB)) {
        // std::cerr << "??: " << source_u << ' ' << match_general_AB.format("[1: $1] [host: $2] [loc: $3] [iso: $4], [y: $5]") << '\n';
        name = general(match_general_AB, flags, messages);
        extra = make_extra(match_general_AB);
    }
    else if (std::smatch match_general_AB_no_isolation; std::regex_search(source_u, match_general_AB_no_isolation, re_flu_name_general_AB_no_isolation)) {
        // std::cerr << "??: " << source_u << ' ' << match_general_AB_no_isolation.format("[1: $1] [host: $2] [loc: $3] [y: $4]") << '\n';
        const std::array fields{match_general_AB_no_isolation[1].str(), match_general_AB_no_isolation[2].str(),
                                fix_location(match_general_AB_no_isolation[3].str(), flags & parse_name_f::lookup_location, &messages), match_general_AB_no_isolation[4].str(),
                                fix_year(match_general_AB_no_isolation[5].str(), &messages)};
        name = virus_name_t(::string::join("/", fields));
        extra = make_extra(match_general_AB_no_isolation);
    }
    else if (std::smatch match_general_A_subtype; std::regex_search(source_u, match_general_A_subtype, re_flu_name_general_A_subtype)) {
        const std::array fields{match_general_A_subtype[1].str(), match_general_A_subtype[2].str(), fix_location(match_general_A_subtype[3].str(), flags & parse_name_f::lookup_location, &messages),
                                match_general_A_subtype[4].str(), fix_year(match_general_A_subtype[5].str(), &messages)};
        name = virus_name_t(::string::join("/", fields));
        extra = make_extra(match_general_A_subtype);
    }
    else {
        name = virus_name_t{source};
        messages.emplace_back(parse_result_t::message_t::unrecognized, std::string{source});
    }

    Reassortant reassortant;
    if (!extra.empty())
        std::tie(reassortant, extra) = parse_reassortant(extra);

    Passage passage;

    if (!extra.empty()) {
        if (std::smatch match_extra_passage; std::regex_match(extra, match_extra_passage, re_extra_passage)) {
            switch (extra[static_cast<size_t>(match_extra_passage.position(1))]) {
              case 'E':
                  passage = Passage{"E?"};
                  break;
              case 'M':
              case 'C':
                  passage = Passage{"MDCK?"};
                  break;
              case 'O':
                  passage = Passage{"OR"};
                  break;
              default:
                  passage = Passage{match_extra_passage[1].str()};
                  break;
            }
            extra.clear();
        }
    }

    if (!extra.empty()) {
        std::smatch match_extra_keywords;
        while (std::regex_search(extra, match_extra_keywords, re_extra_keywords))
            extra = make_extra(match_extra_keywords);
    }

    if (!extra.empty() && !reassortant.empty()) {
        std::smatch match_extra_keywords_when_reassortant;
        while (std::regex_search(extra, match_extra_keywords_when_reassortant, re_extra_keywords_when_reassortant))
            extra = make_extra(match_extra_keywords_when_reassortant);
    }

    if (!extra.empty() && std::regex_match(extra, re_extra_symbols))
        extra.clear();

    return {name, reassortant, passage, extra, messages};

} // acmacs::virus::parse_name

// ----------------------------------------------------------------------

acmacs::virus::virus_name_t isolation_with_location(const std::smatch& match, acmacs::virus::parse_name_f flags, std::vector<acmacs::virus::parse_result_t::message_t>& messages)
{
    using namespace acmacs::virus;

    // std::cerr << "isoloc: " << match.format("[1: $1] [host: $2] [loc: $3] [iso: $4] [iso: $5] [y: $6]") << '\n';

    auto location = fix_location(::string::concat(match[3].str(), ' ', match[4].str()), flags & parse_name_f::lookup_location, nullptr);
    auto isolation = match[5].str();
    if (!location.empty()) {
        if (isolation[0] == '-' || isolation[0] == '_' || isolation[0] == ' ')
            isolation.erase(0);
    }
    else {
        location = fix_location(match[3].str(), flags & parse_name_f::lookup_location, &messages);
        isolation = ::string::concat(match[4].str(), isolation);
    }
    return virus_name_t(::string::join("/", {match[1].str(), match[2].str(), location, isolation, fix_year(match[6].str(), &messages)}));

} // isolation_with_location

// ----------------------------------------------------------------------

acmacs::virus::virus_name_t general(const std::smatch& match, acmacs::virus::parse_name_f flags, std::vector<acmacs::virus::parse_result_t::message_t>& messages)
{
    using namespace acmacs::virus;

    if (auto host = match[2].str(); !host.empty()) {
        auto location = fix_location(::string::concat(host, ' ', match[3].str()), flags & parse_name_f::lookup_location, nullptr);
        auto isolation = match[4].str();
        if (!location.empty()) { // Lyon/CHU -> Lyon CHU
            host.clear();
        }
        else {
            location = fix_location(match[3].str(), flags & parse_name_f::lookup_location, nullptr);
            if (location.empty()) {
                location = fix_location(host, flags & parse_name_f::lookup_location, nullptr);
                if (location.empty()) {
                    location = match[3].str();
                    messages.emplace_back(parse_result_t::message_t::location_not_found, location);
                }
                else {
                    host.clear();
                    isolation = ::string::join("-", {match[3].str(), isolation});
                }
            }
        }
        return virus_name_t(::string::join("/", {match[1].str(), host, location, isolation, fix_year(match[5].str(), &messages)}));
    }
    else { //
        return virus_name_t(::string::join("/", {match[1].str(), host, fix_location(match[3].str(), flags & parse_name_f::lookup_location, &messages), match[4].str(), fix_year(match[5].str(), &messages)}));
    }

} // general

// ----------------------------------------------------------------------

std::string fix_location(std::string source, acmacs::virus::parse_name_f flags, std::vector<acmacs::virus::parse_result_t::message_t>* messages)
{
    if (flags != acmacs::virus::parse_name_f::lookup_location)
        return source;
    try {
        return get_locdb().find(::string::strip(source)).name;
    }
    catch (LocationNotFound& err) {
        // std::cerr << "LocationNotFound: \"" << source << "\"\n";
        if (messages) {
            messages->emplace_back(acmacs::virus::parse_result_t::message_t::location_not_found, err.what());
            return source;
        }
        else
            return {};
    }

} // fix_location

// ----------------------------------------------------------------------

std::string fix_year(std::string source, std::vector<acmacs::virus::parse_result_t::message_t>* messages)
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
        if (messages)
            messages->emplace_back(acmacs::virus::parse_result_t::message_t::invalid_year, source);
        return source;
    }
    else
        return std::to_string(year);

} // fix_year

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
