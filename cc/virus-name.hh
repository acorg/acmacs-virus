#pragma once

#include <vector>
#include <optional>

#include "acmacs-base/named-type.hh"
#include "acmacs-base/fmt.hh"
#include "acmacs-virus/virus.hh"
#include "acmacs-virus/passage.hh"
#include "acmacs-virus/reassortant.hh"

#include "acmacs-virus/virus-name-v1.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus
{
    inline namespace v2
    {
        // normalized or user approved virus name
        using virus_name_t = named_t<std::string, struct virus_name_tag>;
        using host_t = named_t<std::string, struct host_t_tag>;

        std::optional<size_t> year(const virus_name_t& name);

        enum parse_name_f
        {
            none = 0,
            lookup_location = 1 << 0,
            remove_extra_subtype = 1 << 1 // remove (H3N2) at the end of the name, do not put it in extra
        };
        inline constexpr parse_name_f operator|(parse_name_f lh, parse_name_f rh) { return parse_name_f(int(lh) | int(rh)); }
        inline constexpr parse_name_f operator&(parse_name_f lh, parse_name_f rh) { return parse_name_f(int(lh) & int(rh)); }

        struct parse_result_t
        {
            struct message_t
            {
                const char* key;
                std::string value;

                message_t(const char* a_key, std::string a_value) : key(a_key), value(a_value) {}
                constexpr static inline const char* unrecognized = "unrecognized";
                constexpr static inline const char* unrecognized_passage = "unrecognized-passage";
                constexpr static inline const char* location_not_found = "location-not-found";
                constexpr static inline const char* invalid_year = "invalid-year";
                bool operator==(const char* a_key) const { return std::string_view(key) == a_key; }
                friend inline std::ostream& operator<<(std::ostream& out, const message_t& msg) { return out << msg.key << ": \"" << msg.value << '"'; }
            };

            virus_name_t name;
            host_t host;
            acmacs::virus::Reassortant reassortant;
            acmacs::virus::Passage passage;
            std::string extra;
            std::vector<message_t> messages{};
        };

        parse_result_t parse_name(std::string_view source, parse_name_f flags = parse_name_f::lookup_location|parse_name_f::remove_extra_subtype);
    }
}

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::virus::parse_result_t::message_t>
{
  template <typename ParseContext> constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }
  template <typename FormatContext> auto format(const acmacs::virus::parse_result_t::message_t& msg, FormatContext& ctx) { return format_to(ctx.out(), "{}: \"{}\"", msg.key, msg.value); }
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
