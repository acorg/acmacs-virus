#include <numeric>

#include "acmacs-base/debug.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-digits.hh"
#include "acmacs-base/string.hh"
#include "acmacs-virus/parsing-message.hh"
#include "acmacs-virus/host.hh"

// static void count_locations_to_check(acmacs::messages::iter_t first, acmacs::messages::iter_t last, acmacs::Counter<std::string>& locations_to_check);

// ----------------------------------------------------------------------

void acmacs::virus::v2::name::report(acmacs::messages::messages_t& messages)
{
    const auto index = acmacs::messages::make_index(messages);
    AD_INFO("Total messages: {}  keys: {}", messages.size(), index.size());
    for (const auto& [first, last] : index) {
        if (first != last) {
            if (first->key == acmacs::messages::key::location_not_found)
                acmacs::messages::report_by_count(first, last);
            else
                acmacs::messages::report(first, last);
        }
    }

    // acmacs::Counter<std::string> locations_to_check;
    // if (const auto& [first, last] = acmacs::messages::find(acmacs::messages::key::location_field_not_found, index); first != last)
    //     count_locations_to_check(first, last, locations_to_check);
    // if (const auto& [first, last] = acmacs::messages::find(acmacs::messages::key::location_not_found, index); first != last)
    //     std::for_each(first, last, [&locations_to_check](const auto& en) { locations_to_check.count(en.value); });
    // if (!locations_to_check.empty()) {
    //     AD_INFO("Locations to check ({}):", locations_to_check.size());
    //     fmt::print(stderr, "{}\n", locations_to_check.report_sorted_max_first("  {quoted_first:30s} {second:4d}\n"));
    // }
    // else
    //     AD_INFO("No locations to check");

} // acmacs::virus::v2::name::report

// ----------------------------------------------------------------------

// void count_locations_to_check(acmacs::messages::iter_t first, acmacs::messages::iter_t last, acmacs::Counter<std::string>& locations_to_check)
// {
//     const auto add = [&locations_to_check](std::string_view part) {
//         const auto prefix_case = acmacs::string::non_digit_prefix(part);
//         auto prefix = ::string::upper(prefix_case);
//         while (prefix.size() > 4 && (prefix.back() == '_' || prefix.back() == '-' || prefix.back() == ' '))
//             prefix.erase(prefix.size() - 1);
//         if (prefix.size() > 3 && !acmacs::virus::name::is_host(prefix))
//             locations_to_check.count(prefix);
//     };

//     for (; first != last; ++first) {
//         // AD_DEBUG("count_locations_to_check \"{}\"", first->value);
//         const auto parts = acmacs::string::split(first->value, "/", acmacs::string::Split::StripKeepEmpty);
//         switch (parts.size()) {
//             case 1:
//             case 2:
//                 break;
//             case 3:
//                 add(parts[1]);
//                 break;
//             case 4:
//                 add(parts[1]);
//                 add(parts[2]);
//                 break;
//             case 5:
//             case 6:
//             default:
//                 add(parts[2]);
//                 add(parts[3]);
//                 break;
//         }
//     }

// } // count_locations_to_check

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
