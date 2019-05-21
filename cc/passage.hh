#pragma once

#include <iostream>
#include <regex>

#include "acmacs-base/string.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus
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
        bool empty() const { return value_.empty(); }
        size_t size() const { return value_.size(); }

        bool is_egg() const;
        bool is_cell() const;
        std::string without_date() const;
        const char* passage_type() const { return is_egg() ? "egg" : "cell"; }

        int compare(const Passage& rhs) const { return ::string::compare(value_, rhs.value_); }

      private:
        std::string value_;

        friend inline std::ostream& operator<<(std::ostream& out, const Passage& passage) { return out << passage.value_; }

    }; // class Passage

    using parse_passage_t = std::tuple<Passage, std::string>;
    parse_passage_t parse_passage(std::string_view source);

} // namespace acmacs::virus

// ----------------------------------------------------------------------

namespace acmacs
{
    inline std::string to_string(const acmacs::virus::Passage& passage) { return *passage; }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
