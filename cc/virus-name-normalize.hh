#pragma once

#include "acmacs-virus/virus-name.hh"
#include "acmacs-virus/parsing-message.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus::inline v2::name
{
    struct parsed_fields_t
    {
        std::string raw;
        type_subtype_t subtype;
        host_t host;
        std::string location;
        std::string isolation;
        std::string year;
        Reassortant reassortant;
        Passage passage;
        // aa_substitutions
        std::string extra;
        std::string country;
        std::string continent;
        parsing_messages_t messages;

        bool good() const { return !location.empty() && !isolation.empty() && year.size() == 4; }
        name_t name() const;
    };

    enum class warn_on_empty { no, yes };

    parsed_fields_t parse(std::string_view source, warn_on_empty woe = warn_on_empty::yes);

} // namespace acmacs::virus::inline v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::virus::name::parsed_fields_t> : public fmt::formatter<acmacs::fmt_default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::name::parsed_fields_t& fields, FormatContext& ctx)
    {
        format_to(ctx.out(), "{{\"{}\" \"{}\" \"{}\" \"{}\" \"{}\"", fields.subtype, fields.host, fields.location, fields.isolation, fields.year);
        if (!fields.extra.empty())
            format_to(ctx.out(), " <{}>", fields.extra);
        if (!fields.reassortant.empty())
            format_to(ctx.out(), " \"{}\"", fields.reassortant);
        if (!fields.passage.empty())
            format_to(ctx.out(), " \"{}\"", fields.passage);
        if (!fields.country.empty())
            format_to(ctx.out(), " {{\"{}\" {}}}", fields.country, fields.continent);
        return format_to(ctx.out(), "}}");
    }
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
