#include "acmacs-base/debug.hh"
#include "acmacs-base/counter.hh"
#include "acmacs-virus/parsing-message.hh"

// ----------------------------------------------------------------------

void acmacs::virus::name::merge(parsing_messages_t& target, parsing_messages_t&& new_messages)
{
        const auto pos_target = static_cast<ssize_t>(target.size());
        target.resize(target.size() + new_messages.size());
        std::move(std::begin(new_messages), std::end(new_messages), std::next(std::begin(target), pos_target));

} // acmacs::virus::name::merge

// ----------------------------------------------------------------------

void acmacs::virus::name::report_by_key(parsing_messages_t& messages)
{
    AD_INFO("Total messages: {}", messages.size());
    std::sort(std::begin(messages), std::end(messages), [](const auto& e1, const auto& e2) { return std::string_view{e1.key} < std::string_view{e2.key}; });
    const acmacs::Counter<std::string_view> message_keys(std::begin(messages), std::end(messages), [](const auto& en) -> std::string_view { return en.key; });
    std::string_view last_key;
    for (const auto& message : messages) {
        if (std::string_view{message.key} != last_key) {
            last_key = std::string_view{message.key};
            AD_INFO("{} ({})", last_key, message_keys[last_key]);
        }
        fmt::print("    \"{}\" {}\n", message.value, message.suppliment);
    }
    if (message_keys[parsing_message_t::location_not_found]) {
        acmacs::Counter<std::string_view> unrecognized_locations;
        for (const auto& message : messages) {
            if (std::string_view{message.key} == parsing_message_t::location_not_found)
                unrecognized_locations.count(message.value);
        }
        AD_INFO("unrecognized locations {}", unrecognized_locations.report_sorted_max_first("    {first:30s} {second}\n"));
    }

} // acmacs::virus::name::report_by_key

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
