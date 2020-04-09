#include "acmacs-base/string-split.hh"
#include "acmacs-base/string-join.hh"
#include "acmacs-base/date.hh"
#include "acmacs-base/regex.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name-normalize.hh"

// ----------------------------------------------------------------------

std::string acmacs::virus::name::parsed_fields_t::name() const
{
    if (good())
        return string::join("/", subtype, host, location, isolation, year);
    else
        return raw;

} // acmacs::virus::name::parsed_fields_t::name

// ----------------------------------------------------------------------

namespace acmacs::virus::inline v2::name
{
    struct try_fields_t
    {
        std::string_view subtype{};
        std::string_view host{};
        std::string_view location{};
        std::string_view isolation{};
        std::string_view year_rest{};
    };

    struct location_data_t
    {
        std::string name;
        std::string country;
        std::string continent;
    };

    struct location_part_t
    {
        size_t part_no;
        location_data_t location;
    };

    using location_parts_t = std::vector<location_part_t>;

    static void no_location_parts(std::string_view name, const std::vector<std::string_view>& parts, parsed_fields_t& output);
    static void one_location_part(std::string_view name, const std::vector<std::string_view>& parts, const location_part_t& location_part, parsed_fields_t& output);
    static void two_location_parts(std::string_view name, std::vector<std::string_view>& parts, const location_parts_t& location_parts, parsed_fields_t& output);

    static bool check(try_fields_t&& input, parsed_fields_t& output, std::string_view name);

    static bool check_subtype(std::string_view source, parsed_fields_t* output = nullptr, std::string_view name={});
    static bool check_host(std::string_view source, parsed_fields_t* output = nullptr, std::string_view name={});
    static bool check_location(std::string_view source, parsed_fields_t* output = nullptr, std::string_view name={});
    static bool check_isolation(std::string_view source, parsed_fields_t* output = nullptr, std::string_view name={});
    static bool check_year(std::string_view source, parsed_fields_t* output = nullptr, std::string_view name={});
    static bool host_confusing_with_location(std::string_view source);
    // static void host_location_fix(try_fields_t& input, parsed_fields_t& output, std::string_view name);
    static location_parts_t find_location_parts(const std::vector<std::string_view>& parts, parsed_fields_t& output);
    static std::string check_reassortant_in_front(std::string_view source, parsed_fields_t& output);

    static std::optional<location_data_t> location_lookup(std::string_view source);

    // quick check to avoid calling slow (many regexp based check_reassortant_in_front)
    inline bool possible_reassortant_in_front(std::string_view source)
    {
        using namespace std::string_view_literals;
        if (source.size() > 6) {
            switch (source.front()) {
                case 'I': // IVR
                case 'N': // NIB, NYMC
                case 'R': // RG
                case 'X': // X-327
                    return true;
              case 'B':
                  switch (source[1]) {
                    case 'V':   // BVR
                    case 'X':   // BX
                        return true;
                    case '/':
                    if (source.substr(2, 5) == "REASS"sv) // B/REASSORTANT/
                        return true;
                    break;
                  }
                    break;
                case 'A':
                    if (source.substr(1, 6) == "/REASS"sv) // A/REASSORTANT/
                        return true;
                    break;
            }
        }
        return false;
    }

    inline void add_message(parsed_fields_t* output, const char* key, std::string_view value, std::string_view source)
    {
        if (output)
            output->messages.emplace_back(key, value, source);
    }

    // ----------------------------------------------------------------------

} // namespace acmacs::virus::inline v2::name

template <> struct fmt::formatter<acmacs::virus::name::location_parts_t> : public fmt::formatter<acmacs::fmt_default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::name::location_parts_t& parts, FormatContext& ctx)
    {
        format_to(ctx.out(), "{{");
        for (const auto& part : parts)
            format_to(ctx.out(), " {}", part.part_no);
        return format_to(ctx.out(), "}}");
    }
};

// ----------------------------------------------------------------------

acmacs::virus::name::parsed_fields_t acmacs::virus::name::parse(std::string_view source)
{
    parsed_fields_t output{.raw = std::string{source}};
    if (source.empty()) {
        AD_WARNING("empty source");
        return output;
    }

    std::string upcased = ::string::upper(source);
    if (possible_reassortant_in_front(upcased))
        upcased = check_reassortant_in_front(upcased, output);
    auto parts = acmacs::string::split(upcased, "/", acmacs::string::Split::StripKeepEmpty);
    const auto location_parts = find_location_parts(parts, output);
    switch (location_parts.size()) {
      case 0:
          no_location_parts(source, parts, output);
          break;
      case 1:
          one_location_part(source, parts, location_parts.front(), output);
          break;
      case 2:
          two_location_parts(source, parts, location_parts, output);
          break;
      default:
          output.messages.emplace_back("multiple-location", fmt::format("{}", location_parts), source);
          break;
    }

    if (!output.good() && output.messages.empty())
        output.messages.emplace_back(parsing_message_t::unrecognized, source);

    return output;

} // acmacs::virus::name::parse

// ----------------------------------------------------------------------

void acmacs::virus::name::no_location_parts(std::string_view name, const std::vector<std::string_view>& parts, parsed_fields_t& output)
{
    output.messages.emplace_back(parsing_message_t::location_field_not_found, name);

} // acmacs::virus::name::no_location_parts

// ----------------------------------------------------------------------

void acmacs::virus::name::one_location_part(std::string_view name, const std::vector<std::string_view>& parts, const location_part_t& location_part, parsed_fields_t& output)
{
    switch (location_part.part_no) {
        case 0:
            if (parts.size() == 3)
                check(try_fields_t{.location = parts[0], .isolation = parts[1], .year_rest = parts[2]}, output, name);
            break;
        case 1:
            if (parts.size() == 4)
                check(try_fields_t{.subtype = parts[0], .location = parts[1], .isolation = parts[2], .year_rest = parts[3]}, output, name);
            break;
        case 2:
            if (parts.size() == 5)
                check(try_fields_t{.subtype = parts[0], .host = parts[1], .location = parts[2], .isolation = parts[3], .year_rest = parts[4]}, output, name);
            break;
        default:
            output.messages.emplace_back("unexpected-location-part", fmt::format("{}", location_part.part_no), name);
            break;
    }

} // acmacs::virus::name::one_location_part

// ----------------------------------------------------------------------

void acmacs::virus::name::two_location_parts(std::string_view name, std::vector<std::string_view>& parts, const location_parts_t& location_parts, parsed_fields_t& output)
{
    if (host_confusing_with_location(parts[location_parts.front().part_no])) {
        one_location_part(name, parts, location_parts[1], output);
    }
    else if (location_parts[0].location.name == location_parts[1].location.country) { // A/India/Delhi/DB106/2009 -> A/Delhi/DB106/2009
        parts.erase(std::next(parts.begin(), static_cast<ssize_t>(location_parts[0].part_no)));
        one_location_part(name, parts, location_parts[1], output);
    }
    else if (location_parts[0].location.country == location_parts[1].location.name) { // A/Cologne/Germany/01/2009 -> A/Cologne/01/2009
        parts.erase(std::next(parts.begin(), static_cast<ssize_t>(location_parts[1].part_no)));
        one_location_part(name, parts, location_parts[0], output);
    }
    else if (location_parts[0].location.country == location_parts[1].location.country) { // "B/Mount_Lebanon/Ain_W_zein/3/2019" -> "B/Mount Lebanon Ain W zein/3/2019"
        switch (location_parts[0].part_no) {
          case 1:
              if (parts[0].size() == 1 && parts.size() == 5)
                  check(try_fields_t{.subtype=parts[0], .location = fmt::format("{} {}", location_parts[0].location.name, location_parts[1].location.name), .isolation = parts[3], .year_rest = parts[4]}, output, name);
              break;
        }
    }
    else
        output.messages.emplace_back("double-location", fmt::format("{}", location_parts), name);

} // acmacs::virus::name::two_location_parts

// ----------------------------------------------------------------------

bool acmacs::virus::name::check(try_fields_t&& input, parsed_fields_t& output, std::string_view name)
{
    output = parsed_fields_t{};
    // messages.clear();
    if (check_location(input.location, &output, name)) {
        if (check_year(input.year_rest, &output, name) && check_subtype(input.subtype, &output, name) && check_host(input.host, &output, name) &&
            check_isolation(input.isolation, &output, name))
            return true;
    }
    return false;

} // acmacs::virus::name::check

// ----------------------------------------------------------------------

// bool acmacs::virus::name::check(try_fields_t&& input, parsed_fields_t& output, parsing_messages_t& messages, std::string_view name)
// {
//     output = parsed_fields_t{};
//     // messages.clear();
//     if (check_location(input.location, &output, &messages, name)) {
//         host_location_fix(input, output, name);
//         if (check_year(input.year_rest, &output, &messages, name) && check_subtype(input.subtype, &output, &messages, name) && check_host(input.host, &output, &messages, name) &&
//             check_isolation(input.isolation, &output, &messages, name))
//             return true;
//     }
//     return false;

// } // acmacs::virus::name::check

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_subtype(std::string_view source, parsed_fields_t* output, std::string_view name)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::regex re_a{"^A(?:"
                                 "\\((H\\d{1,2}(?:N\\d{1,2})?)\\)" // $1
                                 "|"
                                 "(H\\d{1,2}(?:N\\d{1,2})?)" // $2
                                 ")$"};
#include "acmacs-base/diagnostics-pop.hh"

    try {
        switch (source.size()) {
            case 0:
                break;
            case 1:
                switch (source.front()) {
                    case 'A':
                    case 'B':
                        if (output)
                            output->subtype = type_subtype_t{source};
                        break;
                    default:
                        throw std::exception{};
                }
                break;
            default:
                if (std::cmatch mch; std::regex_match(std::begin(source), std::end(source), mch, re_a)) {
                    if (mch.length(1))
                        output->subtype = type_subtype_t{fmt::format("A({})", mch.str(1))};
                    else if (mch.length(2))
                        output->subtype = type_subtype_t{fmt::format("A({})", mch.str(2))};
                    else
                        throw std::exception{};
                }
                else
                    throw std::exception{};
                break;
        }
        return true;
    }
    catch (std::exception&) {
        add_message(output, parsing_message_t::invalid_subtype, source, name);
        return false;
    }

} // acmacs::virus::name::check_subtype

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_host(std::string_view source, parsed_fields_t* output, std::string_view /*name*/)
{
    if (output)
        output->host = host_t{source};
    return true;

} // acmacs::virus::name::check_host

// ----------------------------------------------------------------------

std::optional<acmacs::virus::name::location_data_t> acmacs::virus::name::location_lookup(std::string_view source)
{
    using namespace std::string_view_literals;

    if (source == "UNKNOWN"sv)
        return std::nullopt;

    const auto look = [](std::string_view look_for) {
        const auto& locdb = get_locdb();
        const auto loc = locdb.find(look_for);
        const auto country = loc.country();
        return location_data_t{.name = loc.name, .country = std::string{country}, .continent = std::string{locdb.continent_of_country(country)}};
    };

    try {
        return look(source);
    }
    catch (LocationNotFound& /*err*/) {
    }

    using pp = std::pair<std::string_view, std::string_view>;
    static std::array common_abbreviations{
        pp{"UK"sv, "UNITED KINGDOM"sv},
        pp{"NY"sv, "NEW YORK"sv},
    };

    try {
        for (const auto& [e1, e2] : common_abbreviations) {
            if (e1 == source)
                return look(e2);
        }
    }
    catch (LocationNotFound& /*err*/) {
    }

    return std::nullopt;

} // acmacs::virus::name::location_lookup

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_location(std::string_view source, parsed_fields_t* output, std::string_view name)
{
    if (const auto loc = location_lookup(source); loc.has_value()) {
        if (output) {
            output->location = loc->name;
            output->country = loc->country;
            output->continent = loc->continent;
        }
        return true;
    }
    else {
        add_message(output, parsing_message_t::location_not_found, source, name);
        return false;
    }

} // acmacs::virus::name::check_location

// ----------------------------------------------------------------------

acmacs::virus::name::location_parts_t acmacs::virus::name::find_location_parts(const std::vector<std::string_view>& parts, parsed_fields_t& output)
{
    location_parts_t location_parts;
    for (size_t part_no = 0; part_no < parts.size(); ++part_no) {
        if (const auto loc = location_lookup(parts[part_no]); loc.has_value())
            location_parts.push_back({part_no, *loc});
    }
    return location_parts;

} // acmacs::virus::name::find_location_parts

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_isolation(std::string_view source, parsed_fields_t* output, std::string_view name)
{
    if (const auto skip_zeros = source.find_first_not_of('0'); skip_zeros != std::string_view::npos)
        output->isolation = source.substr(skip_zeros);
    if (output && output->isolation.empty()) {
        if (!source.empty()) {
            output->isolation = source;
            add_message(output, parsing_message_t::invalid_isolation, source, name);
        }
        else
            add_message(output, parsing_message_t::isolation_absent, source, name);
    }
    return true;

} // acmacs::virus::name::check_isolation

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_year(std::string_view source, parsed_fields_t* output, std::string_view name)
{
#include "acmacs-base/global-constructors-push.hh"
    static const auto current_year = date::current_year();
    static const auto current_year_2 = current_year % 100;
#include "acmacs-base/diagnostics-pop.hh"

    const std::string_view digits{source.data(), static_cast<size_t>(std::find_if(std::begin(source), std::end(source), [](char cc) { return !std::isdigit(cc); }) - std::begin(source))};

    try {
        switch (digits.size()) {
            case 1:
            case 2:
                if (const auto year = ::string::from_chars<size_t>(digits); year <= current_year_2)
                    output->year = fmt::format("{}", year + 2000);
                else if (year < 100) // from_chars returns std::numeric_limits<size_t>::max() if number cannot be read
                    output->year = fmt::format("{}", year + 1900);
                else
                    throw std::exception{};
                break;
            case 4:
                if (const auto year = ::string::from_chars<size_t>(digits); year <= current_year)
                    output->year = fmt::format("{}", year);
                else
                    throw std::exception{};
                break;
            default:
                throw std::exception{};
        }
        if (digits.size() < source.size())
            output->extra.emplace_back(std::next(std::begin(source), static_cast<ssize_t>(digits.size())), std::end(source));
        return true;
    }
    catch (std::exception&) {
        add_message(output, parsing_message_t::invalid_year, source, name);
        return false;
    }

} // acmacs::virus::name::check_year

// ----------------------------------------------------------------------

bool acmacs::virus::name::host_confusing_with_location(std::string_view source)
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
        "UNKNOWN"sv,
        "SWAN"sv,
        "TIGER"sv,
        "WILLET"sv,
        "QUAIL"sv,
        "PELICAN"sv,
        "EGRET"sv,
        "PARTRIDGE"sv,
        "CURLEW"sv,
        "PIGEON"sv,
        "CANINE"sv,
        "TEAL"sv,
        "GULL"sv,
        "AVES"sv,               // Aves is the class of birds (nominative plural of avis "bird" in Latin), and town in Portugal
    };

    return std::find(std::begin(hosts), std::end(hosts), source) != std::end(hosts);

} // acmacs::virus::name::host_confusing_with_location

// ----------------------------------------------------------------------

// void acmacs::virus::name::host_location_fix(try_fields_t& input, parsed_fields_t& output, std::string_view name)
// {
//     const auto use_loc_empty_host = [&](const location_data_t& loc) {
//         input.host = std::string_view{};
//         output.host = host_t{};
//         output.location = loc.name;
//         output.country = loc.country;
//         output.continent = loc.continent;
//     };

//     // Location data is in output, location is valid, country in the output present
//     if (const auto host_loc = location_lookup(input.host); host_loc.has_value()) {
//         if (host_loc->name == output.country) { // A/India/Delhi/DB106/2009 -> A/Delhi/DB106/2009
//             input.host = std::string_view{};
//             output.host = host_t{};
//         }
//         else if (host_loc->country == output.location) { // A/Cologne/Germany/01/2009 -> A/Cologne/01/2009
//             use_loc_empty_host(*host_loc);
//         }
//         else if (host_loc->country == output.country && host_loc->name != output.location) { // "B/Mount_Lebanon/Ain_W_zein/3/2019" -> "B/Mount Lebanon Ain W zein/3/2019"
//             if (const auto loc2 = location_lookup(fmt::format("{} {}", host_loc->name, output.location)); loc2.has_value()) {
//                 use_loc_empty_host(*loc2);
//             }
//             else if (!host_confusing_with_location(input.host)) {
//                 AD_WARNING("location and host are separate locations: loc:\"{}\" host:\"{}\" <-- \"{}\"", input.location, input.host, name);
//             }
//         }
//         else if (!host_confusing_with_location(input.host)) {
//             AD_WARNING("location and host are separate locations: loc:\"{}\" host:\"{}\" <-- \"{}\"", input.location, input.host, name);
//         }
//     }

// } // acmacs::virus::name::host_location_fix

// ----------------------------------------------------------------------

std::string acmacs::virus::name::check_reassortant_in_front(std::string_view source, parsed_fields_t& output)
{
    if (source.front() == 'A' || source.front() == 'B') {
        if (source.size() < 14 && source.substr(1, 13) != "/REASSORTANT/")
            return std::string{source};
    }

    std::string result;
    std::tie(output.reassortant, result) = parse_reassortant(source);

    if (!output.reassortant.empty())
        AD_DEBUG("REASS {}", source);

    if (!result.empty()) {
        if (result.front() == '(' && result.back() == ')') {
            result.erase(std::prev(result.end()));
            result.erase(result.begin());
        }
        else if (result.front() == '(' && result.find(')', 1) == std::string::npos) {
            result.erase(result.begin());
        }
    }
    if (result.size() > 3 && !output.reassortant.empty() && result[0] == 'H' && result[1] == 'Y' && result[2] == ' ')
        result.erase(result.begin(), std::next(result.begin(), 3));

    if (result.empty() && !output.reassortant.empty())
        output.messages.emplace_back(parsing_message_t::empty_name, "", source);

    // AD_DEBUG("check_reassortant_in_front \"{}\" -> \"{}\" R:\"{}\"", source, result, *output.reassortant);

    return result;

} // acmacs::virus::name::check_reassortant_in_front

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
