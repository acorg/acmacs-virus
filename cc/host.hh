#pragma once

#include <string_view>

// ----------------------------------------------------------------------

namespace acmacs::virus::inline v2::name
{
    bool is_host(std::string_view source);
    std::string_view fix_host(std::string_view source);

} // namespace acmacs::virus::inline v2

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
