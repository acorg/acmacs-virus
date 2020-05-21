#pragma once

#error obsolete, use "acmacs-virus/virus-name-normalize.hh"

#include "acmacs-virus/virus-name.hh"
#include "acmacs-virus/parsing-message.hh"

namespace acmacs::virus::inline v2
{
    enum parse_name_f {
        none = 0,
        lookup_location = 1 << 0,
        remove_extra_subtype = 1 << 1 // remove (H3N2) at the end of the name, do not put it in extra
    };
    inline constexpr parse_name_f operator|(parse_name_f lh, parse_name_f rh) { return parse_name_f(int(lh) | int(rh)); }
    inline constexpr parse_name_f operator&(parse_name_f lh, parse_name_f rh) { return parse_name_f(int(lh) & int(rh)); }

    struct parse_result_t
    {
        name_t name;
        host_t host;
        acmacs::virus::Reassortant reassortant;
        acmacs::virus::Passage passage;
        std::string extra;
        std::string country;
        std::string continent;
        std::vector<name::parsing_message_t> messages{};
    };

    parse_result_t parse_name(std::string_view source, parse_name_f flags = parse_name_f::lookup_location | parse_name_f::remove_extra_subtype, acmacs::debug dbg = acmacs::debug::no);

} // namespace acmacs::virus::inline v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::virus::parse_result_t> : public fmt::formatter<acmacs::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::parse_result_t& res, FormatContext& ctx)
    {
        format_to(ctx.out(), "\"{}\"", res.name);
        if (!res.reassortant.empty())
            format_to(ctx.out(), " R:\"{}\"", res.reassortant);
        if (!res.extra.empty())
            format_to(ctx.out(), " <{}>", res.extra);
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
