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
    static bool check_subtype(std::string_view source, fields_t& output, parsing_messages_t& messages);
    static bool check_location(std::string_view source, fields_t& output, parsing_messages_t& messages);
    static bool check_year(std::string_view source, fields_t& output, parsing_messages_t& messages);

} // namespace acmacs::virus::inline v2::name

// ----------------------------------------------------------------------

std::pair<acmacs::virus::name::fields_t, acmacs::virus::name::parsing_messages_t> acmacs::virus::name::parse(std::string_view source)
{
    fields_t fields;
    parsing_messages_t messages;

    const std::string upcased = ::string::upper(source);
    const auto parts = acmacs::string::split(upcased, "/");
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
    if (check_location(input.location, output, messages) && check_year(input.year_rest, output, messages) && check_subtype(input.subtype, output, messages)) {
        output.isolation = input.isolation;
        return true;
    }

    return false;

} // acmacs::virus::name::check

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_subtype(std::string_view source, fields_t& output, parsing_messages_t& messages)
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
        messages.emplace_back(parsing_message_t::invalid_subtype, source);
        return false;
    }

} // acmacs::virus::name::check_subtype

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_location(std::string_view source, fields_t& output, parsing_messages_t& messages)
{
    if (source.size() < 3) {
        messages.emplace_back(parsing_message_t::location_not_found, source);
        return false;
    }

    try {
        const auto& locdb = get_locdb();
        const auto loc = locdb.find(::string::strip(source));
        output.location = loc.name;
        const auto country = loc.country();
        output.country = country;
        output.continent = locdb.continent_of_country(country);
        return true;
    }
    catch (LocationNotFound& /*err*/) {
        messages.emplace_back(parsing_message_t::location_not_found, source);
        return false;
    }

} // acmacs::virus::name::check_location

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_year(std::string_view source, fields_t& output, parsing_messages_t& messages)
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
        messages.emplace_back(parsing_message_t::invalid_year, source);
        return false;
    }

} // acmacs::virus::name::check_year

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
