#include <array>
#include <cctype>

#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-virus/virus-name.hh"
#include "acmacs-virus/passage.hh"

// ----------------------------------------------------------------------

void acmacs::virus::v2::set_type_subtype(name_t& name, const type_subtype_t& type_subtype) noexcept
{
    const std::string_view ts = type_subtype;
    std::string& nam = name.get();
    if (ts.size() > 1 && ts[0] == 'A' && nam.size() > 2 && nam[0] == 'A' && nam[1] == '/')
        nam.replace(0, 1, ts);

} // acmacs::virus::v2::set_type_subtype

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::virus::v2::year(const name_t& name) noexcept
{
    if (name->size() > 4) {
        std::array<char, 5> data{0, 0, 0, 0, 0}; // avoid acceing beyond the name by strtoul
        std::copy_n(name->data() + name->size() - 4, 4, std::begin(data));
        char* end;
        const auto yr = std::strtoul(data.data(), &end, 10);
        if (end == (data.data() + 4) && yr > 0)
            return yr;
    }
    return std::nullopt;

} // acmacs::virus::v2::year

// ----------------------------------------------------------------------

std::string_view acmacs::virus::v2::host(const name_t& name, std::string_view if_not_found) noexcept
{
    if (const auto fields = acmacs::string::split(*name, "/"); fields.size() == 5)
        return fields[1];
    else
        return if_not_found;

} // acmacs::virus::v2::host

// ----------------------------------------------------------------------

std::string_view acmacs::virus::v2::location(const name_t& name, std::string_view if_not_found) noexcept
{
    if (const auto fields = acmacs::string::split(*name, "/"); fields.size() >= 3)
        return fields[fields.size() - 3];
    else
        return if_not_found;

} // acmacs::virus::v2::location

// ----------------------------------------------------------------------

std::string_view acmacs::virus::v2::isolation(const name_t& name, std::string_view if_not_found) noexcept
{
    if (const auto fields = acmacs::string::split(*name, "/"); fields.size() >= 3)
        return fields[fields.size() - 2];
    else
        return if_not_found;

} // acmacs::virus::v2::isolation

// ----------------------------------------------------------------------

std::string_view acmacs::virus::v2::without_subtype(const name_t& name) noexcept
{
    using namespace std::string_view_literals;
    if (const auto fields = acmacs::string::split(*name, "/"); fields.size() >= 3 && fields[0].size() >= 1) {
        switch (fields[0][0]) {
            case 'A':
                if (fields[0].size() == 1 || fields[0][1] == '(')
                    return std::string_view{*name}.substr(fields[0].size() + 1);
                break;
            case 'B':
                if (fields[0].size() == 1 || (fields[0].size() == 2 && (fields[0][1] == 'V' || fields[0][1] == 'Y')) || fields[0].substr(1, 3) == "VIC"sv || fields[0].substr(1, 3) == "YAM"sv)
                    return std::string_view{*name}.substr(fields[0].size() + 1);
                break;
        }
    }
    return *name;

} // acmacs::virus::v2::without_subtype


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
