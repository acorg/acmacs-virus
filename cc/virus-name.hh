#pragma once

#include <vector>
#include <optional>

#include "acmacs-base/log.hh"
#include "acmacs-base/uppercase.hh"
#include "acmacs-virus/virus.hh"
#include "acmacs-virus/type-subtype.hh"
#include "acmacs-virus/passage.hh"
#include "acmacs-virus/reassortant.hh"

// #include "acmacs-virus/virus-name-v1.hh"

// ----------------------------------------------------------------------

namespace acmacs::virus::inline v2
{
    // normalized or user approved virus name
    using name_t = acmacs::uppercased<struct virus_name_tag>;
    using host_t = acmacs::uppercased<struct host_t_tag>;
    using lineage_t = acmacs::uppercased<struct lineage_tag>;
    using mutation_t = acmacs::uppercased<struct mutation_tag>;
    using mutations_t = std::vector<mutation_t>;

    // ----------------------------------------------------------------------

    std::string_view host(const name_t& name) noexcept;
    std::string_view location(const name_t& name) noexcept;
    std::string_view isolation(const name_t& name) noexcept;
    std::optional<size_t> year(const name_t& name) noexcept;

    // ----------------------------------------------------------------------

    void set_type_subtype(name_t& name, const type_subtype_t& type_subtype) noexcept;

} // namespace acmacs::virus::inline v2

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
