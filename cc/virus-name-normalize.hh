#pragma once

#include "acmacs-virus/virus-name.hh"
#include "acmacs-virus/parsing-message.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus::inline v2::name
{
    enum class warn_on_empty { no, yes };
    enum class extract_passage { no, yes };

    struct parsed_fields_t
    {
        std::string raw;
        type_subtype_t subtype{};
        host_t host{};
        std::string location{};
        std::string isolation{};
        std::string year{};
        Reassortant reassortant{};
        Passage passage{};
        mutations_t mutations{};
        // aa_substitutions
        std::string extra{};
        std::string country{};
        std::string continent{};
        acmacs::messages::messages_t messages{};

        extract_passage extract_passage_{extract_passage::yes};

        bool good() const noexcept { return !location.empty() && !isolation.empty() && year.size() == 4; }
        bool good_but_no_country() const noexcept { return good() && country.empty(); }
        bool not_good() const noexcept { return !good() || country.empty(); }
        bool reassortant_only() const { return location.empty() && isolation.empty() && year.empty() && !reassortant.empty(); }
        name_t name() const noexcept;
        std::string full_name() const noexcept;
    };

    parsed_fields_t parse(std::string_view source, warn_on_empty woe = warn_on_empty::yes, extract_passage ep = extract_passage::yes);
    // std::vector<std::string> possible_locations_in_name(std::string_view source);

    inline bool is_good(std::string_view source) { return parse(source, warn_on_empty::no).good(); }

} // namespace acmacs::virus::inline v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::virus::mutations_t> : public fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::mutations_t& mutations, FormatContext& ctx)
    {
        fmt::format_to(ctx.out(), "[");
        bool first{true};
        for (const auto& mut : mutations) {
            if (first)
                first = false;
            else
                fmt::format_to(ctx.out(), ", ");
            fmt::format_to(ctx.out(), "\"{}\"", mut);
        }
        return fmt::format_to(ctx.out(), "]");
    }
};

template <> struct fmt::formatter<acmacs::virus::name::parsed_fields_t> : public fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::name::parsed_fields_t& fields, FormatContext& ctx)
    {
        fmt::format_to(ctx.out(), "{{\"{}\" \"{}\" \"{}\" \"{}\" \"{}\"", fields.subtype, fields.host, fields.location, fields.isolation, fields.year);
        if (!fields.extra.empty())
            fmt::format_to(ctx.out(), " A:\"{}\"", fields.extra);
        if (!fields.reassortant.empty())
            fmt::format_to(ctx.out(), " R:\"{}\"", fields.reassortant);
        if (!fields.passage.empty())
            fmt::format_to(ctx.out(), " P:\"{}\"", fields.passage);
        if (!fields.mutations.empty())
            fmt::format_to(ctx.out(), " M:{}", fields.mutations);
        if (!fields.country.empty())
            fmt::format_to(ctx.out(), " {{\"{}\" {}}}", fields.country, fields.continent);
        return fmt::format_to(ctx.out(), "}}");
    }
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
