#pragma once

#include <tuple>

#include "acmacs-base/named-type.hh"

#include "acmacs-virus/passage.hh"
#include "acmacs-virus/reassortant.hh"

#include "acmacs-virus/virus-name-v1.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus_name
{
    inline namespace v2
    {
        // normalized or user approved virus name
        using virus_name_t = named_t<std::string, struct virus_name_tag>;

        struct Error : public std::runtime_error { using std::runtime_error::runtime_error; };
        
        std::tuple<virus_name_t, acmacs::virus::Reassortant, acmacs::virus::Passage, std::string> parse_name(std::string_view source);
    }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
