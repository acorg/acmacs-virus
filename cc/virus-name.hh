#pragma once

#include <vector>

#include "acmacs-base/named-type.hh"

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

        enum parse_name_f
        {
            none = 0,
            lookup_location = 1 << 0
        };
        inline constexpr parse_name_f operator|(parse_name_f lh, parse_name_f rh) { return parse_name_f(int(lh) | int(rh)); }
        inline constexpr parse_name_f operator&(parse_name_f lh, parse_name_f rh) { return parse_name_f(int(lh) & int(rh)); }

        struct parse_result_t
        {
            virus_name_t name;
            acmacs::virus::Reassortant reassortant;
            acmacs::virus::Passage passage;
            std::string extra;
            std::vector<std::string> messages{};
        };

        parse_result_t parse_name(std::string_view source, parse_name_f flags = parse_name_f::lookup_location);
    }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
