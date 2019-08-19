#include <regex>
#include <array>

#include "acmacs-base/string-split.hh"
#include "acmacs-base/date.hh"
#include "acmacs-base/fmt.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name.hh"
#include "acmacs-virus/passage.hh"

// ----------------------------------------------------------------------

#define SRE_AB "([AB])"
#define SRE_HOST "(?:\\s*([A-Z \\-_]+)\\s*/)?"
// #define SRE_LOC "([A-Z \\-\\.,'&_]{2,})"  // no digits!, otherwise possibility to omit / after LOC does not work
#define SRE_LOC "([A-Z0-9 \\-\\.,'&_\\?]{2,})"
#define SRE_LOC_NO_DIGITS "([A-Z \\-\\.,'&_]{2,})"
#define SRE_ISOLATION "\\s*0*([^/]+)\\s*"
#define SRE_ISOLATION_WITH_LOC "\\s*([A-Z]{3,})([^/]+)\\s*"
#define SRE_LOOKAHEAD_EXCEPT "(?![\\.\\d[:alpha:]\\-/])"

// A/LYON/CHU18.54.48/2018
constexpr const char* sre_flu_name_general_AB_isolation_with_location =
        "\\b"
        SRE_AB "/"                      // type \1
        SRE_HOST                        // host \2
        SRE_LOC "/"                     // location \3
        SRE_ISOLATION_WITH_LOC  "/"     // isolation \4 + \5
        "\\s*(\\d+)"                    // year \6 - any number of digits
        SRE_LOOKAHEAD_EXCEPT              // neither digit nor letter nor / nor - nor _ nor .
        ;

// A/SINGAPORE/INFIMH-16-0019/2016
constexpr const char* sre_flu_name_general_AB_1 =
        "\\b"
        SRE_AB "/"                      // type \1
        SRE_HOST                        // host \2
        SRE_LOC_NO_DIGITS "/"           // location \3
        SRE_ISOLATION  "/+"             // isolation \4  - without leading 0, mutiple / at the end (found in gisaid)
        "\\s*(\\d+)"                    // year \5 - any number of digits
        SRE_LOOKAHEAD_EXCEPT            // neither digit nor letter nor / nor - nor _ nor .
        ;

constexpr const char* sre_flu_name_general_AB_2 =
        "\\b"
        SRE_AB "/"                      // type \1
        SRE_HOST                        // host \2
        SRE_LOC "/"                     // location \3
        SRE_ISOLATION  "/+"             // isolation \4  - without leading 0, mutiple / at the end (found in gisaid)
        "\\s*(\\d+)"                    // year \5 - any number of digits
        SRE_LOOKAHEAD_EXCEPT            // neither digit nor letter nor / nor - nor _ nor .
        ;

constexpr const char* sre_flu_name_general_AB_no_isolation = // or no slash after location
        "\\b"
        SRE_AB "/"                      // type \1
        SRE_HOST                        // host \2
        SRE_LOC_NO_DIGITS               // location \3
        SRE_ISOLATION "/"               // isolation \4
        "\\s*(\\d+)"                    // year \5 - any number of digits
        SRE_LOOKAHEAD_EXCEPT            // neither digit nor letter nor / nor - nor _ nor .
        ;

constexpr const char* sre_flu_name_general_A_subtype =
        "\\b(A\\(H[1-9][0-9]?(?:N[1-9][0-9]?)?\\))/" // A(H3N2) \1
        SRE_HOST                                     // host \2
        SRE_LOC "/"                                  // location \3
        SRE_ISOLATION "/+"                           // isolation \4 - without leading 0, mutiple / at the end (found in gisaid)
        "\\s*(\\d+)"                                 // year \5 - any number of digits
        SRE_LOOKAHEAD_EXCEPT            // neither digit nor letter nor / nor - nor _ nor .
        ;


struct parse_name_error : public std::exception {};

struct location_t
{
    std::string name;
    std::string country;
    std::string continent;
};

struct name_data_t
{
    acmacs::virus::v2::virus_name_t name;
    acmacs::virus::v2::host_t host;
    std::string country;
    std::string continent;
};

static location_t fix_location(std::string source, acmacs::virus::v2::parse_name_f flags, std::vector<acmacs::virus::v2::parse_result_t::message_t>* messages);
static std::string fix_year(std::string source, std::vector<acmacs::virus::v2::parse_result_t::message_t>* messages);
static name_data_t isolation_with_location(const std::smatch& match, acmacs::virus::v2::parse_name_f flags, std::vector<acmacs::virus::v2::parse_result_t::message_t>& messages);
static name_data_t general(const std::smatch& match, acmacs::virus::v2::parse_name_f flags, std::vector<acmacs::virus::v2::parse_result_t::message_t>& messages);

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"

static const std::regex re_flu_name_general_AB_isolation_with_location{sre_flu_name_general_AB_isolation_with_location};
static const std::regex re_flu_name_general_AB_1{sre_flu_name_general_AB_1};
static const std::regex re_flu_name_general_AB_2{sre_flu_name_general_AB_2};
static const std::regex re_flu_name_general_AB_no_isolation{sre_flu_name_general_AB_no_isolation};
static const std::regex re_flu_name_general_A_subtype{sre_flu_name_general_A_subtype};
static const std::regex re_extra_remove{"\\b(?:NEW)\\b"}; // NEW
static const std::regex re_extra_remove_when_reassortant{"\\b(?:HY)\\b"}; // HY
static const std::regex re_extra_symbols{"^[\\(\\)_\\-\\s,\\.]+$"};
static const std::regex re_flu_a_subtype{"\\(H[1-9][0-9]?(?:N[1-9][0-9]?V?)?\\)"};
static const std::regex re_location_stop_list("(?:REASSORTANT|DOMESTIC|EQUINE|SWINE|-$)", std::regex::icase); // if location matches, throw parse_name_error (i.e. name unrecognized)

#include "acmacs-base/diagnostics-pop.hh"

// ----------------------------------------------------------------------

acmacs::virus::v2::parse_result_t acmacs::virus::v2::parse_name(std::string_view source, parse_name_f flags)
{

    const auto make_extra = [](const std::smatch& match) { return ::string::join(" ", {::string::strip(match.prefix().str()), ::string::strip(match.suffix().str())}); };

    name_data_t name_data{virus_name_t{}, host_t{}, std::string{}, std::string{}};
    std::string extra;
    std::vector<acmacs::virus::v2::parse_result_t::message_t> messages;

    const std::string source_u = ::string::upper(source);

    try {
        if (std::smatch match_general_AB_isolation_with_location; std::regex_search(source_u, match_general_AB_isolation_with_location, re_flu_name_general_AB_isolation_with_location)) {
            // fmt::print(stderr, "isoloc: {} {}\n", source_u, match_general_AB_isolation_with_location.format("[1: $1] [host: $2] [loc: $3] [iso1: $4], [iso2: $5], [y: $6]"));
            name_data = isolation_with_location(match_general_AB_isolation_with_location, flags, messages);
            extra = make_extra(match_general_AB_isolation_with_location);
        }
        else if (std::smatch match_general_AB_1; std::regex_search(source_u, match_general_AB_1, re_flu_name_general_AB_1)) {
            //std::cerr << "genAB1: " << source_u << ' ' << match_general_AB_1.format("[1: $1] [host: $2] [loc: $3] [iso: $4], [y: $5]") << '\n';
            name_data = general(match_general_AB_1, flags, messages);
            extra = make_extra(match_general_AB_1);
        }
        else if (std::smatch match_general_AB_2; std::regex_search(source_u, match_general_AB_2, re_flu_name_general_AB_2)) {
            // std::cerr << "genAB2: " << source_u << ' ' << match_general_AB_2.format("[1: $1] [host: $2] [loc: $3] [iso: $4], [y: $5]") << '\n';
            name_data = general(match_general_AB_2, flags, messages);
            extra = make_extra(match_general_AB_2);
        }
        else if (std::smatch match_general_AB_no_isolation; std::regex_search(source_u, match_general_AB_no_isolation, re_flu_name_general_AB_no_isolation)) {
            // std::cerr << "??: " << source_u << ' ' << match_general_AB_no_isolation.format("[1: $1] [host: $2] [loc: $3] [y: $4]") << '\n';
            const auto loc = fix_location(match_general_AB_no_isolation[3].str(), flags & parse_name_f::lookup_location, &messages);
            const std::array fields{match_general_AB_no_isolation[1].str(), match_general_AB_no_isolation[2].str(),
                                    loc.name, match_general_AB_no_isolation[4].str(),
                                    fix_year(match_general_AB_no_isolation[5].str(), &messages)};
            name_data = name_data_t{virus_name_t(::string::join("/", fields)), host_t{fields[1]}, loc.country, loc.continent};
            extra = make_extra(match_general_AB_no_isolation);
        }
        else if (std::smatch match_general_A_subtype; std::regex_search(source_u, match_general_A_subtype, re_flu_name_general_A_subtype)) {
            const auto loc = fix_location(match_general_A_subtype[3].str(), flags & parse_name_f::lookup_location, &messages);
            const std::array fields{match_general_A_subtype[1].str(), match_general_A_subtype[2].str(),
                                    loc.name, match_general_A_subtype[4].str(),
                                    fix_year(match_general_A_subtype[5].str(), &messages)};
            name_data = name_data_t{virus_name_t(::string::join("/", fields)), host_t{fields[1]}, loc.country, loc.continent};
            extra = make_extra(match_general_A_subtype);
        }
        else {
            messages.emplace_back(parse_result_t::message_t::unrecognized, std::string{source});
            throw parse_name_error{};
        }

        Reassortant reassortant;
        if (!extra.empty())
            std::tie(reassortant, extra) = parse_reassortant(extra);

        Passage passage;
        if (!extra.empty()) {
            // const auto orig_extra = extra;
            std::tie(passage, extra) = parse_passage(extra, passage_only::no);
            // if (!passage.empty())
            //     std::cerr << "EXTRA: " << orig_extra << " --> P: " << passage << " E: " << extra << '\n';
        }

        if (!extra.empty() && (flags & parse_name_f::remove_extra_subtype)) {
            if (std::smatch match_flu_a_subtype; std::regex_search(extra, match_flu_a_subtype, re_flu_a_subtype))
                extra = make_extra(match_flu_a_subtype);
        }

        if (!extra.empty()) {
            std::smatch match_extra_remove;
            while (std::regex_search(extra, match_extra_remove, re_extra_remove))
                extra = make_extra(match_extra_remove);
        }

        if (!extra.empty() && !reassortant.empty()) {
            std::smatch match_extra_remove_when_reassortant;
            while (std::regex_search(extra, match_extra_remove_when_reassortant, re_extra_remove_when_reassortant))
                extra = make_extra(match_extra_remove_when_reassortant);
        }

        if (!extra.empty() && std::regex_match(extra, re_extra_symbols))
            extra.clear();

        return {name_data.name, name_data.host, reassortant, passage, extra, name_data.country, name_data.continent, messages};
    }
    catch (parse_name_error&) {
        return {virus_name_t{source}, host_t{}, Reassortant{}, Passage{}, {}, {}, {}, messages};
    }

} // acmacs::virus::v2::parse_name

// ----------------------------------------------------------------------

void acmacs::virus::v2::set_type_subtype(virus_name_t& name, const type_subtype_t& type_subtype)
{
    const std::string_view ts = type_subtype;
    std::string& nam = name.get();
    if (ts.size() > 1 && ts[0] == 'A' && nam.size() > 2 && nam[0] == 'A' && nam[1] == '/')
        nam.replace(0, 1, ts);

} // acmacs::virus::v2::set_type_subtype

// ----------------------------------------------------------------------

name_data_t isolation_with_location(const std::smatch& match, acmacs::virus::v2::parse_name_f flags, std::vector<acmacs::virus::v2::parse_result_t::message_t>& messages)
{
    using namespace acmacs::virus::v2;

    const auto year = fix_year(match[6].str(), &messages);

    if (auto location = fix_location(::string::concat(match[3].str(), ' ', match[4].str()), flags & parse_name_f::lookup_location, nullptr); !location.name.empty()) {
        const auto isolation = match[5].str();
        if (isolation[0] == '-' || isolation[0] == '_' || isolation[0] == ' ')
            return {virus_name_t(::string::join("/", {match[1].str(), match[2].str(), location.name, isolation.substr(1), year})), host_t{match[2].str()}, location.country, location.continent};
        else
            return {virus_name_t(::string::join("/", {match[1].str(), match[2].str(), location.name, isolation, year})), host_t{match[2].str()}, location.country, location.continent};
    }
    else {
        location = fix_location(match[3].str(), flags & parse_name_f::lookup_location, nullptr);
        if (!location.name.empty()) {
            return {virus_name_t(::string::join("/", {match[1].str(), match[2].str(), location.name, ::string::concat(match[4].str(), match[5].str()), year})), host_t{match[2].str()}, location.country, location.continent};
        }
        else if (match[2].length() == 0) {                  // isolation absent?: A/host/location/year
            location = fix_location(::string::concat(match[4].str(), match[5].str()), flags & parse_name_f::lookup_location, &messages);
            return {virus_name_t(::string::join("/", {match[1].str(), match[3].str(), location.name, std::string{"UNKNOWN"}, year})), host_t{match[3].str()}, location.country, location.continent};
        }
        else
            throw parse_name_error{};
    }

} // isolation_with_location

// ----------------------------------------------------------------------

name_data_t general(const std::smatch& match, acmacs::virus::v2::parse_name_f flags, std::vector<acmacs::virus::v2::parse_result_t::message_t>& messages)
{
    using namespace acmacs::virus::v2;

    if (auto host = match[2].str(); !host.empty()) {
        auto location = fix_location(::string::concat(host, ' ', match[3].str()), flags & parse_name_f::lookup_location, nullptr);
        auto isolation = match[4].str();
        if (!location.name.empty()) { // Lyon/CHU -> Lyon CHU
            host.clear();
        }
        else {
            location = fix_location(match[3].str(), flags & parse_name_f::lookup_location, nullptr);
            if (location.name.empty()) {
                location = fix_location(host, flags & parse_name_f::lookup_location, nullptr);
                if (location.name.empty()) {
                    location.name = match[3].str();
                    if (location.name.size() < 3) {
                        messages.emplace_back(parse_result_t::message_t::unrecognized, "not a location: " + location.name);
                        throw parse_name_error{};
                    }
                    else
                        messages.emplace_back(parse_result_t::message_t::location_not_found, location.name);
                }
                else {
                    host.clear();
                    isolation = ::string::join("-", {match[3].str(), isolation});
                }
            }
        }
        return {virus_name_t(::string::join("/", {match[1].str(), host, location.name, isolation, fix_year(match[5].str(), &messages)})), host_t{host}, location.country, location.continent};
    }
    else { //
        const auto location = fix_location(match[3].str(), flags & parse_name_f::lookup_location, &messages);
        return {virus_name_t(::string::join("/", {match[1].str(), host, location.name, match[4].str(), fix_year(match[5].str(), &messages)})), host_t{host}, location.country, location.continent};
    }

} // general

// ----------------------------------------------------------------------

location_t fix_location(std::string source, acmacs::virus::v2::parse_name_f flags, std::vector<acmacs::virus::v2::parse_result_t::message_t>* messages)
{
    if (flags != acmacs::virus::v2::parse_name_f::lookup_location)
        return {source, "", ""};
    if (source.size() < 3) {
        if (messages) {
            messages->emplace_back(acmacs::virus::v2::parse_result_t::message_t::unrecognized, "not a location: " + source);
            throw parse_name_error{};
        }
        else
            return {};
    }

    try {
        const auto& locdb = get_locdb();
        const auto loc = locdb.find(::string::strip(source));
        // fmt::print(stderr, "DEBUG: fix_location {} -> {} -- {} -- {}\n", source, loc.name, loc.replacement, loc.location_name);
        return {loc.name, std::string(loc.country()), std::string(locdb.continent_of_country(loc.country()))};
    }
    catch (LocationNotFound& /*err*/) {
        // std::cerr << "LocationNotFound: \"" << source << "\"\n";
        if (messages) {
            if (std::regex_search(source, re_location_stop_list)) {
                messages->emplace_back(acmacs::virus::v2::parse_result_t::message_t::unrecognized, "not a location: " + source);
                throw parse_name_error{};
            }
            else {
                messages->emplace_back(acmacs::virus::v2::parse_result_t::message_t::location_not_found, source);
                return {source, "", ""};
            }
        }
        else
            return {};
    }

} // fix_location

// ----------------------------------------------------------------------

std::string fix_year(std::string source, std::vector<acmacs::virus::v2::parse_result_t::message_t>* messages)
{
#include "acmacs-base/global-constructors-push.hh"
    static const auto current_year = date::current_year();
    static const auto current_year_2 = current_year % 100;
#include "acmacs-base/diagnostics-pop.hh"

    if (const auto year = string::from_chars<size_t>(source); year < 10) {
        return "200" + std::to_string(year);
    }
    else if (year < 18) {
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
            messages->emplace_back(acmacs::virus::v2::parse_result_t::message_t::invalid_year, source);
        throw parse_name_error{};
        // return source;
    }
    else
        return std::to_string(year);

} // fix_year

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::virus::v2::year(const virus_name_t& name)
{
    if (name->size() > 4) {
        std::array<char, 5> data{0, 0, 0, 0, 0}; // avoid acceing beyond the name by strtoul
        std::copy_n(name->data() + name->size() - 4, 4, std::begin(data));
        char* end;
        const auto yr = std::strtoul(data.data(), &end, 10);
        if (end == (data.data() + 4) && yr > 0)
            return yr;
    }
    return std::nullopt;

} // acmacs::virus::v2::year

// ----------------------------------------------------------------------

std::string_view acmacs::virus::v2::host(const virus_name_t& name)
{
    if (const auto fields = acmacs::string::split(*name, "/"); fields.size() == 5)
        return fields[1];
    else
        return {};

} // acmacs::virus::v2::host

// ----------------------------------------------------------------------

std::string_view acmacs::virus::v2::location(const virus_name_t& name)
{
    const auto fields = acmacs::string::split(*name, "/");
    return fields[fields.size() - 3];

} // acmacs::virus::v2::location

// ----------------------------------------------------------------------

std::string_view acmacs::virus::v2::isolation(const virus_name_t& name)
{
    const auto fields = acmacs::string::split(*name, "/");
    return fields[fields.size() - 2];

} // acmacs::virus::v2::isolation

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
