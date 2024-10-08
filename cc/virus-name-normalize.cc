#include <array>

#include "acmacs-base/string-split.hh"
#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-digits.hh"
#include "acmacs-base/string-strip.hh"
#include "acmacs-base/string-from-chars.hh"
#include "acmacs-base/string-compare.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/date.hh"
#include "acmacs-base/regex.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name-normalize.hh"
#include "acmacs-virus/host.hh"
#include "acmacs-virus/passage.hh"
#include "acmacs-virus/log.hh"

// ----------------------------------------------------------------------

acmacs::virus::name_t acmacs::virus::name::parsed_fields_t::name() const noexcept
{
    if (good())
        return name_t{acmacs::string::join(acmacs::string::join_slash, subtype, host, location, isolation, year)};
    else if (subtype.empty() && host.empty() && location.empty() && isolation.empty() && year.empty() && !reassortant.empty() && extra.empty())
        return name_t{*reassortant};
    else
        return name_t{raw};

} // acmacs::virus::name::parsed_fields_t::name

// ----------------------------------------------------------------------

std::string acmacs::virus::name::parsed_fields_t::full_name() const noexcept
{
    if (good())
        return acmacs::string::join(acmacs::string::join_space, name(), reassortant, extra, passage);
    else
        return raw;

} // acmacs::virus::name::parsed_fields_t::full_name

// ----------------------------------------------------------------------

namespace acmacs::virus::inline v2::name
{
    constexpr const std::string_view unknown_isolation{"UNKNOWN"};

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
        std::string country{};
        std::string continent{};
        constexpr bool good() const { return true; }
    };

    struct location_part_t
    {
        size_t part_no{1000};
        location_data_t location{};

        bool valid() const { return !location.name.empty(); }
    };

    using location_parts_t = std::vector<location_part_t>;

    inline void set_location(parsed_fields_t& output, location_data_t&& location_data)
    {
        output.location = std::move(location_data.name);
        output.country = std::move(location_data.country);
        output.continent = std::move(location_data.continent);
    }

    inline void set_location(parsed_fields_t& output, location_part_t&& location_part)
    {
        set_location(output, std::move(location_part.location));
    }

    // ----------------------------------------------------------------------

    static void no_location_parts(std::vector<std::string_view>& parts, parsed_fields_t& output);
    static void one_location_part(std::vector<std::string_view>& parts, location_part_t&& location_part, parsed_fields_t& output);
    static void one_location_part_at_1(std::vector<std::string_view>& parts, parsed_fields_t& output);
    static void one_location_part_at_2(std::vector<std::string_view>& parts, parsed_fields_t& output);
    static void two_location_parts(std::vector<std::string_view>& parts, location_parts_t&& location_parts, parsed_fields_t& output);

    enum class make_message { no, yes };

    static bool check_subtype(std::string_view source, parsed_fields_t& output, make_message report = make_message::yes);
    static bool check_host(std::string_view source, parsed_fields_t& output);
    static bool check_location(std::string_view source, parsed_fields_t& output);
    static bool check_isolation(std::string_view source, parsed_fields_t& output);
    static bool check_year(std::string_view source, parsed_fields_t& output, make_message report = make_message::yes);
    static location_parts_t find_location_parts(std::vector<std::string_view>& parts, acmacs::messages::messages_t& messages);
    static std::string check_reassortant_in_front(std::string_view source, parsed_fields_t& output);
    static std::string remove_reassortant_second_name(std::string_view source);
    static bool check_nibsc_extra(std::vector<std::string_view>& parts);
    static bool location_as_prefix(std::vector<std::string_view>& parts, size_t part_to_check, parsed_fields_t& output);
    static void check_extra(parsed_fields_t& output);
    static bool location_part_as_isolation_prefix(std::string_view isolation, parsed_fields_t& output);

    // ----------------------------------------------------------------------

    struct location_not_found_t
    {
        std::string name;
        location_not_found_t(std::string_view nn) : name{nn} {}
        constexpr bool good() const { return false; }
    };
    struct location_chinese_name_t : public location_data_t
    {
        location_chinese_name_t(std::string_view nn) : location_data_t{.name = std::string{nn}} {}
        constexpr bool good() const { return false; }
    };

    using location_lookup_result_t = std::variant<location_not_found_t, location_data_t, location_chinese_name_t>;

    constexpr bool good(const location_lookup_result_t& res) {
        return std::visit([](auto&& arg) { return arg.good(); }, res);
    }

    inline location_data_t& get(location_lookup_result_t& res)
    {
        return std::visit(
            []<typename Arg>(Arg&& arg) -> location_data_t& {
                if constexpr (std::is_same_v<location_data_t, std::decay_t<Arg>>)
                    return arg;
                else if constexpr (std::is_same_v<location_chinese_name_t, std::decay_t<Arg>>)
                    throw std::runtime_error{AD_FORMAT("lookup_result_t is location_chinese_name_t \"{}\"", arg.name)};
                else
                    throw std::runtime_error{AD_FORMAT("lookup_result_t is location_not_found_t \"{}\"", arg.name)};
            },
            res);
    }

    static location_lookup_result_t location_lookup(std::string_view source);

    // ----------------------------------------------------------------------

    // quick check to avoid calling slow (many regexp based check_reassortant_in_front)
    inline bool possible_reassortant_in_front(std::string_view source)
    {
        using namespace std::string_view_literals;
        if (source.size() > 6) {
            switch (std::toupper(source.front())) {
                case 'I': // IVR
                case 'N': // NIB, NYMC
                case 'R': // RG
                case 'S': // SAN
                case 'X': // X-327
                    return true;
                case 'C':
                    return ::string::upper(source.substr(1, 4)) == "NIC-"sv; // CNIC-2006
                case 'B':
                    switch (std::toupper(source[1])) {
                        case 'V': // BVR
                        case 'X': // BX
                            return true;
                        case '/':
                            if (::string::upper(source.substr(2, 5)) == "REASS"sv || ::string::upper(source.substr(2, 5)) == "RESAS"sv || ::string::upper(source.substr(2, 2)) == "X-"sv ||
                                ::string::upper(source.substr(2, 4)) == "NYMC"sv) // B/REASSORTANT/
                                return true;
                            break;
                    }
                    break;
                case 'A':
                    if (::string::upper(source.substr(1, 6)) == "/REASS"sv || ::string::upper(source.substr(1, 6)) == "/RESAS"sv || ::string::upper(source.substr(1, 3)) == "/X-"sv ||
                        ::string::upper(source.substr(1, 5)) == "/NYMC"sv) // A/REASSORTANT/
                        return true;
                    if (source[1] == '(') {
                        if (const auto pos = source.find(')'); pos < (source.size() - 6) && ::string::upper(source.substr(pos + 1, 6)) == "/REASS"sv)
                            return true;
                    }
                    break;
            }
        }
        return false;
    }

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

    inline void add_extra(acmacs::virus::name::parsed_fields_t& output, std::string_view to_add, char separator = ' ')
    {
        if (output.extra.empty())
            output.extra.assign(to_add);
        else
            output.extra += fmt::format("{}{}", separator, to_add);
        AD_LOG(acmacs::log::name_parsing, "add_extra \"{}\" <- \"{}\"", output.extra, to_add);
    }

    // ----------------------------------------------------------------------

    inline std::tuple<acmacs::virus::mutations_t, std::string> parse_mutatations(std::string_view source)
    {
        using namespace acmacs::regex;

#include "acmacs-base/global-constructors-push.hh"

#define PM_AMINO_ACID "[ACDEFGHIKLMNPQRSTVWY]"

        static const std::array normalize_data{
            look_replace_t{std::regex("HA[-_](" PM_AMINO_ACID "?\\d{1,3}" PM_AMINO_ACID ")(?![A-Z0-9\\?])", std::regex::icase), {"HA-$1", "$` $'"}},
            look_replace_t{std::regex("\\b(" PM_AMINO_ACID "\\d{1,3}" PM_AMINO_ACID ")(?![A-Z0-9\\?\\)])", std::regex::icase), {"$1", "$` $'"}}, // <letter><number><letter> not followed by letter/number/?/) to avoid matching subpyte spec
        };
#include "acmacs-base/diagnostics-pop.hh"

        mutations_t mutations;
        std::string look_in{source};
        while (true) {
            if (auto mutation_rest = scan_replace(look_in, normalize_data); mutation_rest.has_value()) {
                mutations.push_back(mutation_t{mutation_rest->front()});
                look_in = ::string::collapse_spaces(acmacs::string::strip(mutation_rest->back()));
            }
            else
                break;
        }
        return {std::move(mutations), std::move(look_in)};
    }

} // namespace acmacs::virus::inline v2::name

template <> struct fmt::formatter<acmacs::virus::name::location_parts_t> : public fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::name::location_parts_t& parts, FormatContext& ctx)
    {
        fmt::format_to(ctx.out(), "{{");
        for (const auto& part : parts)
            fmt::format_to(ctx.out(), " {}", part.part_no);
        return fmt::format_to(ctx.out(), "}}");
    }
};

// ----------------------------------------------------------------------

acmacs::virus::name::parsed_fields_t acmacs::virus::name::parse(std::string_view source, warn_on_empty woe, extract_passage ep)
{
    source = acmacs::string::strip(source);
    parsed_fields_t output{.raw = std::string{source}, .extract_passage_ = ep};
    if (source.empty()) {
        if (woe == warn_on_empty::yes)
            AD_WARNING("empty source");
        return output;
    }

    std::string source_s{source};
    if (possible_reassortant_in_front(source_s))
        source_s = check_reassortant_in_front(source_s, output);

    auto parts = acmacs::string::split(source_s, "/", acmacs::string::Split::StripRemoveEmpty);
    auto location_parts = find_location_parts(parts, output.messages);
    switch (location_parts.size()) {
      case 0:
          no_location_parts(parts, output);
          break;
      case 1:
          one_location_part(parts, std::move(location_parts.front()), output);
          break;
      case 2:
          two_location_parts(parts, std::move(location_parts), output);
          break;
      default:
          output.messages.emplace_back("multiple-location", fmt::format("{}", location_parts), MESSAGE_CODE_POSITION);
          break;
    }

    check_extra(output);

    if (output.good() && output.host.empty() && std::isalpha(output.isolation[0]) && is_host(output.location)) // perhaps real location and isolation are inside the same part
        output.messages.emplace_back(acmacs::messages::key::location_or_host, source, MESSAGE_CODE_POSITION);

    if (!output.good() && output.messages.empty())
        output.messages.emplace_back(acmacs::messages::key::unrecognized, source, MESSAGE_CODE_POSITION);

    return output;

} // acmacs::virus::name::parse

// ----------------------------------------------------------------------

// std::vector<std::string> acmacs::virus::name::possible_locations_in_name(std::string_view source)
// {
//     std::vector<std::string> result;
//     if (source.empty())
//         return result;

//     const auto add = [&result](std::string_view part) {
//         auto prefix = acmacs::string::non_digit_prefix(part);
//         while (prefix.size() > 2 && prefix.back() == '-')
//             prefix.remove_suffix(1);
//         const std::string u_prefix = ::string::upper(prefix);
//         if (prefix.size() > 2 && std::isalpha(prefix[0]) && std::isalpha(prefix[1]) && u_prefix != "SWL" && u_prefix != "REASSORTANT" && !acmacs::string::startswith(u_prefix, "NYMC") && !is_host(u_prefix))
//             result.push_back(std::string{prefix});
//     };

//     parsed_fields_t output{.raw = std::string{source}};
//     std::string source_s{source};
//     if (possible_reassortant_in_front(source_s))
//         source_s = check_reassortant_in_front(source_s, output);
//     const auto parts = acmacs::string::split(source_s, "/", acmacs::string::Split::StripKeepEmpty);
//     for (const auto& part : parts)
//         add(part);
//     // switch (parts.size()) {
//     //   case 3:
//     //       add(part[0]);
//     //       add(part[1]);
//     //       break;
//     // }

//     return result;

// } // acmacs::virus::name::possible_locations_in_name

// ----------------------------------------------------------------------

void acmacs::virus::name::no_location_parts(std::vector<std::string_view>& parts, parsed_fields_t& output)
{
    AD_LOG(acmacs::log::name_parsing, "no_location_parts {}", parts);

    const auto set_unknown_location = [&output](std::string_view name) {
        output.location = ::string::upper(name);
        output.messages.emplace_back(acmacs::messages::key::location_not_found, name, MESSAGE_CODE_POSITION);
    };

    try {
        switch (parts.size()) {
            case 3:
                if (!std::isdigit(parts[1][0]) && check_subtype(parts[0], output, make_message::no) && check_year(parts[2], output, make_message::no) && location_as_prefix(parts, 1, output))
                    ; // A/Baylor1A/81
                else if (acmacs::string::non_digit_prefix(parts[1]).size() == parts[1].size() && !is_host(parts[1]) && check_subtype(parts[0], output, make_message::no) &&
                         check_year(parts[2], output, make_message::no) && check_isolation(unknown_isolation, output))
                    set_unknown_location(parts[1]); // A/unrecognized location/57(H2N2) -> A(H2N2)/unrecognized location/UNKNWON/1957
                else
                    throw std::exception{};
                break;
            case 4:
                if ((std::isdigit(parts[2][0]) && location_as_prefix(parts, 1, output)) || location_as_prefix(parts, 2, output)) // "A/BiliranTB5/0423/2015" "A/chicken/Iran221/2001"
                    ;
                else if (!is_host(parts[1]) && check_subtype(parts[0], output, make_message::no) && check_isolation(parts[2], output) &&
                         check_year(parts[3], output, make_message::no)) // A/Medellin/FLU8292/2007(H3) - Medellin  is unknown location
                    set_unknown_location(parts[1]);
                else
                    throw std::exception{};
                break;
            case 5:
                if (!is_host(parts[2]) && check_subtype(parts[0], output, make_message::no) && check_host(parts[1], output) && check_isolation(parts[3], output) &&
                    check_year(parts[4], output, make_message::no)) // A/QUAIL/DELISERDANG/01160025/2016(H5N1) - DELISERDANG is unknown location, QUAIL is known host
                    set_unknown_location(parts[2]);
                else
                    throw std::exception{};
                break;
            default:
                throw std::exception{};
        }
    }
    catch (std::exception&) {
        output.messages.emplace_back(acmacs::messages::key::location_field_not_found, acmacs::string::join(acmacs::string::join_slash, parts), MESSAGE_CODE_POSITION);
    }

} // acmacs::virus::name::no_location_parts

// ----------------------------------------------------------------------

bool acmacs::virus::name::location_as_prefix(std::vector<std::string_view>& parts, size_t part_to_check, parsed_fields_t& output)
{
    const auto check_prefix = [&](std::string_view prefix) -> bool {
        if (auto location_data = location_lookup(prefix); good(location_data)) { // A/Baylor1A/81
            parts.insert(std::next(std::begin(parts), static_cast<ssize_t>(part_to_check)), prefix);
            parts[part_to_check + 1].remove_prefix(prefix.size());
            one_location_part(parts, location_part_t{.part_no = part_to_check, .location = get(location_data)}, output);
            return true;
        }
        else
            return false;
    };

    // A/Baylor1A/81 A/BiliranTB5/0423/2015 A/FriuliVeneziaGiuliaPN/230/2019
    // find longest prefix that can be found in the location database
    for (auto prefix = acmacs::string::non_digit_prefix(parts[part_to_check]); prefix.size() > 2; prefix.remove_suffix(1)) {
        if (check_prefix(prefix))
            return true;
    }
    return false;

} // acmacs::virus::name::location_as_prefix

// ----------------------------------------------------------------------

void acmacs::virus::name::one_location_part(std::vector<std::string_view>& parts, location_part_t&& location_part, parsed_fields_t& output)
{
    AD_LOG(acmacs::log::name_parsing, "ONE location part {} \"{}\" in {}", location_part.part_no, parts[location_part.part_no], parts);

    if (location_part.valid())
        set_location(output, std::move(location_part));
    switch (location_part.part_no) {
        case 0:
            if (parts.size() == 3 && check_year(parts[2], output) && check_isolation(parts[1], output)) {
                // empty
            }
            // else
            //     AD_DEBUG("location in part 0 {}", parts);
            break;
        case 1:
            one_location_part_at_1(parts, output);
            break;
        case 2:
            one_location_part_at_2(parts, output);
            break;
        default:
            output.messages.emplace_back("unexpected-location-part", fmt::format("{} {}", location_part.part_no, parts), MESSAGE_CODE_POSITION);
            break;
    }

} // acmacs::virus::name::one_location_part

// ----------------------------------------------------------------------

void acmacs::virus::name::one_location_part_at_1(std::vector<std::string_view>& parts, parsed_fields_t& output)
{
    try {
        AD_LOG(acmacs::log::name_parsing, "ONE location part at 1 and {} parts: {}", parts.size(), parts);
        switch (parts.size()) {
            case 3: // A/Alaska/1935
                if (!check_subtype(parts[0], output) || !check_year(parts[2], output) || !check_isolation(unknown_isolation, output))
                    throw std::exception{};
                break;
            case 4: // A/Germany/1/2014
                if (!check_subtype(parts[0], output) || !check_year(parts[3], output))
                    throw std::exception{};
                if (!location_part_as_isolation_prefix(parts[2], output) // A/Lyon/CHU18.54.48/2018
                    && !check_isolation(parts[2], output))
                    throw std::exception{};
                break;
            case 5:
                if (check_year(parts[4], output, make_message::no)) {
                    if (auto location_data = location_lookup(string::join(acmacs::string::join_space, parts[1], parts[2])); good(location_data)) { // A/Lyon/CHU/R19.03.77/2019
                        set_location(output, std::move(get(location_data)));
                        if (!check_subtype(parts[0], output) || !check_isolation(parts[3], output)) // A/Algeria/G0164/15/2015 :h1n1
                            throw std::exception{};
                    }
                    else if (!check_subtype(parts[0], output) || !check_isolation(string::join(acmacs::string::join_dash, parts[2], parts[3]), output)) // A/Algeria/G0164/15/2015 :h1n1
                        throw std::exception{};
                }
                else if (check_nibsc_extra(parts) && parts.size() == 4 /* check_nibsc_extra removed last part */) { // "A/Beijing/2019-15554/2018  CNIC-1902  (19/148)"
                    // AD_DEBUG("nisbc extra {}", parts);
                    one_location_part_at_1(parts, output);
                }
                else if (!check_subtype(parts[0], output) || !check_isolation(parts[2], output) || !check_year(parts[3], output))
                    throw std::exception{};
                else
                    add_extra(output, parts[4], '/');
                break;
            case 0:
            case 1:
            case 2:
                throw std::exception{};
            default:
                if (!check_subtype(parts[0], output) || !check_isolation(parts[2], output) || !check_year(parts[3], output))
                    throw std::exception{};
                if (parts.size() > 4) {
                    for (auto part{std::next(std::begin(parts), 4)}; part != std::end(parts); ++part)
                        add_extra(output, *part, '/');
                }
                break;
        }
    }
    catch (std::exception&) {
        output.messages.emplace_back("unexpected-location-part", fmt::format("1 {}", parts), MESSAGE_CODE_POSITION);
    }

} // acmacs::virus::name::one_location_part_at_1

// ----------------------------------------------------------------------

bool acmacs::virus::name::location_part_as_isolation_prefix(std::string_view isolation, parsed_fields_t& output)
{
    if (std::isdigit(isolation[0]))
        return false;

    for (auto prefix = acmacs::string::non_digit_prefix(isolation); prefix.size() > 2 && prefix.size() < isolation.size(); prefix.remove_suffix(1)) {
        // AD_DEBUG("location_part_as_isolation_prefix \"{}\" + \"{}\"", output.location, prefix);
        if (auto location_data_combined = location_lookup(fmt::format("{} {}", output.location, prefix)); good(location_data_combined)) { // "LYON CHU" <- A/Lyon/CHU19.03.77/2019
            set_location(output, std::move(get(location_data_combined)));
            check_isolation(isolation.substr(prefix.size()), output);
            return true;
        }
        else if (output.host.empty() && is_host(output.location)) {
            if (auto location_data_isolation = location_lookup(prefix); good(location_data_isolation)) { // "A/turkey/Italy12rs206-2/1999(H7N1)" -> A/Turkey/Italy/12rs206-2/1999(H7N1)
                check_host(output.location, output);
                set_location(output, std::move(get(location_data_isolation)));
                check_isolation(isolation.substr(prefix.size()), output);
                return true;
            }
        }
    }

    return false;

} // acmacs::virus::name::location_part_as_isolation_prefix

// ----------------------------------------------------------------------

void acmacs::virus::name::one_location_part_at_2(std::vector<std::string_view>& parts, parsed_fields_t& output)
{
    try {
        // AD_DEBUG("one_location_part_at_2: {}", parts);
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
                    if (auto location_data = location_lookup(string::join(acmacs::string::join_space, parts[2], parts[3])); good(location_data)) { // A/swine/Lyon/CHU/R19.03.77/2019
                        set_location(output, std::move(get(location_data)));
                        if (!check_subtype(parts[0], output) || !check_host(parts[1], output) || !check_isolation(parts[4], output))
                            throw std::exception{};
                    }
                    else if (!check_subtype(parts[0], output) || !check_host(parts[1], output) ||
                             !check_isolation(string::join(acmacs::string::join_dash, parts[3], parts[4]), output)) // A/chicken/CentralJava/Solo/VSN331/2013
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
        output.messages.emplace_back("unexpected-location-part", fmt::format("2 {}", parts), MESSAGE_CODE_POSITION);
    }

} // acmacs::virus::name::one_location_part_at_2

// ----------------------------------------------------------------------

void acmacs::virus::name::two_location_parts(std::vector<std::string_view>& parts, location_parts_t&& location_parts, parsed_fields_t& output)
{
    AD_LOG(acmacs::log::name_parsing, "TWO location part {}", parts);

    const auto double_location = [&](const acmacs::messages::code_position_t& code_pos) {
        output.messages.emplace_back("double-location", fmt::format("{} \"{}\"", location_parts, acmacs::string::join(acmacs::string::join_slash, parts)), code_pos);
    };

    if ((location_parts[0].part_no + 1) != location_parts[1].part_no) {
        double_location(MESSAGE_CODE_POSITION);
    }
    else if (is_host(parts[location_parts.front().part_no])) {
        one_location_part(parts, std::move(location_parts[1]), output);
    }
    else if (location_parts[0].location.name == location_parts[1].location.country) { // A/India/Delhi/DB106/2009 -> A/Delhi/DB106/2009
        parts.erase(std::next(parts.begin(), static_cast<ssize_t>(location_parts[0].part_no)));
        one_location_part(parts, {location_parts[1].part_no - 1, std::move(location_parts[1].location)}, output);
    }
    else if (location_parts[0].location.country == location_parts[1].location.name) { // A/Cologne/Germany/01/2009 -> A/Cologne/01/2009
        parts.erase(std::next(parts.begin(), static_cast<ssize_t>(location_parts[1].part_no)));
        one_location_part(parts, std::move(location_parts[0]), output);
    }
    else if (location_parts[0].location.country == location_parts[1].location.country) { // "B/Mount_Lebanon/Ain_W_zein/3/2019" -> "B/Mount Lebanon Ain W zein/3/2019"
        check_location(string::join(acmacs::string::join_space, location_parts[0].location.name, location_parts[1].location.name), output);
        parts.erase(std::next(parts.begin(), static_cast<ssize_t>(location_parts[1].part_no)));
        one_location_part(parts, location_part_t{.part_no=location_parts[0].part_no}, output);
    }
    else
        double_location(MESSAGE_CODE_POSITION);

} // acmacs::virus::name::two_location_parts

// ----------------------------------------------------------------------

inline std::string normalize_a_subtype(std::string_view source)
{
    // AD_DEBUG("normalize_a_subtype \"{}\"", source);
#include "acmacs-base/global-constructors-push.hh"
    static const std::regex re_full{"H\\d{1,2}/?N\\d{1,2}V?", acmacs::regex::icase | std::regex::nosubs},
        re_part{"[HN]\\d{1,2}", acmacs::regex::icase | std::regex::nosubs},
        re_h{"(H\\d{1,2})/?N[\\?\\-x]?", acmacs::regex::icase},
        re_n{"H[\\?\\-xo]?/?(N\\d{1,2})", acmacs::regex::icase},
        re_ignore{"(H[\\?\\-x]/?N[\\?\\-x]|[HN\\d]+\\?|H\\d+H\\d+)", acmacs::regex::icase | std::regex::nosubs};
#include "acmacs-base/diagnostics-pop.hh"

    if (std::regex_match(std::begin(source), std::end(source), re_full)) { // "H3N2" "H3/N2" "H1N2V"
        if (source[2] == '/')
            return fmt::format("{}{}", source.substr(0, 2), source.substr(3));
        else if (source[3] == '/')
            return fmt::format("{}{}", source.substr(0, 3), source.substr(4));
        else
            return std::string{source};
    }
    if (std::regex_match(std::begin(source), std::end(source), re_part)) // "H3", "N2"
        return std::string{source};
    if (std::cmatch match_hn; std::regex_match(std::begin(source), std::end(source), match_hn, re_h) || std::regex_match(std::begin(source), std::end(source), match_hn, re_n))
        return match_hn.str(1); // "H3N?" "H?N2" - either H or N known
    if (std::regex_match(std::begin(source), std::end(source), re_ignore)) // "HxNx", "H-N-", "H?N?" "H5N2?" "H3H2" - both are unknown
        return {};
    throw std::exception{};
}

bool acmacs::virus::name::check_subtype(std::string_view source, parsed_fields_t& output, make_message report)
{
    using namespace acmacs::regex;

    try {
        // AD_DEBUG("check_subtype \"{}\"", source);
        switch (source.size()) {
            case 0:
                break;
            case 1:
                switch (std::toupper(source[0])) {
                    case 'A':
                    case 'B':
                        output.subtype = type_subtype_t{::string::upper(source)};
                        break;
                    default:
                        throw std::exception{};
                }
                break;
            default:
                switch (std::toupper(source[0])) {
                    case 'A': {
                        // AD_DEBUG("check_subtype \"{}\"", source);
                        source.remove_prefix(1);
                        if (source[0] == '(' && source.back() == ')') {
                            source.remove_prefix(1);
                            source.remove_suffix(1);
                        }
                        if (const auto norm_subtype = normalize_a_subtype(source); !norm_subtype.empty())
                            output.subtype = type_subtype_t{fmt::format("A({})", norm_subtype)}; // may throw
                        else
                            output.subtype = type_subtype_t{"A"};
                        break;
                    }
                    case 'H':
                        if (source.size() > 3 && std::toupper(source[1]) == 'Y' && source[2] == ' ') // "HY A"
                            return check_subtype(source.substr(3), output, report);
                        else
                            throw std::exception{};
                    default:
                        throw std::exception{};
                }
                break;
        }
        return true;
    }
    catch (std::exception&) {
        // AD_ERROR("invalid_subtype \"{}\"", source);
        if (report == make_message::yes)
            output.messages.emplace_back(acmacs::messages::key::invalid_subtype, source, MESSAGE_CODE_POSITION);
        return false;
    }

} // acmacs::virus::name::check_subtype

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_host(std::string_view source, parsed_fields_t& output)
{
    using namespace std::string_view_literals;
    if (source.size() >= 4 && source.substr(0, 4) == "TEST"sv)
        output.messages.emplace_back(acmacs::messages::key::invalid_host, source, MESSAGE_CODE_POSITION);
    output.host = host_t{fix_host(::string::remove(source, "'\""))};
    return true;

} // acmacs::virus::name::check_host

// ----------------------------------------------------------------------

acmacs::virus::name::location_lookup_result_t acmacs::virus::name::location_lookup(std::string_view source)
{
    using namespace std::string_view_literals;

    const auto upcased{::string::upper(source)};
    if (upcased == "UNKNOWN"sv)
        return location_not_found_t{source};

    if (const auto loc = acmacs::locationdb::get().find(source, acmacs::locationdb::include_continent::yes); loc.has_value())
        return location_data_t{.name{loc->name}, .country{std::string{loc->country()}}, .continent{loc->continent}};

    // https://www.shabsin.com/~rshabsin/chineseutf8chars.html
    if (static_cast<unsigned char>(source[0]) >= 0xE3 && static_cast<unsigned char>(source[0]) < 0xEA) // chinese or possibly chinese symbol
        return location_chinese_name_t{source};

    using pp = std::pair<std::string_view, std::string_view>;
    static const std::array common_abbreviations{
        pp{"UK"sv, "UNITED KINGDOM"sv}, pp{"NY"sv, "NEW YORK"sv}, pp{"HK"sv, "HONG KONG"sv}, pp{"DE"sv, "GERMANY"sv}, pp{"TX"sv, "TEXAS"sv},
        // MN - Mongolia, Montenegro (obsolete ISO code), Minnesota
    };

    for (const auto& [e1, e2] : common_abbreviations) {
        if (e1 == upcased) {
            if (const auto loc = acmacs::locationdb::get().find(e2, acmacs::locationdb::include_continent::yes); loc.has_value())
                return location_data_t{.name{loc->name}, .country{std::string{loc->country()}}, .continent{loc->continent}};
        }
    }

    return location_not_found_t{source};

} // acmacs::virus::name::location_lookup

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_location(std::string_view source, parsed_fields_t& output)
{
    if (auto loc_enc = location_lookup(source); good(loc_enc)) {
        const auto& loc = get(loc_enc);
        output.location = loc.name;
        output.country = loc.country;
        output.continent = loc.continent;
        return true;
    }
    else {
        output.messages.emplace_back(acmacs::messages::key::location_not_found, source, MESSAGE_CODE_POSITION);
        return false;
    }

} // acmacs::virus::name::check_location

// ----------------------------------------------------------------------

acmacs::virus::name::location_parts_t acmacs::virus::name::find_location_parts(std::vector<std::string_view>& parts, acmacs::messages::messages_t& messages)
{
    location_parts_t location_parts;
    for (size_t part_no = 0; part_no < parts.size(); ++part_no) {
        std::visit(
            [&location_parts, part_no, &messages]<typename Arg>(Arg&& arg) {
                if constexpr (std::is_same_v<location_data_t, std::decay_t<Arg>>) {
                    location_parts.push_back({part_no, arg});
                }
                else if constexpr (std::is_same_v<location_chinese_name_t, std::decay_t<Arg>>) {
                    location_parts.push_back({part_no, arg});
                    messages.emplace_back(acmacs::messages::key::location_not_found, arg.name);
                }
            },
            location_lookup(parts[part_no]));
    }

    // if just one location part found, it is in place 0 or 1, next part starts with a letter, this location is perhaps a host (e.g. TURKEY)
    if (location_parts.size() == 1 && location_parts[0].part_no < 2 && location_parts[0].part_no < (parts.size() - 1) && is_host(location_parts[0].location.name)) {
        if (acmacs::string::non_digit_prefix(parts[location_parts[0].part_no + 1]).size() == parts[location_parts[0].part_no + 1].size())
            return {}; // location is most probably next part, but locdb cannot detect it
    }

    if (location_parts.size() > 2 && is_host(location_parts[0].location.name)) // A/Turkey/Bulgaria/Haskovo/336/2018
        location_parts.erase(location_parts.begin());

    return location_parts;

} // acmacs::virus::name::find_location_parts

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_isolation(std::string_view source, parsed_fields_t& output)
{
    using namespace std::string_view_literals;
    // AD_DEBUG("check_isolation \"{}\"", source);
    if (const auto skip_spaces_zeros = source.find_first_not_of(" 0"sv); skip_spaces_zeros != std::string_view::npos)
        output.isolation = ::string::upper(source.substr(skip_spaces_zeros));
    if (output.isolation.size() > 3 && output.isolation.substr(output.isolation.size() - 3) == "_HA") // isolation ending with _HA means HA segment in sequences from ncbi
        output.isolation.erase(output.isolation.size() - 3);
    if (output.isolation.empty()) {
        if (!source.empty()) {
            output.isolation = source;
            // output.messages.emplace_back(acmacs::messages::key::invalid_isolation, source, MESSAGE_CODE_POSITION);
        }
        else
            output.messages.emplace_back(acmacs::messages::key::isolation_absent, source, MESSAGE_CODE_POSITION);
    }
    else
        ::string::replace_in_place(output.isolation, '_', ' ');
    return true;

} // acmacs::virus::name::check_isolation

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_year(std::string_view source, parsed_fields_t& output, make_message report)
{
#include "acmacs-base/global-constructors-push.hh"
    static const auto current_year = date::current_year();
    static const auto current_year_2 = current_year % 100;
#include "acmacs-base/diagnostics-pop.hh"

    const auto digits = acmacs::string::digit_prefix(source);
    AD_LOG(acmacs::log::name_parsing, "check_year digits: \"{}\" <- \"{}\"", digits, source);

    try {
        if (paren_match(source) < 0) // e.g. last part in "A/Beijing/2019-15554/2018  CNIC-1902  (19/148)"
            throw std::exception{};
        switch (digits.size()) {
            case 1:
            case 2:
                if (const auto year = acmacs::string::from_chars<size_t>(digits); year <= current_year_2)
                    output.year = fmt::format("{}", year + 2000);
                else if (year < 100) // from_chars returns std::numeric_limits<size_t>::max() if number cannot be read
                    output.year = fmt::format("{}", year + 1900);
                else
                    throw std::exception{};
                break;
            case 4:
                if (const auto year = acmacs::string::from_chars<size_t>(digits); year <= current_year)
                    output.year = fmt::format("{}", year);
                else
                    throw std::exception{};
                break;
            default:
                throw std::exception{};
        }
        if (digits.size() < source.size())
            add_extra(output, source.substr(digits.size()));
        return true;
    }
    catch (std::exception&) {
        // AD_LOG(acmacs::log::name_parsing, "check_year ERROR in \"{}\" digits:\"{}\" digits-size:{}", source, digits, digits.size());
        if (report == make_message::yes)
            output.messages.emplace_back(acmacs::messages::key::invalid_year, fmt::format("\"{}\" <- \"{}\"", source, output.raw), MESSAGE_CODE_POSITION);
        return false;
    }

} // acmacs::virus::name::check_year

// ----------------------------------------------------------------------

std::string acmacs::virus::name::check_reassortant_in_front(std::string_view source, parsed_fields_t& output)
{
    std::string result, rest;
    std::tie(output.reassortant, result) = parse_reassortant(source);

    if (!result.empty() && result[0] == '(') { // "(Johannesburg/33/1994)(H3N2)" -> "Johannesburg/33/1994" + "(H3N2)"
        const std::string inside{acmacs::string::prefix_in_parentheses(result)};
        if ((inside.size() + 2) <= result.size())
            rest.assign(result.data() + inside.size() + 2, result.size() - inside.size() - 2);
        result.assign(inside);
    }

    if (!result.empty()) {
        if (result.front() == '(' && result.back() == ')') {
            result.erase(std::prev(result.end()));
            result.erase(result.begin());
        }
        else if (result.front() == '(' && result.find(')', 1) == std::string::npos) {
            result.erase(result.begin());
        }

        if (result.size() > 3 && !output.reassortant.empty() && result[0] == 'H' && result[1] == 'Y' && result[2] == ' ')
            result.erase(result.begin(), std::next(result.begin(), 3));

        // remove common reassortant parts to allow parsing the main virus name
        if (!result.empty() && !output.reassortant.empty()) {
            result = remove_reassortant_second_name(result);
            // AD_DEBUG("check_reassortant_in_front \"{}\" <-- \"{}\"", result, source);
        }

        if ((source[0] == 'A' || source[0] == 'B') && source[1] == '/' && result[1] != '/') {
            result.insert(0, source.substr(0, 2));
            // AD_DEBUG("reass in front \"{}\" <-- \"{}\" '{}' '{}' '{}'", result, source, source[0], source[1], result[1]);
        }
    }

    if (result.empty() && !output.reassortant.empty())
        output.messages.emplace_back(acmacs::messages::key::reassortant_without_name, source, MESSAGE_CODE_POSITION);

    // AD_DEBUG("check_reassortant_in_front \"{}\" -> \"{}\" R:\"{}\"", source, result, *output.reassortant);

    return fmt::format("{}{}", result, rest);

} // acmacs::virus::name::check_reassortant_in_front

// ----------------------------------------------------------------------

std::string acmacs::virus::name::remove_reassortant_second_name(std::string_view source)
{

#define RRSN_PR8 "(?:(?:A/)?Puerto +Rico/8/(?:19)34|PR8(?:\\s*\\([A-Z\\d\\-]+\\))?|A/PR/8/34(?:\\(H1N1\\))?)" // "A/Puerto Rico/8/1934" "PR8  (CNIC-HB29578)" "A/PR/8/34(H1N1)"
#define RRSN_LEE40 "(?:B/)?Lee/(?:19)40"
#define RRSN_TX77  "Texas/1/(?:19)?77"
#define RRSN_AA60  "Ann Arbor/6/(?:19)?60"
#define RRSN_PN90  "Panama/45/(?:19)?90"
#define RRSN_NYMC "(?:(?:NYMC )?B?X-\\d+[A-Z]+|NYMC-\\d+[A-Z]+)"

#define RRSN_SXS " x "

    using namespace acmacs::regex;
#include "acmacs-base/global-constructors-push.hh"
    static const std::array common_reassortant_names{
        look_replace_t{std::regex("(?:"
                                  RRSN_SXS RRSN_PR8                    "|"
                                  RRSN_PR8 RRSN_SXS                    "|"
                                  RRSN_PR8 "-"                         "|"
                                  RRSN_SXS RRSN_LEE40                  "|"
                                  RRSN_LEE40 RRSN_SXS                  "|"
                                  RRSN_SXS RRSN_LEE40 "\\s+" RRSN_NYMC "|"
                                  RRSN_SXS RRSN_NYMC                   "|"
                                  RRSN_SXS RRSN_TX77                   "|"
                                  RRSN_SXS RRSN_AA60                   "|"
                                  RRSN_PN90 RRSN_SXS                   "|"
                                  RRSN_NYMC RRSN_SXS
                                  ")", std::regex::icase), {"$`", "$'"}}
    };
#include "acmacs-base/diagnostics-pop.hh"

    if (const auto res = scan_replace(source, common_reassortant_names); res.has_value()) {
        const auto pre = acmacs::string::strip(res->front()), post = acmacs::string::strip(res->back());
        if (!post.empty() && post[0] == ')' && pre.find('(') == std::string::npos)
            return fmt::format("{}{}", pre, post.substr(1));
        else
            return fmt::format("{} {}", pre, post);
    }
    else
        return std::string{source};

} // acmacs::virus::name::remove_reassortant_second_name

// ----------------------------------------------------------------------

bool acmacs::virus::name::check_nibsc_extra(std::vector<std::string_view>& parts)
{
#include "acmacs-base/global-constructors-push.hh"
    static const std::regex re_1("\\s*\\(\\d*$", acmacs::regex::icase);
    static const std::regex re_2("^\\d*\\)\\s*", acmacs::regex::icase);
#include "acmacs-base/diagnostics-pop.hh"

    // ! Should not modify: "B/Phuket/3073/2013 (Ferret POOL / CSID 2014768619) BOOSTED"
    std::cmatch m1, m2;
    if (acmacs::regex::search(parts[parts.size() - 2], m1, re_1) && acmacs::regex::search(parts.back(), m2, re_2)) {
        // remove (19/148) in "A/Beijing/2019-15554/2018  CNIC-1902  (19/148)"
        AD_LOG(acmacs::log::name_parsing, "check_nibsc_extra {}", parts);
        parts[parts.size() - 2].remove_suffix(static_cast<size_t>(m1.length(0)));
        parts.back().remove_prefix(static_cast<size_t>(m2.length(0)));
        if (parts.back().empty())
            parts.pop_back();
        AD_LOG(acmacs::log::name_parsing, "check_nibsc_extra done {}", parts);
        return true;
    }
    else
        return false;

} // acmacs::virus::name::check_nibsc_extra

// ----------------------------------------------------------------------

void acmacs::virus::name::check_extra(parsed_fields_t& output)
{
    using namespace acmacs::regex;

    AD_LOG(acmacs::log::name_parsing, "check_extra \"{}\"", output.extra);
    if (!output.extra.empty()) {
        AD_LOG_INDENT;

        if (!output.extra.empty() && output.reassortant.empty()) {
            std::tie(output.reassortant, output.extra) = parse_reassortant(output.extra);
            AD_LOG(acmacs::log::name_parsing, "check_extra after extracting reassortant \"{}\"", output.extra);
        }

        if (!output.extra.empty() && output.mutations.empty()) {
            std::tie(output.mutations, output.extra) = parse_mutatations(output.extra);
            AD_LOG(acmacs::log::name_parsing, "check_extra after extracting mutations \"{}\"", output.extra);
        }

        if (!output.extra.empty() && output.passage.empty() && output.extract_passage_ == extract_passage::yes) {
            std::tie(output.passage, output.extra) = parse_passage(output.extra, passage_only::no);
            AD_LOG(acmacs::log::name_parsing, "check_extra after extracting passage \"{}\"", output.extra);
        }

#include "acmacs-base/global-constructors-push.hh"
        static const std::array normalize_data{
            look_replace_t{std::regex("(?:"
                                      "\\b(?:NEW)\\b"
                                      "|"
                                      "\\(MIXED(?:[\\.,][HN\\d]+)?\\)"
                                      ")",
                                      acmacs::regex::icase),
                           {"$` $'"}}, // NEW, (MIXED) - remove
            look_replace_t{std::regex("[\\(\\?]"
                                      "("
                                      "(?:H(?:\\d{1,2}|[XO\\?\\-]))?"
                                      "(?:[HN](?:\\d{1,2}|[X\\?\\-])?V?)?"
                                      "\\??"
                                      ")"
                                      "[\\)\\?]",
                                      acmacs::regex::icase),
                           {"$` $'", "$1"}}, // (H3N2) (H3N?) (H1N2V) (H1N1?) (H3) (H11N) ?H5N6? - subtype (in ? in gisaid)
            look_replace_t{std::regex("^(?:-LIKE|JANUARY|FEBRUARY|MARCH|APRIL|MAY|JUNE|JULY|AUGUST|SEPTEMBER|OCTOBER|NOVEMBER|DECEMBER)$", acmacs::regex::icase),
                           {"$` $'"}},                                                              // remove few common annotations (meaningless for us)
            look_replace_t{std::regex("^[_\\-\\s,\\.]+", acmacs::regex::icase), {"$'"}},            // remove meaningless prefixes used as separators in the name
            look_replace_t{std::regex("^[\\(\\)_/\\-\\s,\\.]+$", acmacs::regex::icase), {"$` $'"}}, // remove artefacts
            look_replace_t{std::regex("^\\((.+)\\)$", acmacs::regex::icase), {"$1"}},               // remove parentheses that enclose entire extra
        };

        static const std::array extra_for_reassortants{
            look_replace_t{std::regex("^/?high\\s+yield(?:ing)?(?:\\s+reassortant)?", acmacs::regex::icase), {"$'"}}, // remove meaningless prefixes used as separators in the name
        };

#include "acmacs-base/diagnostics-pop.hh"

        const auto fix_extra = [&output](const scan_replace_result_t& scan_replace_result) {
            if (scan_replace_result.has_value()) {
                // AD_DEBUG("check_extra {} {}", *res, output);
                output.extra = acmacs::string::strip(scan_replace_result->at(0));
                if (scan_replace_result->size() > 1 && !scan_replace_result->at(1).empty()) { // subtype
                    // AD_DEBUG("check_extra {} {}", *res, output.subtype);
                    if (output.subtype == type_subtype_t{"A"})
                        check_subtype(fmt::format("A({})", scan_replace_result->at(1)), output);
                }
            }
            return scan_replace_result.has_value();
        };

        while (!output.extra.empty()) {
            if (fix_extra(scan_replace(output.extra, normalize_data)) || (!output.reassortant.empty() && fix_extra(scan_replace(output.extra, extra_for_reassortants)))) {
                // pass
            }
            else
                break;
        }

        AD_LOG(acmacs::log::name_parsing, "check_extra done \"{}\"", output.extra);
    }

} // acmacs::virus::name::check_extra

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
