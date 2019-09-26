#pragma once

#include <iostream>
#include <regex>

#include "acmacs-base/named-type.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus
{
    class Passage : public named_string_t<struct virus_passage_tag_t>
    {
      public:
        using acmacs::named_string_t<struct virus_passage_tag_t>::named_string_t;

        bool is_egg() const;
        bool is_cell() const;
        std::string without_date() const;
        const char* passage_type() const { return is_egg() ? "egg" : "cell"; }

    }; // class Passage

    using parse_passage_t = std::tuple<Passage, std::string>;
    enum class passage_only { no, yes };

    parse_passage_t parse_passage(std::string_view source, passage_only po);

    inline bool passages_match(const Passage& p1, const Passage& p2)
    {
        return p1.is_egg() == p2.is_egg();
    }

} // namespace acmacs::virus

// ----------------------------------------------------------------------

template<> struct fmt::formatter<acmacs::virus::Passage> : fmt::formatter<std::string> {
    template <typename FormatCtx> auto format(const acmacs::virus::Passage& passage, FormatCtx& ctx) { return fmt::formatter<std::string>::format(passage.get(), ctx); }
};

namespace acmacs
{
    inline std::string to_string(const acmacs::virus::Passage& passage) { return *passage; }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
