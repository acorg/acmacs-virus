#pragma once

#include "acmacs-base/named-type.hh"
#include "acmacs-base/regex.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus
{
    class Passage : public named_string_t<struct virus_passage_tag_t>
    {
      public:
        using acmacs::named_string_t<struct virus_passage_tag_t>::named_string_t;

        bool is_egg() const;
        bool is_cell() const;
        std::string_view without_date() const;
        std::string_view last_number() const; // E2/E3 -> 3, X? -> ?
        std::string_view last_type() const; // MDCK3/SITA1 -> SIAT

        std::string_view passage_type() const
        {
            using namespace std::string_view_literals;
            if (is_egg())
                return "egg"sv;
            else
                return "cell"sv;
        }

        size_t find(std::string_view look_for) const { return get().find(look_for); }
        bool search(const std::regex& re) const { return std::regex_search(get(), re); }

    }; // class Passage

    using parse_passage_t = std::tuple<Passage, std::string>;
    enum class passage_only { no, yes };

    parse_passage_t parse_passage(std::string_view source, passage_only po);

    inline bool passages_match(const Passage& p1, const Passage& p2)
    {
        return p1.is_egg() == p2.is_egg();
    }

    inline bool is_good_passage(std::string_view source)
    {
        const auto [passage, extra] = parse_passage(source, passage_only::no);
        return !passage->empty() && extra.empty();
    }

    int passage_compare(const Passage& p1, const Passage& p2);

} // namespace acmacs::virus

// ----------------------------------------------------------------------
