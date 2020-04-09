#include "acmacs-base/regex.hh"
#include "acmacs-base/date.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name-parse.hh"

// ----------------------------------------------------------------------

#define SRE_AB "([AB])"
#define SRE_HOST "(?:\\s*([A-Z0-9 \\-_]+)\\s*/)?" // digits in HOST: A/CLOACAL SWAB_ECE1/BELGIUM/17330-45/2013
// #define SRE_LOC "([A-Z \\-\\.,'&_]{2,})"  // no digits!, otherwise possibility to omit / after LOC does not work
#define SRE_LOC "([A-Z0-9 \\-\\.,;'&_\\?]{2,})" // nbci has ; in location name: "A/MEXICAN DUCK/ESTADODEMEXICO; LERMA/HA_UIFMVZ377/2016"
#define SRE_LOC_NO_DIGITS "([A-Z \\-\\.,'&_]{2,})"
#define SRE_ISOLATION "\\s*0*([^/]+)\\s*"
#define SRE_ISOLATION_WITH_LOC "\\s*([A-Z]{3,})([^A-Z/][^/]*)\\s*" // isolation without leading / cannot start with a letter
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

constexpr const char* sre_flu_name_general_AB_no_isolation2 = // really no isolation
        "\\b"
        SRE_AB "/"                      // type \1
        SRE_HOST                        // host \2
        SRE_LOC_NO_DIGITS               // location \3
        "/"                             // no isolation
        "\\s*(\\d+)"                    // year \4 - any number of digits
        SRE_LOOKAHEAD_EXCEPT            // neither digit nor letter nor / nor - nor _ nor .
        ;

// A/Tambov/CRIE/309/2019
constexpr const char* sre_flu_name_general_AB_noisy_infix = // or no slash after location
        "\\b"
        SRE_AB "/"                      // type \1
        "()"                            // NO host \2
        SRE_LOC_NO_DIGITS "/"           // location \3
        "(?:CRIE)" "/"                  // infix to remove
        SRE_ISOLATION "/"               // isolation \4
        "\\s*(\\d+)"                    // year \5 - any number of digits
        SRE_LOOKAHEAD_EXCEPT            // neither digit nor letter nor / nor - nor _ nor .
        ;

// A/CHICKEN/VIETNAM/QUANGBINH/BD1113/2017
constexpr const char* sre_flu_name_general_AB_location_2_fields =
        "\\b"
        SRE_AB "/"                      // type \1
        SRE_HOST                        // host \2
        SRE_LOC_NO_DIGITS "/"           // location \3
        SRE_LOC_NO_DIGITS "/"           // location \4
        SRE_ISOLATION  "/+"             // isolation \5  - without leading 0, mutiple / at the end (found in gisaid)
        "\\s*(\\d+)"                    // year \6 - any number of digits
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

constexpr const char* sre_seq_id =
        "\\b(A?H[1-9][0-9]?(?:N[1-9][0-9]?)?|B)/"    // AH3N2 or B \1
        SRE_HOST                                     // host \2
        SRE_LOC "/"                                  // location \3
        SRE_ISOLATION "/"                            // isolation \4
        "(\\d\\d\\d\\d)"                             // year \5
        "(?:_|$)"                                    // underscore or end
        ;


struct parse_name_error : public std::exception {};

struct location_t
{
    location_t() = default;
    location_t(std::string_view a_name, std::string_view a_country, std::string_view a_continent) : name{a_name}, country{a_country}, continent{a_continent} {}
    std::string name;
    std::string country;
    std::string continent;
};

struct name_data_t
{
    acmacs::virus::v2::name_t name;
    acmacs::virus::v2::host_t host;
    std::string country{};
    std::string continent{};
};

static bool check_location(std::string_view source, acmacs::virus::v2::parse_name_f flags);
static bool check_host(std::string_view host);
// static int find_location(std::string_view field0, std::string_view field1, std::string_view field2, acmacs::virus::v2::parse_name_f flags);
static location_t fix_location(std::string_view source, acmacs::virus::v2::parse_name_f flags, std::vector<acmacs::virus::v2::name::parsing_message_t>* messages);
static std::string fix_isolation(std::string_view source);
static std::string fix_year(std::string_view source, std::string_view name, std::vector<acmacs::virus::v2::name::parsing_message_t>* messages);
static name_data_t isolation_with_location(const std::smatch& match, acmacs::virus::v2::parse_name_f flags, std::string_view name, std::vector<acmacs::virus::v2::name::parsing_message_t>& messages);
static name_data_t general(const std::smatch& match, acmacs::virus::v2::parse_name_f flags, std::string_view name, std::vector<acmacs::virus::v2::name::parsing_message_t>& messages);
static name_data_t location_not_found(const std::smatch& match, std::string_view name, std::vector<acmacs::virus::v2::name::parsing_message_t>& messages);

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"

static const std::regex re_flu_name_general_AB_isolation_with_location{sre_flu_name_general_AB_isolation_with_location};
static const std::regex re_flu_name_general_AB_1{sre_flu_name_general_AB_1};
static const std::regex re_flu_name_general_AB_2{sre_flu_name_general_AB_2};
static const std::regex re_flu_name_general_AB_no_isolation{sre_flu_name_general_AB_no_isolation};
static const std::regex re_flu_name_general_AB_no_isolation2{sre_flu_name_general_AB_no_isolation2};
static const std::regex re_flu_name_general_A_subtype{sre_flu_name_general_A_subtype};
static const std::regex re_flu_name_general_AB_location_2_fields{sre_flu_name_general_AB_location_2_fields};
static const std::regex re_seq_id{sre_seq_id};
static const std::regex re_flu_name_general_AB_noisy_infix{sre_flu_name_general_AB_noisy_infix};
static const std::regex re_extra_remove{"\\b(?:NEW)\\b"}; // NEW
static const std::regex re_extra_remove_when_reassortant{"\\b(?:HY)\\b"}; // HY
static const std::regex re_extra_symbols{"^[\\(\\)_\\-\\s,\\.]+$"};
static const std::regex re_flu_a_subtype{"\\(H[1-9][0-9]?(?:N[1-9][0-9]?V?)?\\)"};
static const std::regex re_location_stop_list("(?:REASSORTANT|DOMESTIC|EQUINE|SWINE|-$)", std::regex::icase); // if location matches, throw parse_name_error (i.e. name unrecognized)

#include "acmacs-base/diagnostics-pop.hh"

// ----------------------------------------------------------------------

acmacs::virus::v2::parse_result_t acmacs::virus::v2::parse_name(std::string_view source, parse_name_f flags, acmacs::debug dbg)
{

    const auto make_extra = [](const std::smatch& match) { return string::join(" ", {::string::strip(match.prefix().str()), ::string::strip(match.suffix().str())}); };

    name_data_t name_data{name_t{}, host_t{}, std::string{}, std::string{}};
    std::string extra;
    std::vector<acmacs::virus::v2::name::parsing_message_t> messages;

    const std::string source_u = ::string::upper(source);

    try {
        if (std::smatch match_general_AB_noisy_infix; std::regex_search(source_u, match_general_AB_noisy_infix, re_flu_name_general_AB_noisy_infix)) {
            AD_DEBUG_IF(dbg, "match_general_AB_noisy_infix {}", match_general_AB_noisy_infix[0].str());
            name_data = general(match_general_AB_noisy_infix, flags, source, messages);
            extra = make_extra(match_general_AB_noisy_infix);
        }
        else if (std::smatch match_general_AB_isolation_with_location; std::regex_search(source_u, match_general_AB_isolation_with_location, re_flu_name_general_AB_isolation_with_location)) {
            AD_DEBUG_IF(dbg, "match_general_AB_isolation_with_location {}", match_general_AB_isolation_with_location);
            name_data = isolation_with_location(match_general_AB_isolation_with_location, flags, source, messages);
            extra = make_extra(match_general_AB_isolation_with_location);
        }
        else if (std::smatch match_general_AB_1;
                 std::regex_search(source_u, match_general_AB_1, re_flu_name_general_AB_1) && (check_location(match_general_AB_1.str(2), flags) || check_location(match_general_AB_1.str(3), flags))) {
            AD_DEBUG_IF(dbg, "match_general_AB_1 {}", match_general_AB_1);
            name_data = general(match_general_AB_1, flags, source, messages);
            extra = make_extra(match_general_AB_1);
        }
        else if (std::smatch match_general_AB_2;
                 std::regex_search(source_u, match_general_AB_2, re_flu_name_general_AB_2) && (check_location(match_general_AB_2.str(2), flags) || check_location(match_general_AB_2.str(3), flags))) {
            AD_DEBUG_IF(dbg, "match_general_AB_2 {}", match_general_AB_2);
            name_data = general(match_general_AB_2, flags, source, messages);
            extra = make_extra(match_general_AB_2);
        }
        else if (std::smatch match_general_AB_no_isolation;
                 std::regex_search(source_u, match_general_AB_no_isolation, re_flu_name_general_AB_no_isolation) && check_location(match_general_AB_no_isolation.str(3), flags)) {
            AD_DEBUG_IF(dbg, "match_general_AB_no_isolation {}", match_general_AB_no_isolation);
            const auto loc = fix_location(match_general_AB_no_isolation[3].str(), flags & parse_name_f::lookup_location, &messages);
            const std::array fields{match_general_AB_no_isolation[1].str(), match_general_AB_no_isolation[2].str(), loc.name, match_general_AB_no_isolation[4].str(),
                                    fix_year(match_general_AB_no_isolation[5].str(), source, &messages)};
            name_data = name_data_t{name_t(string::join("/", fields)), host_t{fields[1]}, loc.country, loc.continent};
            extra = make_extra(match_general_AB_no_isolation);
        }
        else if (std::smatch match_general_AB_no_isolation2;
                 std::regex_search(source_u, match_general_AB_no_isolation2, re_flu_name_general_AB_no_isolation2) && check_location(match_general_AB_no_isolation2.str(3), flags)) {
            AD_DEBUG_IF(dbg, "match_general_AB_no_isolation2 {}", match_general_AB_no_isolation2);
            const auto loc = fix_location(match_general_AB_no_isolation2[3].str(), flags & parse_name_f::lookup_location, &messages);
            const std::array fields{match_general_AB_no_isolation2[1].str(), match_general_AB_no_isolation2[2].str(), loc.name, std::string{"UNKNOWN"},
                                    fix_year(match_general_AB_no_isolation2[4].str(), source, &messages)};
            name_data = name_data_t{name_t(string::join("/", fields)), host_t{fields[1]}, loc.country, loc.continent};
            extra = make_extra(match_general_AB_no_isolation2);
        }
        else if (std::smatch match_general_A_subtype; std::regex_search(source_u, match_general_A_subtype, re_flu_name_general_A_subtype)) {
            AD_DEBUG_IF(dbg, "match_general_A_subtype {}", match_general_A_subtype);
            const auto loc = fix_location(match_general_A_subtype[3].str(), flags & parse_name_f::lookup_location, &messages);
            const std::array fields{match_general_A_subtype[1].str(), match_general_A_subtype[2].str(), loc.name, match_general_A_subtype[4].str(),
                                    fix_year(match_general_A_subtype[5].str(), source, &messages)};
            name_data = name_data_t{name_t(string::join("/", fields)), host_t{fields[1]}, loc.country, loc.continent};
            extra = make_extra(match_general_A_subtype);
        }
        else if (std::smatch match_general_AB_location_2_fields;
                 std::regex_search(source_u, match_general_AB_location_2_fields, re_flu_name_general_AB_location_2_fields) && check_location(fmt::format("{} {}", match_general_AB_location_2_fields.str(3), match_general_AB_location_2_fields.str(4)), flags)) {
            AD_DEBUG_IF(dbg, "match_general_AB_location_2_fields {}", match_general_AB_location_2_fields);
            const auto loc = fix_location(fmt::format("{} {}", match_general_AB_location_2_fields.str(3), match_general_AB_location_2_fields.str(4)), flags & parse_name_f::lookup_location, &messages);
            const std::array fields{match_general_AB_location_2_fields[1].str(), match_general_AB_location_2_fields[2].str(), loc.name, fix_isolation(match_general_AB_location_2_fields[5].str()),
                                    fix_year(match_general_AB_location_2_fields[6].str(), source, &messages)};
            name_data = name_data_t{name_t(string::join("/", fields)), host_t{fields[1]}, loc.country, loc.continent};
            extra = make_extra(match_general_AB_location_2_fields);
        }
        else if (std::smatch match_seq_id; std::regex_search(source_u, match_seq_id, re_seq_id)) {
            AD_DEBUG_IF(dbg, "seq_id matched {}", match_seq_id);
            const auto loc = fix_location(match_seq_id[3].str(), flags & parse_name_f::lookup_location, &messages);
            std::array fields{match_seq_id[1].str(), match_seq_id[2].str(), loc.name, match_seq_id[4].str(), fix_year(match_seq_id[5].str(), source, &messages)};
            if (fields[0].size() >= 5 && fields[0].front() == 'A')
                fields[0] = fmt::format("{}({})", fields[0].front(), fields[0].substr(1));
            name_data = name_data_t{name_t(string::join("/", fields)), host_t{fields[1]}, loc.country, loc.continent};
            extra = ::string::replace(make_extra(match_seq_id), '_', ' ');
        }
        else {
            const auto make_result = [&](const std::smatch& match) {
                name_data = location_not_found(match, source, messages);
                extra = make_extra(match);
            };
            if (match_general_AB_1.size() == 6)
                make_result(match_general_AB_1);
            else if (match_general_AB_2.size() == 6)
                make_result(match_general_AB_2);
            else if (match_general_AB_no_isolation.size() == 6)
                make_result(match_general_AB_no_isolation);
            else if (match_general_AB_no_isolation2.size() == 5)
                make_result(match_general_AB_no_isolation2);
            else {
                AD_WARNING("no virus name parsing regex matched: \"{}\"", source_u);
                messages.emplace_back(name::parsing_message_t::unrecognized, std::string{source});
                throw parse_name_error{};
            }
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

        if (name_data.host.size() >= 4 && name_data.host->substr(0, 4) == "TEST")
            messages.emplace_back(acmacs::virus::v2::name::parsing_message_t::invalid_host, *name_data.host);
        return {name_data.name, name_data.host, reassortant, passage, extra, name_data.country, name_data.continent, messages};
    }
    catch (parse_name_error&) {
        return {name_t{source}, host_t{}, Reassortant{}, Passage{}, {}, {}, {}, messages};
    }

} // acmacs::virus::v2::parse_name

// ----------------------------------------------------------------------

name_data_t isolation_with_location(const std::smatch& match, acmacs::virus::v2::parse_name_f flags, std::string_view name, std::vector<acmacs::virus::v2::name::parsing_message_t>& messages)
{
    using namespace acmacs::virus::v2;

    const auto year = fix_year(match[6].str(), name, &messages);

    const auto any_digit = [](const auto& src) -> bool { return std::any_of(std::begin(src), std::end(src), &isdigit); };

    if (auto location = fix_location(::string::concat(match[3].str(), ' ', match[4].str()), flags & parse_name_f::lookup_location, nullptr); !location.name.empty()) {
        const auto isolation = match[5].str();
        if (isolation[0] == '-' || isolation[0] == '_' || isolation[0] == ' ')
            return {name_t(acmacs::string::join("/", match[1].str(), match[2].str(), location.name, isolation.substr(1), year)), host_t{match[2].str()}, location.country, location.continent};
        else
            return {name_t(acmacs::string::join("/", match[1].str(), match[2].str(), location.name, isolation, year)), host_t{match[2].str()}, location.country, location.continent};
    }
    else {
        location = fix_location(match[3].str(), flags & parse_name_f::lookup_location, nullptr);
        // fmt::print(stderr, "DEBUG: isolation_with_location {} -> 1:{} 2:{} 3:{} 4:{} 5:{} 6:{}: location: \"{}\" -> \"{}\"\n", match.str(0), match.str(1), match.str(2), match.str(3), match.str(4), match.str(5), match.str(6), match.str(3), location.name);
        if (!location.name.empty()) {
            return {name_t(acmacs::string::join("/", match[1].str(), match[2].str(), location.name, ::string::concat(match[4].str(), match[5].str()), year)), host_t{match[2].str()}, location.country, location.continent};
        }
        else if (match.length(2) == 0 && any_digit(match.str(5))) { // location match.str(3) not found in locdb
            messages.emplace_back(acmacs::virus::v2::name::parsing_message_t::location_not_found, match.str(3));
            return {name_t{fmt::format("{}/{}/{}{}/{}", match.str(1), match.str(3), match.str(4), match.str(5), year)}, host_t{match.str(2)}, "", ""};
        }
        else if (match[2].length() == 0) {                  // isolation absent?: A/host/location/year
            // fmt::print(stderr, "DEBUG: isolation_with_location {} -> 1:{} 2:{} 3:{} 4:{} 5:{} 6:{}:\n", match.str(0), match.str(1), match.str(2), match.str(3), match.str(4), match.str(5), match.str(6));
            location = fix_location(::string::concat(match[4].str(), match[5].str()), flags & parse_name_f::lookup_location, &messages);
            messages.emplace_back(acmacs::virus::v2::name::parsing_message_t::isolation_absent, match[0].str());
            return {name_t(acmacs::string::join("/", match[1].str(), match[3].str(), location.name, std::string{"UNKNOWN"}, year)), host_t{match[3].str()}, location.country, location.continent};
        }
        else {
            messages.emplace_back(acmacs::virus::v2::name::parsing_message_t::location_not_found, match.str(3));
            throw parse_name_error{};
        }
    }

} // isolation_with_location

// ----------------------------------------------------------------------

name_data_t general(const std::smatch& match, acmacs::virus::v2::parse_name_f flags, std::string_view name, std::vector<acmacs::virus::v2::name::parsing_message_t>& messages)
{
    using namespace acmacs::virus::v2;

    // AD_DEBUG("general {}", match);
    if (auto host = match[2].str(); !host.empty()) {
        auto location = fix_location(::string::concat(host, ' ', match[3].str()), flags & parse_name_f::lookup_location, nullptr);
        auto isolation = fix_isolation(match[4].str());
        if (!location.name.empty()) { // Lyon/CHU -> Lyon CHU
            host.clear();
        }
        else {
            location = fix_location(match[3].str(), flags & parse_name_f::lookup_location, nullptr);
            if (location.name.empty()) {
                location = fix_location(host, flags & parse_name_f::lookup_location, nullptr);
                if (location.name.empty()) {
                    location.name = match[3].str();
                    messages.emplace_back(name::parsing_message_t::location_not_found, location.name);
                    // AD_DEBUG("general location not found {}", location.name);
                    if (location.name.size() < 3)
                        throw parse_name_error{};
                }
                else {
                    host.clear();
                    isolation = acmacs::string::join("-", match[3].str(), isolation);
                }
            }
        }
        return {name_t(acmacs::string::join("/", match[1].str(), host, location.name, isolation, fix_year(match[5].str(), name, &messages))), host_t{host}, location.country, location.continent};
    }
    else { //
        const auto location = fix_location(match[3].str(), flags & parse_name_f::lookup_location, &messages);
        return {name_t(acmacs::string::join("/", match[1].str(), host, location.name, match[4].str(), fix_year(match[5].str(), name, &messages))), host_t{host}, location.country, location.continent};
    }

} // general

// ----------------------------------------------------------------------

name_data_t location_not_found(const std::smatch& match, std::string_view name, std::vector<acmacs::virus::v2::name::parsing_message_t>& messages)
{
    // AD_DEBUG("location_not_found {}", match);
    messages.emplace_back(acmacs::virus::name::parsing_message_t::location_not_found, match[3].str());
    if (match.size() == 6)
        return {acmacs::virus::name_t(acmacs::string::join("/", match[1].str(), match[2].str(), match[3].str(), fix_isolation(match[4].str()), fix_year(match[5].str(), name, &messages))), acmacs::virus::host_t{match[2].str()}};
    else
        return {acmacs::virus::name_t(acmacs::string::join("/", match[1].str(), match[2].str(), match[3].str(), std::string{"UNKNOWN"}, fix_year(match[4].str(), name, &messages))), acmacs::virus::host_t{match[2].str()}};

} // location_not_found

// ----------------------------------------------------------------------

bool check_host(std::string_view host)
{
    using namespace std::string_view_literals;

    static std::array hosts{
        "TURKEY"sv,
        "DUCK"sv,
        "MALLARD"sv,
        "CHICKEN"sv,
        "GOOSE"sv,
        "PEACOCK"sv,
        "CAT"sv,
        "DOMESTIC"sv,
        "EQUINE"sv,
        "SWINE"sv,
    };

    return std::find(std::begin(hosts), std::end(hosts), host) != std::end(hosts);

} // check_host

// ----------------------------------------------------------------------

bool check_location(std::string_view source, acmacs::virus::v2::parse_name_f flags)
{
    const auto location = fix_location(source, flags & acmacs::virus::parse_name_f::lookup_location, nullptr);
    return !location.name.empty();

} // check_location

// ----------------------------------------------------------------------

// // returns which field is really location
// int find_location(std::string_view field0, std::string_view field1, std::string_view field2, acmacs::virus::v2::parse_name_f flags)
// {
//     if (check_host(field0) && check_location(field1, flags))
//         return 1;
//     if (check_location(field0, flags) && !check_location(field1, flags))
//         return 0;
//     if (check_location(field2, flags))
//         return 3;
//     return -1;

// } // find_location

// ----------------------------------------------------------------------

location_t fix_location(std::string_view source, acmacs::virus::v2::parse_name_f flags, std::vector<acmacs::virus::v2::name::parsing_message_t>* messages)
{
    if (flags != acmacs::virus::v2::parse_name_f::lookup_location)
        return {source, "", ""};
    if (source.size() < 3) {
        if (messages) {
            messages->emplace_back(acmacs::virus::v2::name::parsing_message_t::location_not_found, source);
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
            messages->emplace_back(acmacs::virus::v2::name::parsing_message_t::location_not_found, source);
            if (std::regex_search(std::begin(source), std::end(source), re_location_stop_list))
                throw parse_name_error{};
            return {source, "", ""};
        }
        else
            return {};
    }

} // fix_location

// ----------------------------------------------------------------------

std::string fix_isolation(std::string_view source)
{
    using namespace ::string;
    auto result = collapse_spaces(source); // perhaps replacing space with - is not a good idea -- replace_spaces(collapse_spaces(source), '-');
    if (result.size() > 3 && result.substr(result.size() - 3) == "_HA") // isolation ending with _HA means HA segment in sequences from ncbi
        result.erase(result.size() - 3);
    return result;

} // fix_isolation

// ----------------------------------------------------------------------

std::string fix_year(std::string_view source, std::string_view name, std::vector<acmacs::virus::v2::name::parsing_message_t>* messages)
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
            messages->emplace_back(acmacs::virus::v2::name::parsing_message_t::invalid_year, source);
        throw parse_name_error{};
        // return source;
    }
    else
        return std::to_string(year);

} // fix_year

// ----------------------------------------------------------------------



// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
