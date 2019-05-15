#include <iostream>
#include <cctype>
#include <array>
#include <tuple>
#include <regex>

#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name-v1.hh"

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"
using re_entry_t = std::tuple<std::regex, const char*>;
static const std::array sReReassortant = {
    re_entry_t{"NYMC[\\- ]*([0-9]+[A-Z]*)",     "NYMC-$1"},
    re_entry_t{"NYMC *BX[\\- ]*([0-9]+[A-Z]*)", "NYMC-$1"},
    re_entry_t{"BVR[\\- ]*0*([0-9]+[A-Z]*)",    "CBER-$1"}, // B/PHUKET/3073/2013 BVR-1B -> CBER-1B
    re_entry_t{"CBER[\\- ]*0*([0-9]+[A-Z]*)",   "CBER-$1"}, // A(H3N2)/KANSAS/14/2017 CBER-22B
    // re_entry_t{"X[\\- ]*([0-9]+[A-Z]*)",        "NYMC-$1"},
};
#include "acmacs-base/diagnostics-pop.hh"

// ----------------------------------------------------------------------

namespace virus_name
{
    static void split_and_strip(std::string name, std::string& virus_type, std::string& host, std::string& location, std::string& isolation, std::string& year, std::string& extra);

    // // Extracts virus name without passage, reassortant, extra,
    // // etc. and calculates match threshold (to use with
    // // acmacs_chart::Antigens::find_by_name_matching), match threshold is a square
    // // of virus name length.
    // static size_t match_threshold(std::string name);

    namespace _internal
    {
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

        const std::regex cdc_name{"^([A-Z][A-Z][A-Z]?)[ \\-]"}; // cdc names had location abbreviation separated with '-' earlier

        // const std::string flu_a_subtype{"H[1-9][0-9]?(?:N[1-9][0-9]?)?"};
        // const std::string flu_type_subtype("(?:B|A\\s*[\\(/]?" + flu_a_subtype + "\\)?)");
        // const std::string flu_host{"[A-Z\\s]+"};
        // const std::string flu_location{"[A-Z\\-_\\s0-9]+"};
        // const std::string flu_isolation{"[^/]+"};
        // const std::string flu_year{""};

        constexpr const char* re_international_name =
                "^(?:([AB][^/]*)/)?" // type subtype
                "(?:([^/]+)/)?"      // host
                "([^/]{2,})/"        // location/
                "0*([^/]+)/"         // isolation /
                "(19|20)?(\\d\\d)"   // year
                "(?:\\s*\\)\\s*)?"   // closing ) surrounded with spaces
                ;

        constexpr const char* re_reassortant_passage = "(?:(?:\\s+|__)(.+))?";

          // [1] - type+subtype, [2] - host, [3] - location, [4] - isolation-number (omitting leading zeros), [5] - century, [6] - year (2 last digit), [7] - reassortant and passage
          // const std::regex international{"^([AB][^/]*)/(?:([^/]+)/)?([^/]+)/0*([^/]+)/(19|20)?(\\d\\d)(?:(?:\\s+|__)(.+))?$"};
        const std::regex international{std::string(re_international_name) + re_reassortant_passage + '$'};
        const std::regex international_name{re_international_name};
        const std::regex international_name_with_extra{std::string(re_international_name) + "(?:[\\s_\\-]*(.+))?"};

        const std::regex passage_after_name{" (MDCK|SIAT|MK|E|X)[X\\?\\d]"}; // to extract cdc name only! NOT to extract passage!

#pragma GCC diagnostic pop

        inline std::string make_year(const std::smatch& m)
        {
            std::string year;
            if (m[5].length())
                year = m[5].str() + m[6].str();
            else if (m[6].str()[0] > '2')
                year = "19" + m[6].str();
            else
                year = "20" + m[6].str();
            return year;
        }

        // constexpr const size_t international_name_suffix_size = 9; // len(lo/isolation-number/year) >= 9

        inline std::string location_human(std::string name, size_t prefix_size)
        {
            const auto end = name.find('/', prefix_size);
            return name.substr(prefix_size, end - prefix_size);
        }
    }

    // ----------------------------------------------------------------------

    std::string_view name(std::string_view aFullName)
    {
        std::cmatch m;
        if (std::regex_search(aFullName.begin(), aFullName.end(), m, _internal::international_name)) {
            return aFullName.substr(0, static_cast<size_t>(m.length()));
        }
        else if (std::regex_search(aFullName.begin(), aFullName.end(), m, _internal::passage_after_name)) { // works for cdc name without extra and without reassortant (cdc names usually do not have reassortant)
            return aFullName.substr(0, static_cast<size_t>(m.position()));
        }
        else {
            return aFullName;   // failed to split, perhaps cdc name without passage
        }
    }

    // ----------------------------------------------------------------------

    std::string location_for_cdc_name(std::string name)
    {
        std::smatch m;
        if (std::regex_search(name, m, _internal::cdc_name))
            return "#" + m[1].str();
        throw Unrecognized{"No cdc abbreviation in " + name};
    }

    // ----------------------------------------------------------------------

    std::string location(std::string name, prioritize_cdc_name check_cdc_first)
    {
        try {
            if (check_cdc_first == prioritize_cdc_name::yes)
                return location_for_cdc_name(name);
        }
        catch (Unrecognized&) {
        }

        std::smatch m;
        if (std::regex_search(name, m, _internal::international_name)) // international name with possible "garbage" after year, e.g. A/TOKYO/UT-IMS2-1/2014_HY-PR8-HA-N121K
            return m[3].str();

        try {
            if (check_cdc_first == prioritize_cdc_name::no)
                return location_for_cdc_name(name);
        }
        catch (Unrecognized&) {
        }

        throw Unrecognized{"No location in " + name};
    }

    // // Faster version of location() for A(H3N2)/ and A(H1N1)/ names without host field
    // inline std::string location_human_a(std::string name)
    // {
    //     constexpr const size_t prefix_size = 8;
    //     return (name.size() > (prefix_size + _internal::international_name_suffix_size) && name[prefix_size - 1] == '/') ? _internal::location_human(name, prefix_size) : location(name);
    // }

    // // Faster version of location() for B/ names without host field
    // inline std::string location_human_b(std::string name)
    // {
    //     constexpr const size_t prefix_size = 2;
    //     return (name.size() > (prefix_size + _internal::international_name_suffix_size) && name[prefix_size - 1] == '/') ? _internal::location_human(name, prefix_size) : location(name);
    // }

    // ----------------------------------------------------------------------

    // std::string year(std::string name)
    // {
    //     std::smatch m;
    //     if (std::regex_search(name, m, _internal::international_name))
    //         return _internal::make_year(m);
    //     throw Unrecognized("No year in " + name);

    // } // year

    // ----------------------------------------------------------------------

    // size_t match_threshold(std::string name)
    // {
    //     size_t result = 0;
    //     std::smatch m;
    //     if (std::regex_search(name, m, _internal::international_name)) {
    //         // find end of year (m[6])
    //         const auto end_of_year_offset = static_cast<size_t>(m[6].second - name.begin());
    //         result = end_of_year_offset * end_of_year_offset;
    //         // std::cerr << "INFO: match_threshold: end_of_year_offset:" << end_of_year_offset << " name:" << name << std::endl;
    //     }
    //     return result;
    // }

    // ----------------------------------------------------------------------

    std::string_view virus_type(const std::string& name) // pass by reference! because we return string_view to it
    {
        std::smatch m;
        if (std::regex_search(name, m, _internal::international_name))
            return {name.data() + m.position(1), static_cast<size_t>(m.length(1))};
        throw Unrecognized("No virus_type in " + name);

    } // AntigenSerum::virus_type

    // ----------------------------------------------------------------------

    void split(std::string name, std::string& virus_type, std::string& host, std::string& location, std::string& isolation, std::string& year, std::string& passage)
    {
        std::smatch m;
        if (std::regex_match(name, m, _internal::international)) {
            virus_type = m[1].str();
            host = m[2].str();
            location = m[3].str();
            isolation = m[4].str();
            year = _internal::make_year(m);
            passage = m[7].str();
        }
        else
            throw Unrecognized("Cannot split " + name);
    }

    void split_and_strip(std::string name, std::string& virus_type, std::string& host, std::string& location, std::string& isolation, std::string& year, std::string& extra)
    {
        std::smatch m;
        if (std::regex_search(name, m, _internal::international_name_with_extra)) {
            virus_type = string::strip(m[1].str());
            host = string::strip(m[2].str());
            location = string::strip(m[3].str());
            isolation = string::strip(m[4].str());
            year = _internal::make_year(m);
            extra = string::join(" ", {string::strip(m.prefix().str()), string::strip(m[7].str())});
        }
        else
            throw Unrecognized("Cannot split " + name);
    }

    // split for names looking like international but with unrecognized "garbage" (extra) at the end
    void split_with_extra(std::string name, std::string& virus_type, std::string& host, std::string& location, std::string& isolation, std::string& year, std::string& passage, std::string& extra)
    {
        try {
            split(name, virus_type, host, location, isolation, year, passage);
        }
        catch (Unrecognized&) {
            std::smatch m;
            if (std::regex_search(name, m, _internal::international_name_with_extra)) {
                virus_type = m[1].str();
                host = m[2].str();
                location = m[3].str();
                isolation = m[4].str();
                year = _internal::make_year(m);
                extra = string::join(" ", {m.prefix().str(), m[7].str()});
            }
            else
                throw Unrecognized("Cannot split " + name);
        }
    }

    Name::Name(std::string source)
    {
        try {
            split_and_strip(source, virus_type, host, location, isolation, year, extra);
        }
        catch (Unrecognized&) {
            bool re_throw = true;
            if (const auto slashes = std::count(std::begin(source), std::end(source), '/'); slashes == 2) {
                if (const auto num_start = std::find_if(std::begin(source), std::end(source), [](char cc) { return std::isdigit(cc); });
                    num_start != std::begin(source) && num_start != std::end(source) && *(num_start - 1) != '/') {
                    // A/PTO MONTT75856/2015
                    source.insert(num_start, '/');
                    split_and_strip(source, virus_type, host, location, isolation, year, extra);
                    re_throw = false;
                }
            }
            if (re_throw)
                throw;
        }
        if (const auto first_not_zero = isolation.find_first_not_of('0'); first_not_zero != std::string::npos)
            isolation.erase(0, first_not_zero);
    }

    void Name::fix_extra(report_extra rep)
    {
        for (const auto& re_entry : sReReassortant) {
            std::smatch mat;
            if (std::regex_search(extra, mat, std::get<std::regex>(re_entry))) {
                reassortant = mat.format(std::get<const char*>(re_entry));
                extra = string::join(" ", {string::strip(mat.format("$`")), string::strip(mat.format("$'"))});
            }
        }
        if (!extra.empty() && rep == report_extra::yes)
            std::cerr << "WARNING: name contains extra \"" << extra << "\": \"" << full() << "\"\n";
    }

    // std::string normalize(std::string name)
    // {
    //     std::string result;
    //     std::string virus_type, host, location, isolation, year, passage;
    //     try {
    //         split(name, virus_type, host, location, isolation, year, passage);

    //         virus_type = string::strip(virus_type);
    //         host = string::strip(host);
    //         location = string::strip(location);
    //         isolation = string::strip(isolation);
    //         auto first_not_zero = isolation.find_first_not_of('0');
    //         if (first_not_zero != std::string::npos)
    //             isolation.erase(0, first_not_zero);
    //         year = string::strip(year);
    //         passage = string::strip(passage);
    //         if (host.empty())
    //             result = string::join("/", {virus_type, location, isolation, year});
    //         else
    //             result = string::join("/", {virus_type, host, location, isolation, year});
    //         if (!passage.empty()) {
    //             std::cerr << "WARNING: name contains extra: \"" << name << '"' << std::endl;
    //             result.append(1, ' ');
    //             result.append(passage);
    //         }
    //     }
    //     catch (Unrecognized&) {
    //         std::cerr << "WARNING: Virus name unrecognized: \"" << name << '"' << std::endl;
    //         result = name;
    //     }
    //     return result;

    // } // normalize

    void fix_location(Name& name)
    {
        const auto fix1 = [](const auto& src) {
            // std::cerr << "DEBUG: fix1 " << src << '\n';
            const auto& locdb = get_locdb();
            if (detail::find_indexed_by_name_no_fixes(locdb.names(), src).has_value())
                return src;
            if (const auto replacement_it = detail::find_indexed_by_name_no_fixes(locdb.replacements(), src); replacement_it.has_value())
                return replacement_it.value()->second;
            throw LocationNotFound(src);
        };

        if (name.host.empty() && std::isalpha(name.isolation[0])) {
            if (const auto num_start = std::find_if(std::begin(name.isolation), std::end(name.isolation), [](char cc) { return std::isdigit(cc); }); num_start != std::end(name.isolation)) {
                // A/Jilin/Nanguan112/2007 (CDC sequences)
                try {
                    name.location = fix1(string::concat(name.location, ' ', std::string_view(name.isolation.data(), static_cast<size_t>(num_start - name.isolation.begin()))));
                    name.isolation = std::string(num_start, name.isolation.end());
                    return;
                }
                catch (LocationNotFound&) {
                }
            }
        }

        try {
            name.location = fix1(name.location);
        }
        catch (LocationNotFound&) {
            if (!name.host.empty()) {
                try {
                    name.location = fix1(string::concat(name.host, ' ', name.location));
                    name.host.clear();
                }
                catch (LocationNotFound&) {
                    const auto location = fix1(name.host);
                    // A/Algeria/G0281/16/2016
                    name.host.clear();
                    name.isolation = string::concat(name.location, '-', name.isolation);
                    name.location = location;
                }
            }
        }
    }

} // namespace virus_name

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
