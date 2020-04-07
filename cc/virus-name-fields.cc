#include <array>
#include <cctype>

#include "acmacs-base/string-split.hh"
#include "acmacs-virus/virus-name.hh"
#include "acmacs-virus/passage.hh"

// ----------------------------------------------------------------------

void acmacs::virus::v2::set_type_subtype(name_t& name, const type_subtype_t& type_subtype)
{
    const std::string_view ts = type_subtype;
    std::string& nam = name.get();
    if (ts.size() > 1 && ts[0] == 'A' && nam.size() > 2 && nam[0] == 'A' && nam[1] == '/')
        nam.replace(0, 1, ts);

} // acmacs::virus::v2::set_type_subtype

// ----------------------------------------------------------------------

std::optional<size_t> acmacs::virus::v2::year(const name_t& name)
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

std::string_view acmacs::virus::v2::host(const name_t& name)
{
    if (const auto fields = acmacs::string::split(*name, "/"); fields.size() == 5)
        return fields[1];
    else
        return {};

} // acmacs::virus::v2::host

// ----------------------------------------------------------------------

std::string_view acmacs::virus::v2::location(const name_t& name)
{
    const auto fields = acmacs::string::split(*name, "/");
    return fields[fields.size() - 3];

} // acmacs::virus::v2::location

// ----------------------------------------------------------------------

std::string_view acmacs::virus::v2::isolation(const name_t& name)
{
    const auto fields = acmacs::string::split(*name, "/");
    return fields[fields.size() - 2];

} // acmacs::virus::v2::isolation

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
