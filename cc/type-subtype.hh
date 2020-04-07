#pragma once

#include "acmacs-base/fmt.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus::inline v2
{
    class type_subtype_t
    {
      public:
        type_subtype_t() = default;
        explicit type_subtype_t(std::string_view src) : value_{src} {}
        // template <typename T> explicit constexpr type_subtype_t(T&& value) : value_(std::forward<T>(value)) {}

        // constexpr operator const std::string &() const { return value_; }
        operator std::string_view() const { return value_; }
        std::string_view operator*() const { return value_; }
        // constexpr const std::string& operator*() const { return value_; }
        // constexpr const std::string* operator->() const { return &value_; }
        // constexpr const std::string& get() const { return value_; }
        // constexpr std::string& get() { return value_; }

        bool operator==(const type_subtype_t& rhs) const { return value_ == rhs.value_; }
        // bool operator!=(const type_subtype_t& rhs) const { return !operator==(rhs); }
        bool operator<(const type_subtype_t& rhs) const { return value_ < rhs.value_; }
        // int compare(const type_subtype_t& rhs) const { return ::string::compare(value_, rhs.value_); }

        bool empty() const { return value_.empty(); }
        size_t size() const { return value_.size(); }

        // returns part of the type_subtype: B for B, H1 for A(H1...), etc.
        std::string_view h_or_b() const
        {
            if (!value_.empty()) {
                switch (value_[0]) {
                    case 'B':
                        return std::string_view(value_.data(), 1);
                    case 'A':
                        if (value_.size() > 4) {
                            switch (value_[4]) {
                                case 'N':
                                case ')':
                                    return std::string_view(value_.data() + 2, 2);
                                default:
                                    return std::string_view(value_.data() + 2, 3);
                            }
                        }
                }
            }
            return value_;
        }

        bool contains(std::string_view sub) const { return value_.find(sub) != std::string::npos; }

        char type() const
        {
            if (value_.empty())
                return '?';
            else
                return value_[0];
        }

      private:
        std::string value_;

        // friend inline std::ostream& operator<<(std::ostream& out, const type_subtype_t& ts) { return out << ts.value_; }
    };

} // namespace acmacs::virus::inline v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::virus::type_subtype_t> : public fmt::formatter<acmacs::fmt_default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::virus::type_subtype_t& ts, FormatContext& ctx) { return format_to(ctx.out(), "{}", static_cast<std::string_view>(ts)); }
};


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
