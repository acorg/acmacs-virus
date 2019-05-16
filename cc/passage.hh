#pragma once

#include <iostream>
#include <regex>

#include "acmacs-base/string.hh"

// ----------------------------------------------------------------------

namespace acmacs::passage
{
    namespace _internal
    {
        constexpr const char* re_egg = R"#(((E|SPF(CE)?|SPE)(\?|[0-9][0-9]?)|EGG))#";
        constexpr const char* re_cell = R"#((MDCK|SIAT|QMC|MK|CKC|CEK|CACO|LLC|LLK|PRMK|MEK|C|SPFCK)(\?|[0-9][0-9]?))#";
        constexpr const char* re_nimr_isolate = R"#(( (ISOLATE|CLONE) [0-9\-]+)*)#"; // NIMR isolate and/or clone, NIMR H1pdm has CLONE 38-32
        constexpr const char* re_niid_plus_number = R"#(( *\+[1-9])?)#"; // NIID has +1 at the end of passage
        constexpr const char* re_passage_date = R"#(( \([12][0129][0-9][0-9]-[01][0-9]-[0-3][0-9]\))?)#"; // passage date
    }

    inline bool is_egg(std::string aPassage)
    {
#include "acmacs-base/global-constructors-push.hh"
        using namespace _internal;
        static std::regex egg_passage{std::string(re_egg) + re_nimr_isolate +  re_niid_plus_number + re_passage_date}; // NIMR has "EGG 10-6" in h3-neut
#include "acmacs-base/diagnostics-pop.hh"
        return std::regex_search(aPassage, egg_passage);

    } // is_egg

// ----------------------------------------------------------------------

    inline bool is_cell(std::string aPassage)
    {
#include "acmacs-base/global-constructors-push.hh"
        using namespace _internal;
        static std::regex cell_passage{std::string(re_cell) + re_nimr_isolate +  re_niid_plus_number + re_passage_date};
#include "acmacs-base/diagnostics-pop.hh"
        return std::regex_search(aPassage, cell_passage);

    } // is_cell

// ----------------------------------------------------------------------

    inline std::string without_date(std::string aPassage)
    {
        if (aPassage.size() > 13 && aPassage[aPassage.size() - 1] == ')' && aPassage[aPassage.size() - 12] == '(' && aPassage[aPassage.size() - 13] == ' ' && aPassage[aPassage.size() - 4] == '-' && aPassage[aPassage.size() - 7] == '-')
            return std::string(aPassage, 0, aPassage.size() - 13);
        else
            return aPassage;
    }

// ----------------------------------------------------------------------

    enum class CellOrEgg {Unknown, Cell, Egg, CellAndEgg};

    inline CellOrEgg cell_or_egg(std::string aPassage)
    {
        return is_egg(aPassage) ? CellOrEgg::Egg : (is_cell(aPassage) ? CellOrEgg::Cell : CellOrEgg::CellAndEgg); // OR is CellAndEgg
    }

    inline CellOrEgg cell_or_egg(const std::vector<std::string>& variants)
    {
        CellOrEgg r = CellOrEgg::Unknown;
        for (const auto& passage: variants) {
            CellOrEgg p = cell_or_egg(passage);
            if (r == CellOrEgg::Unknown)
                r = p;
            else if (p != r) {
                r = CellOrEgg::CellAndEgg;
                break;
            }
        }
        return r == CellOrEgg::Unknown ? CellOrEgg::CellAndEgg : r; // unknown (or no passage) gives CellOrEgg::CellAndEgg to match anything
    }

    inline bool match_cell_egg(CellOrEgg a, CellOrEgg b)
    {
        return a == b || a == CellOrEgg::CellAndEgg || b == CellOrEgg::CellAndEgg;

    }

} // namespace acmacs::passage

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Passage
    {
      public:
        Passage() = default;
        template <typename T> explicit constexpr Passage(T&& value) : value_(std::forward<T>(value)) {}

        constexpr operator const std::string&() const { return value_; }
        // operator std::string() const { return value_; }
        constexpr const std::string& operator*() const { return value_; }
        constexpr const std::string& get() const { return value_; }

        bool operator==(const Passage& rhs) const { return value_ == rhs.value_; }
        bool operator!=(const Passage& rhs) const { return !operator==(rhs); }
        bool operator<(const Passage& rhs) const { return value_ < rhs.value_; }
        constexpr bool empty() const { return value_.empty(); }
        constexpr size_t size() const { return value_.size(); }

        bool is_egg() const { return acmacs::passage::is_egg(value_); }
        bool is_cell() const { return acmacs::passage::is_cell(value_); }
        std::string without_date() const { return acmacs::passage::without_date(value_); }
        const char* passage_type() const { return is_egg() ? "egg" : "cell"; }

        int compare(const Passage& rhs) const { return ::string::compare(value_, rhs.value_); }

      private:
        std::string value_;

        friend inline std::ostream& operator<<(std::ostream& out, const Passage& passage) { return out << passage.value_; }

    }; // class Passage
} // namespace acmacs::chart

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
