#pragma once

#include <map>
#include <set>

#include "acmacs-base/fmt.hh"
#include "acmacs-base/messages.hh"

// ----------------------------------------------------------------------

namespace acmacs::messages::inline v1::key
{
    constexpr static inline std::string_view empty_name{"~~empty-name"};
    constexpr static inline std::string_view invalid_subtype{"invalid-subtype"};
    constexpr static inline std::string_view invalid_host{"invalid-host"};
    constexpr static inline std::string_view location_not_found{"location-not-found"};
    constexpr static inline std::string_view location_field_not_found{"+location-field-not-found"};
    constexpr static inline std::string_view isolation_absent{"isolation-absent"};
    constexpr static inline std::string_view invalid_isolation{"invalid-isolation"};
    constexpr static inline std::string_view invalid_year{"invalid-year"};
    constexpr static inline std::string_view unrecognized_passage{"unrecognized-passage"};
    constexpr static inline std::string_view reassortant_without_name{"reassortant-without-name"};
    constexpr static inline std::string_view location_or_host{"++location-or-host"};
} // namespace acmacs::messages::inline v1::key

namespace acmacs::virus::inline v2::name
{
    void report(acmacs::messages::messages_t& messages);
    void collect_not_found_locations(std::set<std::string>& locations, const acmacs::messages::messages_t& messages);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
