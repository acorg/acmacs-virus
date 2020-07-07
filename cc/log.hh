#pragma once

#include "acmacs-base/debug.hh"

// ----------------------------------------------------------------------

namespace acmacs::log
{
    enum {
        name_parsing = 5,
    };

    inline void register_enabler_acmacs_virus()
    {
        using namespace std::string_view_literals;
        register_enabler_acmacs_base();
        register_enabler("name"sv, name_parsing);
    }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
