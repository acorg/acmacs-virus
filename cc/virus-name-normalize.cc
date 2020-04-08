#include "acmacs-base/string-split.hh"
#include "acmacs-base/date.hh"
#include "acmacs-base/regex.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name-normalize.hh"

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

    static bool check(try_fields_t&& input, fields_t& output, parsing_messages_t& messages);
    static bool check_subtype(std::string_view source, fields_t* output = nullptr, parsing_messages_t* messages = nullptr);
    static bool check_host(std::string_view source, fields_t* output = nullptr, parsing_messages_t* messages = nullptr);
    static bool check_location(std::string_view source, fields_t* output = nullptr, parsing_messages_t* messages = nullptr);
    static bool check_isolation(std::string_view source, fields_t* output = nullptr, parsing_messages_t* messages = nullptr);
    static bool check_year(std::string_view source, fields_t* output = nullptr, parsing_messages_t* messages = nullptr);
    static bool host_confusing_with_location(std::string_view source);
    static void host_location_fix(try_fields_t& input, fields_t& output);

    inline void add_message(parsing_messages_t* messages, const char* key, std::string_view value)
    {
        if (messages)
            messages->emplace_back(key, value);
    }

    // ----------------------------------------------------------------------

    struct location_data_t
    {
        std::string name;
        std::string country;
        std::string continent;
    };

    static std::optional<location_data_t> location_lookup(std::string_view source);

} // namespace acmacs::virus::inline v2::name

// ----------------------------------------------------------------------

std::pair<acmacs::virus::name::fields_t, acmacs::virus::name::parsing_messages_t> acmacs::virus::name::parse(std::string_view source)
{
    fields_t fields;
    parsing_messages_t messages;

    const std::string upcased = ::string::upper(source);
    auto parts = acmacs::string::split(upcased, "/");
    for (auto& part : parts)
        part = ::string::strip(part);
    switch (parts.size()) {
      case 1:
          break;
      case 2:
          break;
      case 3:
          check(try_fields_t{.location=parts[0], .isolation=parts[1], .year_rest=parts[2]}, fields, messages);
          break;
      case 4:
          check(try_fields_t{.subtype=parts[0], .location=parts[1], .isolation=parts[2], .year_rest=parts[3]}, fields, messages);
          break;
      case 5:
          check(try_fields_t{.subtype=parts[0], .host=parts[1], .location=parts[2], .isolation=parts[3], .year_rest=parts[4]}, fields, messages);
          break;
    }

    if (!fields.empty() || !messages.empty())
        return {fields, messages};
    else
        return {fields_t{}, parsing_messages_t{{parsing_message_t::unrecognized, source}}};

} // acmacs::virus::name::parse

// ----------------------------------------------------------------------

bool acmacs::virus::name::check(try_fields_t&& input, fields_t& output, parsing_messages_t& messages)
{
    output = fields_t{};
    messages.clear();
    if (check_location(input.location, &output, &messages)) {
        host_location_fix(input, output);
        if (check_year(input.year_rest, &output, &messages) && check_subtype(input.subtype, &output, &messages) && check_host(input.host, &output, &messages) &&
            check_isolation(input.isolation, &output, &messages))
            return true;
    }
    return false;

} // acmacs::virus::name::check

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_subtype(std::string_view source, fields_t* output, parsing_messages_t* messages)
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
        add_message(messages, parsing_message_t::invalid_subtype, source);
        return false;
    }

} // acmacs::virus::name::check_subtype

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_host(std::string_view source, fields_t* output, parsing_messages_t* messages)
{
    if (output)
        output->host = host_t{source};
    return true;

} // acmacs::virus::name::check_host

// ----------------------------------------------------------------------

std::optional<acmacs::virus::name::location_data_t> acmacs::virus::name::location_lookup(std::string_view source)
{
    try {
        const auto& locdb = get_locdb();
        const auto loc = locdb.find(source);
        const auto country = loc.country();
        return location_data_t{.name = loc.name, .country = std::string{country}, .continent = std::string{locdb.continent_of_country(country)}};
    }
    catch (LocationNotFound& /*err*/) {
        return std::nullopt;
    }

} // acmacs::virus::name::location_lookup

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_location(std::string_view source, fields_t* output, parsing_messages_t* messages)
{
    if (source.size() < 3) {
        add_message(messages, parsing_message_t::location_not_found, source);
        return false;
    }

    if (const auto loc = location_lookup(source); loc.has_value()) {
        if (output) {
            output->location = loc->name;
            output->country = loc->country;
            output->continent = loc->continent;
        }
        return true;
    }
    else {
        add_message(messages, parsing_message_t::location_not_found, source);
        return false;
    }

} // acmacs::virus::name::check_location

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_isolation(std::string_view source, fields_t* output, parsing_messages_t* messages)
{
    if (const auto skip_zeros = source.find_first_not_of('0'); skip_zeros != std::string_view::npos)
        output->isolation = source.substr(skip_zeros);
    if (output && output->isolation.empty()) {
        if (!source.empty()) {
            output->isolation = source;
            add_message(messages, parsing_message_t::invalid_isolation, source);
        }
        else
            add_message(messages, parsing_message_t::isolation_absent, source);
    }
    return true;

} // acmacs::virus::name::check_isolation

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_year(std::string_view source, fields_t* output, parsing_messages_t* messages)
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
        add_message(messages, parsing_message_t::invalid_year, source);
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

void acmacs::virus::name::host_location_fix(try_fields_t& input, fields_t& output)
{
    // Location data is in output, location is valid, country in the output present
    if (const auto host_loc = location_lookup(input.host); host_loc.has_value()) {
        if (host_loc->name == output.country) { // A/India/Delhi/DB106/2009 -> A/Delhi/DB106/2009
            input.host = std::string_view{};
            output.host = host_t{};
        }
        else if (host_loc->country == output.location) { // A/Cologne/Germany/01/2009 -> A/Cologne/01/2009
            input.host = std::string_view{};
            output.host = host_t{};
            output.location = host_loc->name;
            output.country = host_loc->country;
            output.continent = host_loc->continent;
        }
        else if (!host_confusing_with_location(input.host)) {
            AD_WARNING("location and host are separate locations: loc:\"{}\" host:\"{}\"", input.location, input.host);
        }
    }

} // acmacs::virus::name::host_location_fix

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
