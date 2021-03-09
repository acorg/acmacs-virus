#pragma once

#include "acmacs-base/named-type.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus
{
    class Reassortant : public named_string_t<struct virus_reassortant_tag_t>
    {
     public:
        using acmacs::named_string_t<struct virus_reassortant_tag_t>::named_string_t;


    }; // class Reassortant

    std::tuple<Reassortant, std::string> parse_reassortant(std::string_view source);

} // namespace acmacs::virus

// ----------------------------------------------------------------------

namespace acmacs
{
    inline std::string to_string(const acmacs::virus::Reassortant& reassortant) { return *reassortant; }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
