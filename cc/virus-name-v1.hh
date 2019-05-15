#pragma once

#include <iostream>

#include "acmacs-base/string.hh"

// ----------------------------------------------------------------------

namespace virus_name
{

    struct Unrecognized : public std::runtime_error { using std::runtime_error::runtime_error; };

// ----------------------------------------------------------------------

    struct Name
    {
        Name(std::string source);
        std::string name() const { return string::join("/", {virus_type, host, location, isolation, year}); }
        std::string name_extra() const { return string::join(" ", {name(), extra}); }
        std::string full() const { return string::join(" ", {name(), reassortant, extra}); }

        enum class report_extra { no, yes };
        void fix_extra(report_extra rep = report_extra::yes);

        std::string virus_type, host, location, isolation, year, reassortant, extra;
    };

    inline std::string normalize(std::string name)
    {
        try {
            return Name(name).full();
        }
        catch (Unrecognized&) {
            std::cerr << "WARNING: Virus name unrecognized: \"" << name << '"' << std::endl;
            return name;
        }
    }

// ----------------------------------------------------------------------

      // Extracts virus name without passage, reassortant, extra, etc.
    std::string_view name(std::string_view aFullName); // noexcept, used by: AceAntigens::find_by_full_name, Acd1Antigens::find_by_full_name

    using location_func_t = std::string (*)(std::string);
    enum class prioritize_cdc_name { no, yes };

    std::string location_for_cdc_name(std::string name); // throws Unrecognized, used by: LocDb::find_for_virus_name

    // returned cdc abbreviation starts with #
    std::string location(std::string name, prioritize_cdc_name check_cdc_first = prioritize_cdc_name::no); // throws Unrecognized

    std::string_view virus_type(const std::string& name); // pass by reference! because we return string_view to it, throws Unrecognized, used by: geographic-map.cc location_of_antigen

    // std::string year(std::string name); // throws Unrecognized, unused

    void split(std::string name, std::string& virus_type, std::string& host, std::string& location, std::string& isolation, std::string& year, std::string& passage); // used by: hidb Antigen::Antigen, Serum::Serum

    // split for names looking like international but with unrecognized "garbage" (extra) at the end
    void split_with_extra(std::string name, std::string& virus_type, std::string& host, std::string& location, std::string& isolation, std::string& year, std::string& passage, std::string& extra);

} // namespace virus_name

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
