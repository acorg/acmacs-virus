#pragma once

#include "acmacs-base/log.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

namespace acmacs::log::inline v1
{
    const log_key_t name_parsing{"name"};
    const log_key_t passage_parsing{"passage"};
}

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
