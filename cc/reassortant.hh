#pragma once

#include <iostream>
#include <string>

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Reassortant
    {
     public:
        Reassortant() = default;
        template <typename T> explicit constexpr Reassortant(T&& value) : value_(std::forward<T>(value)) {}

        constexpr operator const std::string&() const { return value_; }
        // operator std::string() const { return value_; }
        constexpr const std::string& operator*() const { return value_; }
        constexpr const std::string& get() const { return value_; }

        bool operator==(const Reassortant& rhs) const { return value_ == rhs.value_; }
        bool operator!=(const Reassortant& rhs) const { return !operator==(rhs); }
        bool operator<(const Reassortant& rhs) const { return value_ < rhs.value_; }
        int compare(const Reassortant& rhs) const { return ::string::compare(value_, rhs.value_); }

        constexpr bool empty() const { return value_.empty(); }
        constexpr size_t size() const { return value_.size(); }

      private:
        std::string value_;

        friend inline std::ostream& operator<<(std::ostream& out, const Reassortant& reassortant) { return out << reassortant.value_; }

    }; // class Reassortant

} // namespace acmacs::chart

// ----------------------------------------------------------------------

namespace acmacs
{
    inline std::string to_string(const acmacs::chart::Reassortant& reassortant) { return *reassortant; }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
