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
    constexpr const std::string_view unknown_isolation{"UNKNWON"};

    // ----------------------------------------------------------------------

    struct try_fields_t
    {
        std::string_view subtype{};
        std::string_view host{};
        std::string_view location{};
        std::string_view isolation{};
        std::string_view year_rest{};
    };

    // ----------------------------------------------------------------------

    struct location_data_t
    {
        std::string name;
        std::string country;
        std::string continent;
    };

    struct location_part_t
    {
        size_t part_no{1000};
        location_data_t location;

        bool valid() const { return !location.name.empty(); }
    };

    using location_parts_t = std::vector<location_part_t>;

    inline void set(parsed_fields_t& output, location_data_t&& location_data)
    {
        output.location = std::move(location_data.name);
        output.country = std::move(location_data.country);
        output.continent = std::move(location_data.continent);
    }

    inline void set(parsed_fields_t& output, location_part_t&& location_part)
    {
        set(output, std::move(location_part.location));
    }

    // ----------------------------------------------------------------------

    static void no_location_parts(std::string_view name, const std::vector<std::string_view>& parts, parsed_fields_t& output);
    static void one_location_part(std::vector<std::string_view>& parts, location_part_t&& location_part, parsed_fields_t& output);
    static void one_location_part_at_1(std::vector<std::string_view>& parts, parsed_fields_t& output);
    static void one_location_part_at_2(std::vector<std::string_view>& parts, parsed_fields_t& output);
    static void two_location_parts(std::vector<std::string_view>& parts, location_parts_t&& location_parts, parsed_fields_t& output);

    enum class make_message { no, yes };

    static bool check_subtype(std::string_view source, parsed_fields_t& output);
    static bool check_host(std::string_view source, parsed_fields_t& output);
    static bool check_location(std::string_view source, parsed_fields_t& output);
    static bool check_isolation(std::string_view source, parsed_fields_t& output);
    static bool check_year(std::string_view source, parsed_fields_t& output, make_message report = make_message::yes);
    static bool host_confusing_with_location(std::string_view source);
    // static void host_location_fix(try_fields_t& input, parsed_fields_t& output, std::string_view name);
    static location_parts_t find_location_parts(const std::vector<std::string_view>& parts);
    static std::string check_reassortant_in_front(std::string_view source, parsed_fields_t& output);
    static bool check_nibsc_extra(std::vector<std::string_view>& parts);

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

    // inline void add_message(parsed_fields_t* output, std::string_view key, std::string_view value)
    // {
    //     if (output)
    //         output->messages.emplace_back(key, value);
    // }

    inline ssize_t count_open_paren(std::string_view source) { return std::count(std::begin(source), std::end(source), '('); }
    inline ssize_t count_close_paren(std::string_view source) { return std::count(std::begin(source), std::end(source), ')'); }

    // returns 0 if paren match: "()(())"
    // returns <0 if close paren prevail: "xx)"
    // returns >0 if open paren prevail: "xx(()"
    inline int paren_match(std::string_view source, std::string_view paren = "()")
    {
        int counter{0};
        for (auto sym : source) {
            if (sym == paren[0])
                ++counter;
            else if (sym == paren[1]) {
                if (--counter < 0)
                    return counter;
            }
        }
        return counter;
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
    source = ::string::strip(source);
    parsed_fields_t output{.raw = std::string{source}};
    if (source.empty()) {
        AD_WARNING("empty source");
        return output;
    }

    std::string upcased = ::string::upper(source);
    if (possible_reassortant_in_front(upcased))
        upcased = check_reassortant_in_front(upcased, output);
    auto parts = acmacs::string::split(upcased, "/", acmacs::string::Split::StripKeepEmpty);
    auto location_parts = find_location_parts(parts);
    switch (location_parts.size()) {
      case 0:
          no_location_parts(source, parts, output);
          break;
      case 1:
          one_location_part(parts, std::move(location_parts.front()), output);
          break;
      case 2:
          two_location_parts(parts, std::move(location_parts), output);
          break;
      default:
          output.messages.emplace_back("multiple-location", fmt::format("{}", location_parts));
          break;
    }

    if (!output.good() && output.messages.empty())
        output.messages.emplace_back(parsing_message_t::unrecognized, source);

    return output;

} // acmacs::virus::name::parse

// ----------------------------------------------------------------------

void acmacs::virus::name::no_location_parts(std::string_view name, const std::vector<std::string_view>& /*parts*/, parsed_fields_t& output)
{
    output.messages.emplace_back(parsing_message_t::location_field_not_found, name);

} // acmacs::virus::name::no_location_parts

// ----------------------------------------------------------------------

void acmacs::virus::name::one_location_part(std::vector<std::string_view>& parts, location_part_t&& location_part, parsed_fields_t& output)
{
    if (location_part.valid())
        set(output, std::move(location_part));
    switch (location_part.part_no) {
        case 0:
            AD_DEBUG("location in part 0 {}", parts);
            // if (parts.size() == 3)
            //     check(try_fields_t{.location = parts[0], .isolation = parts[1], .year_rest = parts[2]}, output);
            break;
        case 1:
            one_location_part_at_1(parts, output);
            break;
        case 2:
            one_location_part_at_2(parts, output);
            break;
        default:
            output.messages.emplace_back("unexpected-location-part", fmt::format("{}", location_part.part_no));
            break;
    }

} // acmacs::virus::name::one_location_part

// ----------------------------------------------------------------------

void acmacs::virus::name::one_location_part_at_1(std::vector<std::string_view>& parts, parsed_fields_t& output)
{
    try {
        switch (parts.size()) {
            case 3: // A/Alaska/1935
                if (!check_subtype(parts[0], output) || !check_year(parts[2], output) || !check_isolation(unknown_isolation, output))
                    throw std::exception{};
                break;
            case 4: // A/Germany/1/2014
                if (!check_subtype(parts[0], output) || !check_isolation(parts[2], output) || !check_year(parts[3], output))
                    throw std::exception{};
                break;
            case 5:
                if (check_year(parts[4], output, make_message::no)) {
                    if (auto location_data = location_lookup(string::join(" ", parts[1], parts[2])); location_data.has_value()) { // A/Lyon/CHU/R19.03.77/2019
                        set(output, std::move(*location_data));
                        if (!check_subtype(parts[0], output) || !check_isolation(parts[3], output)) // A/Algeria/G0164/15/2015 :h1n1
                            throw std::exception{};
                    }
                    else if (!check_subtype(parts[0], output) || !check_isolation(string::join("-", parts[2], parts[3]), output)) // A/Algeria/G0164/15/2015 :h1n1
                        throw std::exception{};
                }
                else if (check_nibsc_extra(parts) && parts.size() == 4 /* check_nibsc_extra removed last part */) { // "A/Beijing/2019-15554/2018  CNIC-1902  (19/148)"
                    // AD_DEBUG("nisbc extra {}", parts);
                    one_location_part_at_1(parts, output);
                }
                else
                    throw std::exception{};
                break;
            default:
                throw std::exception{};
        }
    }
    catch (std::exception&) {
        output.messages.emplace_back("unexpected-location-part", fmt::format("1 {}", parts));
    }

} // acmacs::virus::name::one_location_part_at_1

// ----------------------------------------------------------------------

void acmacs::virus::name::one_location_part_at_2(std::vector<std::string_view>& parts, parsed_fields_t& output)
{
    try {
        switch (parts.size()) {
            case 4: // A/Chicken/Liaoning/99
                if (!check_subtype(parts[0], output) || !check_host(parts[1], output) || !check_isolation(unknown_isolation, output) || !check_year(parts[3], output))
                    throw std::exception{};
                break;
            case 5: // A/Swine/Germany/1/2014
                if (!check_subtype(parts[0], output) || !check_host(parts[1], output) || !check_isolation(parts[3], output) || !check_year(parts[4], output))
                    throw std::exception{};
                break;
            case 6:
                if (check_year(parts[5], output, make_message::no)) {
                    if (auto location_data = location_lookup(string::join(" ", parts[2], parts[3])); location_data.has_value()) { // A/swine/Lyon/CHU/R19.03.77/2019
                        set(output, std::move(*location_data));
                        if (!check_subtype(parts[0], output) || !check_host(parts[1], output) || !check_isolation(parts[4], output))
                            throw std::exception{};
                    }
                    else if (!check_subtype(parts[0], output) || !check_host(parts[1], output) ||
                             !check_isolation(string::join("-", parts[3], parts[4]), output)) // A/chicken/CentralJava/Solo/VSN331/2013
                        throw std::exception{};
                }
                else if (check_nibsc_extra(parts) && parts.size() == 5 /* check_nibsc_extra removed last part */) { // A/duck/Vietnam/NCVD1584/2012 NIBRG-301 (18/134)
                    one_location_part_at_2(parts, output);
                }
                else
                    throw std::exception{};
                break;
            default:
                throw std::exception{};
        }
    }
    catch (std::exception&) {
        output.messages.emplace_back("unexpected-location-part", fmt::format("2 {}", parts));
    }

} // acmacs::virus::name::one_location_part_at_2

// ----------------------------------------------------------------------

void acmacs::virus::name::two_location_parts(std::vector<std::string_view>& parts, location_parts_t&& location_parts, parsed_fields_t& output)
{
    if ((location_parts[0].part_no + 1) != location_parts[1].part_no) {
        output.messages.emplace_back("double-location", fmt::format("{}", location_parts));
    }
    else if (host_confusing_with_location(parts[location_parts.front().part_no])) {
        one_location_part(parts, std::move(location_parts[1]), output);
    }
    else if (location_parts[0].location.name == location_parts[1].location.country) { // A/India/Delhi/DB106/2009 -> A/Delhi/DB106/2009
        parts.erase(std::next(parts.begin(), static_cast<ssize_t>(location_parts[0].part_no)));
        one_location_part(parts, std::move(location_parts[0]), output);
    }
    else if (location_parts[0].location.country == location_parts[1].location.name) { // A/Cologne/Germany/01/2009 -> A/Cologne/01/2009
        parts.erase(std::next(parts.begin(), static_cast<ssize_t>(location_parts[1].part_no)));
        one_location_part(parts, std::move(location_parts[0]), output);
    }
    else if (location_parts[0].location.country == location_parts[1].location.country) { // "B/Mount_Lebanon/Ain_W_zein/3/2019" -> "B/Mount Lebanon Ain W zein/3/2019"
        check_location(string::join(" ", location_parts[0].location.name, location_parts[1].location.name), output);
        parts.erase(std::next(parts.begin(), static_cast<ssize_t>(location_parts[1].part_no)));
        one_location_part(parts, location_part_t{.part_no=location_parts[0].part_no}, output);
    }
    else
        output.messages.emplace_back("double-location", fmt::format("{}", location_parts));

} // acmacs::virus::name::two_location_parts

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_subtype(std::string_view source, parsed_fields_t& output)
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
                        output.subtype = type_subtype_t{source};
                        break;
                    default:
                        throw std::exception{};
                }
                break;
            default:
                if (std::cmatch mch; std::regex_match(std::begin(source), std::end(source), mch, re_a)) {
                    if (mch.length(1))
                        output.subtype = type_subtype_t{fmt::format("A({})", mch.str(1))};
                    else if (mch.length(2))
                        output.subtype = type_subtype_t{fmt::format("A({})", mch.str(2))};
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
        output.messages.emplace_back(parsing_message_t::invalid_subtype, source);
        return false;
    }

} // acmacs::virus::name::check_subtype

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_host(std::string_view source, parsed_fields_t& output)
{
    output.host = host_t{source};
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

bool acmacs::virus::name::check_location(std::string_view source, parsed_fields_t& output)
{
    if (const auto loc = location_lookup(source); loc.has_value()) {
        output.location = loc->name;
        output.country = loc->country;
        output.continent = loc->continent;
        return true;
    }
    else {
        output.messages.emplace_back(parsing_message_t::location_not_found, source);
        return false;
    }

} // acmacs::virus::name::check_location

// ----------------------------------------------------------------------

acmacs::virus::name::location_parts_t acmacs::virus::name::find_location_parts(const std::vector<std::string_view>& parts)
{
    location_parts_t location_parts;
    for (size_t part_no = 0; part_no < parts.size(); ++part_no) {
        if (const auto loc = location_lookup(parts[part_no]); loc.has_value())
            location_parts.push_back({part_no, *loc});
    }
    return location_parts;

} // acmacs::virus::name::find_location_parts

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_isolation(std::string_view source, parsed_fields_t& output)
{
    if (const auto skip_zeros = source.find_first_not_of('0'); skip_zeros != std::string_view::npos)
        output.isolation = source.substr(skip_zeros);
    if (output.isolation.empty()) {
        if (!source.empty()) {
            output.isolation = source;
            // output.messages.emplace_back(parsing_message_t::invalid_isolation, source);
        }
        else
            output.messages.emplace_back(parsing_message_t::isolation_absent, source);
    }
    return true;

} // acmacs::virus::name::check_isolation

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_year(std::string_view source, parsed_fields_t& output, make_message report)
{
#include "acmacs-base/global-constructors-push.hh"
    static const auto current_year = date::current_year();
    static const auto current_year_2 = current_year % 100;
#include "acmacs-base/diagnostics-pop.hh"

    const std::string_view digits{source.data(), static_cast<size_t>(std::find_if(std::begin(source), std::end(source), [](char cc) { return !std::isdigit(cc); }) - std::begin(source))};

    try {
        if (paren_match(source) != 0) // e.g. last part in "A/Beijing/2019-15554/2018  CNIC-1902  (19/148)"
            throw std::exception{};
        switch (digits.size()) {
            case 1:
            case 2:
                if (const auto year = ::string::from_chars<size_t>(digits); year <= current_year_2)
                    output.year = fmt::format("{}", year + 2000);
                else if (year < 100) // from_chars returns std::numeric_limits<size_t>::max() if number cannot be read
                    output.year = fmt::format("{}", year + 1900);
                else
                    throw std::exception{};
                break;
            case 4:
                if (const auto year = ::string::from_chars<size_t>(digits); year <= current_year)
                    output.year = fmt::format("{}", year);
                else
                    throw std::exception{};
                break;
            default:
                throw std::exception{};
        }
        if (digits.size() < source.size())
            output.extra.emplace_back(std::next(std::begin(source), static_cast<ssize_t>(digits.size())), std::end(source));
        return true;
    }
    catch (std::exception&) {
        if (report == make_message::yes)
            output.messages.emplace_back(parsing_message_t::invalid_year, source);
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
    std::string result;
    std::tie(output.reassortant, result) = parse_reassortant(source);

    // if (!output.reassortant.empty())
    //     AD_DEBUG("REASS {}", source);

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
        output.messages.emplace_back(parsing_message_t::empty_name);

    // AD_DEBUG("check_reassortant_in_front \"{}\" -> \"{}\" R:\"{}\"", source, result, *output.reassortant);

    return result;

} // acmacs::virus::name::check_reassortant_in_front

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_nibsc_extra(std::vector<std::string_view>& parts)
{
    // remove (19/148) in "A/Beijing/2019-15554/2018  CNIC-1902  (19/148)"
    if (parts.size() > 2 && paren_match(parts[parts.size() - 2]) > 0 && paren_match(parts[parts.size() - 1]) < 0) {
        parts.erase(std::prev(std::end(parts)));
        parts.back().remove_suffix(parts.back().size() - parts.back().find_last_of('('));
        return true;
    }
    else
        return false;

} // acmacs::virus::name::check_nibsc_extra

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
