#pragma once

#include <iostream>
#include <string>

#include "acmacs-base/string.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus
{
    class Reassortant
    {
     public:
        Reassortant() = default;
        template <typename T> explicit constexpr Reassortant(T&& value) : value_(std::forward<T>(value)) {}

        constexpr operator std::string_view() const { return value_; }
        constexpr const std::string& operator*() const { return value_; }
        constexpr const std::string& get() const { return value_; }

        bool operator==(const Reassortant& rhs) const { return value_ == rhs.value_; }
        bool operator!=(const Reassortant& rhs) const { return !operator==(rhs); }
        bool operator<(const Reassortant& rhs) const { return value_ < rhs.value_; }
        int compare(const Reassortant& rhs) const { return ::string::compare(value_, rhs.value_); }
        bool empty() const { return value_.empty(); }
        size_t size() const { return value_.size(); }

      private:
        std::string value_;

        friend inline std::ostream& operator<<(std::ostream& out, const Reassortant& reassortant) { return out << reassortant.value_; }

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
