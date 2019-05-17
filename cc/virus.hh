#pragma once

namespace acmacs::virus
{
    inline namespace v2
    {
        struct Error : public std::runtime_error { using std::runtime_error::runtime_error; };

    }
}


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
