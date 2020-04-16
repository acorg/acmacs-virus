#pragma once

#include <map>

#include "acmacs-base/fmt.hh"
#include "acmacs-base/messages.hh"

// ----------------------------------------------------------------------

namespace acmacs::messages::inline v1
{
    namespace key
    {
        constexpr static inline std::string_view empty_name{"~~empty-name"};
        constexpr static inline std::string_view invalid_subtype{"invalid-subtype"};
        constexpr static inline std::string_view invalid_host{"invalid-host"};
        constexpr static inline std::string_view location_not_found{"location-not-found"};
        constexpr static inline std::string_view location_field_not_found{"~location-field-not-found"};
        constexpr static inline std::string_view isolation_absent{"isolation-absent"};
        constexpr static inline std::string_view invalid_isolation{"invalid-isolation"};
        constexpr static inline std::string_view invalid_year{"invalid-year"};
        constexpr static inline std::string_view unrecognized_passage{"unrecognized-passage"};
        constexpr static inline std::string_view reassortant_without_name{"reassortant-without-name"};
        constexpr static inline std::string_view location_or_host{"+location-or-host"};
    }
}

namespace acmacs::virus::inline v2::name
{
    void report_by_type(acmacs::messages::messages_t& messages);
}


//     struct parsing_message_t
//     {
//         constexpr static inline std::string_view unrecognized{"unrecognized"};

//         std::string_view key;
//         std::string value;
//         // std::string suppliment;

//         parsing_message_t(const parsing_message_t&) = default;
//         parsing_message_t(std::string_view a_key, std::string_view a_value) : key{a_key}, value{a_value} {}
//         // parsing_message_t(std::string_view a_key, std::string_view a_value, std::string_view a_suppliment) : key{a_key}, value{a_value}, suppliment{a_suppliment} {}
//         parsing_message_t(std::string_view a_key = unrecognized) : key(a_key) {}
//         bool operator==(std::string_view a_key) const { return key == a_key; }
//         bool operator==(const parsing_message_t& rhs) const { return key == rhs.key && value == rhs.value; }
//     };

//     using parsing_messages_t = std::vector<name::parsing_message_t>;

//     using parsing_messages_by_key_t = std::map<std::string_view, std::vector<std::pair<std::string, std::string>>>;

//     void merge(parsing_messages_by_key_t& target, parsing_messages_t&& new_messages, std::string_view source);
//     void report(parsing_messages_by_key_t& messages);

// } // namespace acmacs::virus::inline v2::name

// // ----------------------------------------------------------------------

// template <> struct fmt::formatter<acmacs::virus::name::parsing_message_t> : public fmt::formatter<acmacs::fmt_default_formatter>
// {
//     template <typename FormatContext> auto format(const acmacs::virus::name::parsing_message_t& msg, FormatContext& ctx)
//     {
//         format_to(ctx.out(), "{}: \"{}\"", msg.key, msg.value);
//         // if (!msg.suppliment.empty())
//         //     format_to(ctx.out(), " ({})", msg.suppliment);
//         return ctx.out();
//     }
// };

// template <> struct fmt::formatter<acmacs::virus::name::parsing_messages_t> : public fmt::formatter<acmacs::fmt_default_formatter>
// {
//     template <typename FormatContext> auto format(const acmacs::virus::name::parsing_messages_t& messages, FormatContext& ctx)
//     {
//         bool first = true;
//         for (const auto& msg : messages) {
//             if (first)
//                 first = false;
//             else
//                 format_to(ctx.out(), "\n");
//             format_to(ctx.out(), "{}", msg);
//         }
//         return ctx.out();
//     }
// };

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
