#pragma once

#include "acmacs-base/fmt.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus::inline v2::name
{
    struct parsing_message_t
    {
        constexpr static inline const char* unrecognized = "unrecognized";
        constexpr static inline const char* unrecognized_passage = "unrecognized-passage";
        constexpr static inline const char* location_not_found = "location-not-found";
        constexpr static inline const char* invalid_year = "invalid-year";
        constexpr static inline const char* isolation_absent = "isolation-absent";
        constexpr static inline const char* invalid_host = "invalid-host";

        const char* key;
        std::string value;
        std::string suppliment;

        // parsing_message_t(const char* a_key, std::string a_value) : key(a_key), value(a_value) {}
        parsing_message_t(const parsing_message_t&) = default;
        parsing_message_t(const char* a_key, std::string_view a_value) : key{a_key}, value{a_value} {}
        parsing_message_t(const char* a_key, std::string_view a_value, std::string_view a_suppliment) : key{a_key}, value{a_value}, suppliment{a_suppliment} {}
        parsing_message_t(const char* a_key = unrecognized) : key(a_key) {}
        bool operator==(const char* a_key) const { return std::string_view(key) == a_key; }
        bool operator==(const parsing_message_t& rhs) const { return std::string_view(key) == rhs.key && value == rhs.value; }
    };

    using parsing_messages_t = std::vector<name::parsing_message_t>;

}

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::virus::name::parsing_message_t> : public fmt::formatter<acmacs::fmt_default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::name::parsing_message_t& msg, FormatContext& ctx)
    {
        format_to(ctx.out(), "{}: \"{}\"", msg.key, msg.value);
        if (!msg.suppliment.empty())
            format_to(ctx.out(), " ({})", msg.suppliment);
        return ctx.out();
    }
};

template <> struct fmt::formatter<acmacs::virus::name::parsing_messages_t> : public fmt::formatter<acmacs::fmt_default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::name::parsing_messages_t& messages, FormatContext& ctx)
    {
        for (const auto& msg : messages)
            format_to(ctx.out(), "{}\n", msg);
        return ctx.out();
    }
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
