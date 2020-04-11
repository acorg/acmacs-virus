#include <numeric>

#include "acmacs-base/debug.hh"
#include "acmacs-base/counter.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-digits.hh"
#include "acmacs-base/string.hh"
#include "acmacs-virus/parsing-message.hh"
#include "acmacs-virus/host.hh"

// ----------------------------------------------------------------------

// ----------------------------------------------------------------------

void acmacs::virus::name::merge(parsing_messages_by_key_t& target, parsing_messages_t&& new_messages, std::string_view source)
{
    for (auto& msg : new_messages)
        target[msg.key].emplace_back(std::move(msg.value), source);

} // acmacs::virus::name::merge

// ----------------------------------------------------------------------

void acmacs::virus::name::report(parsing_messages_by_key_t& messages)
{
    AD_INFO("Total messages: {}", std::accumulate(std::begin(messages), std::end(messages), 0ul, [](auto sum, const auto& msgs) { return sum + msgs.second.size(); }));
    for (auto& [key, msgs] : messages) {
        fmt::print(stderr, "\n");
        AD_INFO("{} ({})", key, msgs.size());
        std::sort(std::begin(msgs), std::end(msgs), [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
        for (const auto& [msg, source] : msgs)
           fmt::print("    \"{}\"     \"{}\"\n", msg, source);
    }

    if (const auto found = messages.find(parsing_message_t::location_field_not_found); found != std::end(messages)) {
        // AD_INFO("LFNF ({})", found->second.size());

        acmacs::Counter<std::string> locations_to_check;
        const auto add = [&locations_to_check](std::string_view part) {
            const auto prefix_case = acmacs::string::non_digit_prefix(part);
            auto prefix = ::string::upper(prefix_case);
            while (prefix.size() > 4 && (prefix.back() == '_' || prefix.back() == '-' || prefix.back() == ' '))
                prefix.erase(prefix.size() - 1);
            if (prefix.size() > 3 && !is_host(prefix))
                locations_to_check.count(prefix);
        };

        for (const auto& mm : found->second) {
            // fmt::print("{}\n", mm.second);
            const auto parts = acmacs::string::split(mm.second, "/", acmacs::string::Split::StripKeepEmpty);
            switch (parts.size()) {
              case 3:
                  add(parts[1]);
                  break;
              case 4:
                  add(parts[1]);
                  add(parts[2]);
                  break;
              case 5:
              case 6:
                  add(parts[2]);
                  add(parts[3]);
                  break;
            }
        }
        if (!locations_to_check.empty()) {
            fmt::print(stderr, "\n");
            AD_INFO("Locations to check ({})", locations_to_check.size());
            fmt::print("{}\n", locations_to_check.report_sorted_max_first("{quoted_first:30s} {second:4d}\n"));
            // for (const auto& loc : locations_to_check)
            //     fmt::print("{}\n", loc);

            // fmt::print(stderr, "\nlocdb");
            // for (const auto& loc : locations_to_check)
            //     fmt::print(stderr, " \"{}\"", loc);
            // fmt::print(stderr, "\n");
        }
    }

} // acmacs::virus::name::report

// ----------------------------------------------------------------------

// void acmacs::virus::name::merge(parsing_messages_t& target, parsing_messages_t&& new_messages)
// {
//         const auto pos_target = static_cast<ssize_t>(target.size());
//         target.resize(target.size() + new_messages.size());
//         std::move(std::begin(new_messages), std::end(new_messages), std::next(std::begin(target), pos_target));

// } // acmacs::virus::name::merge

// // ----------------------------------------------------------------------

// void acmacs::virus::name::report_by_key(parsing_messages_t& messages)
// {
//     AD_INFO("Total messages: {}", messages.size());
//     std::sort(std::begin(messages), std::end(messages), [](const auto& e1, const auto& e2) { return std::string_view{e1.key} < std::string_view{e2.key}; });
//     const acmacs::Counter<std::string_view> message_keys(std::begin(messages), std::end(messages), [](const auto& en) -> std::string_view { return en.key; });
//     std::string_view last_key;
//     for (const auto& message : messages) {
//         if (std::string_view{message.key} != last_key) {
//             last_key = std::string_view{message.key};
//             AD_INFO("{} ({})", last_key, message_keys[last_key]);
//         }
//         fmt::print("    \"{}\" {}\n", message.value, message.suppliment);
//     }
//     if (message_keys[parsing_message_t::location_not_found]) {
//         acmacs::Counter<std::string_view> unrecognized_locations;
//         for (const auto& message : messages) {
//             if (std::string_view{message.key} == parsing_message_t::location_not_found)
//                 unrecognized_locations.count(message.value);
//         }
//         AD_INFO("unrecognized locations ({})\n{}", unrecognized_locations.size(), unrecognized_locations.report_sorted_max_first("    {first:40s} {second:4d}\n"));
//     }

// } // acmacs::virus::name::report_by_key

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
