#pragma once

#include <vector>
#include <optional>

#include "acmacs-base/debug.hh"
#include "acmacs-base/uppercase.hh"
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
        using name_t = acmacs::uppercased<struct virus_name_tag>;
        using host_t = acmacs::uppercased<struct host_t_tag>;
        using lineage_t = acmacs::uppercased<struct lineage_tag>;

        // ----------------------------------------------------------------------

        std::string_view host(const name_t& name);
        std::string_view location(const name_t& name);
        std::string_view isolation(const name_t& name);
        std::optional<size_t> year(const name_t& name);

        // ----------------------------------------------------------------------

        class type_subtype_t
        {
          public:
            type_subtype_t() = default;
            template <typename T> explicit constexpr type_subtype_t(T&& value) : value_(std::forward<T>(value)) {}

            constexpr operator const std::string&() const { return value_; }
            operator std::string_view() const { return value_; }
            constexpr const std::string& operator*() const { return value_; }
            constexpr const std::string* operator->() const { return &value_; }
            constexpr const std::string& get() const { return value_; }
            constexpr std::string& get() { return value_; }

            bool operator==(const type_subtype_t& rhs) const { return value_ == rhs.value_; }
            bool operator!=(const type_subtype_t& rhs) const { return !operator==(rhs); }
            bool operator<(const type_subtype_t& rhs) const { return value_ < rhs.value_; }
            int compare(const type_subtype_t& rhs) const { return ::string::compare(value_, rhs.value_); }
            bool empty() const { return value_.empty(); }
            size_t size() const { return value_.size(); }

            // returns part of the type_subtype: B for B, H1 for A(H1...), etc.
            std::string_view h_or_b() const
            {
                if (!value_.empty()) {
                    switch (value_[0]) {
                        case 'B':
                            return std::string_view(value_.data(), 1);
                        case 'A':
                            if (value_.size() > 4) {
                                switch (value_[4]) {
                                  case 'N':
                                  case ')':
                                      return std::string_view(value_.data() + 2, 2);
                                  default:
                                      return std::string_view(value_.data() + 2, 3);
                                }
                            }
                    }
                }
                return value_;
            }

            char type() const
            {
                if (value_.empty())
                    return '?';
                else
                    return value_[0];
            }

          private:
            std::string value_;

            // friend inline std::ostream& operator<<(std::ostream& out, const type_subtype_t& ts) { return out << ts.value_; }
        };

        // ----------------------------------------------------------------------

        enum parse_name_f {
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
                constexpr static inline const char* unrecognized = "unrecognized";
                constexpr static inline const char* unrecognized_passage = "unrecognized-passage";
                constexpr static inline const char* location_not_found = "location-not-found";
                constexpr static inline const char* invalid_year = "invalid-year";
                constexpr static inline const char* isolation_absent = "isolation-absent";
                constexpr static inline const char* invalid_host = "invalid-host";

                const char* key;
                std::string value;
                std::string suppliment;

                // message_t(const char* a_key, std::string a_value) : key(a_key), value(a_value) {}
                message_t(const message_t&) = default;
                message_t(const char* a_key, std::string_view a_value) : key{a_key}, value{a_value} {}
                message_t(const char* a_key, std::string_view a_value, std::string_view a_suppliment) : key{a_key}, value{a_value}, suppliment{a_suppliment} {}
                message_t(const char* a_key = unrecognized) : key(a_key) {}
                bool operator==(const char* a_key) const { return std::string_view(key) == a_key; }
                bool operator==(const message_t& rhs) const { return std::string_view(key) == rhs.key && value == rhs.value; }
                // friend inline std::ostream& operator<<(std::ostream& out, const message_t& msg) { return out << msg.key << ": \"" << msg.value << '"'; }
            };

            name_t name;
            host_t host;
            acmacs::virus::Reassortant reassortant;
            acmacs::virus::Passage passage;
            std::string extra;
            std::string country;
            std::string continent;
            std::vector<message_t> messages{};
        };

        parse_result_t parse_name(std::string_view source, parse_name_f flags = parse_name_f::lookup_location | parse_name_f::remove_extra_subtype, acmacs::debug dbg = acmacs::debug::no);
        void set_type_subtype(name_t& name, const type_subtype_t& type_subtype);

    } // namespace v2
} // namespace acmacs::virus

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::virus::type_subtype_t> : public fmt::formatter<acmacs::fmt_default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::type_subtype_t& ts, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string_view>(ts)); }
};

template <> struct fmt::formatter<acmacs::virus::parse_result_t::message_t> : public fmt::formatter<acmacs::fmt_default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::parse_result_t::message_t& msg, FormatContext& ctx) {
        format_to(ctx.out(), "{}: \"{}\"", msg.key, msg.value);
        if (!msg.suppliment.empty())
            format_to(ctx.out(), " ({})", msg.suppliment);
        return ctx.out();
    }
};

template <> struct fmt::formatter<acmacs::virus::parse_result_t> : public fmt::formatter<acmacs::fmt_default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::parse_result_t& res, FormatContext& ctx)
    {
        format_to(ctx.out(), "{}", res.name);
        if (!res.reassortant.empty())
            format_to(ctx.out(), " R:\"{}\"", res.reassortant);
        if (!res.extra.empty())
            format_to(ctx.out(), " E:\"{}\"", res.extra);
        if (!res.passage.empty())
            format_to(ctx.out(), " P:\"{}\"", res.passage);
        if (!res.host.empty())
            format_to(ctx.out(), " H:\"{}\"", res.host);
        if (!res.messages.empty())
            format_to(ctx.out(), " msg:{}", res.messages);
        return ctx.out();
    }
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
