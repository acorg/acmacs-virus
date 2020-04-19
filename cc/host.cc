#include <algorithm>
#include <array>

#include "acmacs-base/string-compare.hh"
#include "acmacs-virus/host.hh"

// ----------------------------------------------------------------------

std::string_view acmacs::virus::name::fix_host(std::string_view source)
{
    using namespace std::string_view_literals;
    using pp = std::pair<std::string_view, std::string_view>; // lookup, replace with

    const static std::array hosts{
        pp{"ANAS PLATYRHYNCHOS"sv, "MALLARD"sv},
        pp{"AYTHYA FULIGULA"sv, "TUFTED DUCK"sv},
        pp{"BLACK BELLIED WHISTLING DUCK"sv, "BLACK-BELLIED WHISTLING DUCK"sv},
        pp{"BLACK HEADED GULL"sv, "BLACK-HEADED GULL"sv},
        pp{"BLACK-BELLIED WHISTLING-DUCK"sv, "BLACK-BELLIED WHISTLING DUCK"sv},
        pp{"BLUE WINGED TEAL"sv, "BLUE-WINGED TEAL"sv},
        pp{"GREAT BLACK HEADED GULL"sv, "GREAT BLACK-HEADED GULL"sv},
        pp{"HUMAN"sv, ""sv},
        pp{"JAPANESE WHITE EYE"sv, "JAPANESE WHITE-EYE"sv},
        pp{"KNOT"sv, "RED KNOT"sv}, // KNOT is in UK-English
        pp{"LARUS ICHTHYAETUS"sv, "GREAT BLACK-HEADED GULL"sv},
        pp{"MALLARD DUCK"sv, "MALLARD"sv},
        pp{"PALLAS GULL"sv, "GREAT BLACK-HEADED GULL"sv},
        pp{"PALLAS'S GULL"sv, "GREAT BLACK-HEADED GULL"sv},
        pp{"PALLASS GULL"sv, "GREAT BLACK-HEADED GULL"sv},
        pp{"PIED MEGPIE"sv, "PIED MAGPIE"sv},
        pp{"PINTAIL DUCK"sv, "PINTAIL"sv},
        pp{"RUDDY_SHELDUCK"sv, "RUDDY SHELDUCK"sv},
        pp{"WHITE FACED WHISTLING DUCK"sv, "WHITE-FACED WHISTLING DUCK"sv},
        pp{"WHITE FRONTED GOOSE"sv, "WHITE-FRONTED GOOSE"sv},
        pp{"WHITE-FACED WHISTLING-DUCK"sv, "WHITE-FACED WHISTLING DUCK"sv},
        pp{"THICK-BILLED_MURRE"sv, "THICK-BILLED MURRE"sv},
        pp{"MELEAGRIS GALLOPAVO"sv, "WILD TURKEY"sv},
        pp{"GRAY TEAL"sv, "GREY TEAL"sv},
        pp{"AFRI.STAR."sv, "AFRICAN STARLING"sv},
        pp{"CHUKKAR"sv, "CHUKAR"sv},
        // pp{sv, sv},
    };

    if (const auto found = std::find_if(std::begin(hosts), std::end(hosts), [source](const auto& en) { return acmacs::string::equals_ignore_case(source, en.first); }); found != std::end(hosts))
        return found->second;
    else
        return source;

} // acmacs::virus::name::fix_host

// ----------------------------------------------------------------------

bool acmacs::virus::name::is_host(std::string_view source)
{
    using namespace std::string_view_literals;

    const static std::array hosts{
        "AFRICAN STARLING"sv
        "AMERICAN BLACK DUCK"sv,
        "AMERICAN GREEN-WINGED TEAL"sv,
        "AMERICAN WIGEON"sv,
        "AVES"sv, // Aves is the class of birds (nominative plural of avis "bird" in Latin), and town in Portugal
        "BLACK DUCK"sv,
        "BLACK-BELLIED WHISTLING DUCK"sv
        "BLACK-HEADED GULL"sv,
        "BLACK-TAILED GULL"sv,
        "BLUE-WINGED TEAL"sv,
        "BRANT GOOSE"sv,
        "CANINE"sv,
        "CAT"sv,
        "CHICKEN"sv,
        "CHUKAR"sv
        "COCKATOO"sv, // parrot
        "COMMON EIDER"sv,
        "CURLEW"sv,
        "DOMESTIC DUCK"sv,
        "DOMESTIC GOOSE"sv,
        "DOMESTIC"sv,
        "DUCK"sv,
        "EGRET"sv,
        "ENVIRONMENT"sv,
        "EQUINE"sv,
        "FOWL"sv,
        "FRANKLINS GULL"sv,
        "GOOSE"sv,
        "GREAT BLACK-HEADED GULL"sv,
        "GREEN-WINGED TEAL"sv,
        "GREY HERON"sv,
        "GREY TEAL"sv,
        "GULL"sv,
        "HERRING GULL"sv,
        "JAPANESE QUAIL"sv,
        "JAPANESE WHITE-EYE"sv,
        "LAUGHING GULL"sv,
        "LITTLE CUCKOO-DOVE"sv,
        "MAGPIE"sv,
        "MALLARD"sv,
        "MIGRATORY DUCK"sv,
        "MUSCOVY DUCK"sv,
        "NORTHERN SHOVELER"sv,
        "ORIENTAL WHITE STORK"sv,
        "PARTRIDGE"sv,
        "PEACOCK"sv,
        "PELICAN"sv,
        "PIED MAGPIE"sv,
        "PIGEON"sv,
        "PINTAIL"sv,
        "QUAIL"sv,
        "RING-BILLED GULL"sv,
        "RUDDY SHELDUCK"sv,
        "RUDDY TURNSTONE"sv,
        "SLENDER-BILLED GULL"sv,
        "SPOT-BILLED DUCK"sv,
        "SWAN"sv,
        "SWINE"sv,
        "TEAL"sv,
        "THICK-BILLED MURRE"sv
        "TIGER"sv,
        "TUFTED DUCK"sv,
        "TURKEY"sv,
        // "UNKNOWN"sv,
        "VILLAGE CHICKEN"sv,
        "WHITE-FACED WHISTLING DUCK"sv,
        "WHITE-FRONTED GOOSE"sv,
        "WHOOPER SWAN"sv,
        "WILD BIRD FECES"sv,
        "WILD BIRD"sv,
        "WILD DUCK"sv,
        "WILD TURKEY"sv
        "WILLET"sv,
        "YELLOW-LEGGED GULL"sv,
    };

    const std::string_view fixed = fix_host(source);
    const auto found =
        std::lower_bound(std::begin(hosts), std::end(hosts), fixed, [](std::string_view from_list, std::string_view look_for) { return acmacs::string::compare_ignore_case(from_list, look_for) < 0; });
    return found != std::end(hosts) && acmacs::string::equals_ignore_case(*found, fixed);

} // acmacs::virus::name::is_host

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
