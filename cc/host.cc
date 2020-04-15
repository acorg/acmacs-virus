#include <algorithm>
#include <array>

#include "acmacs-base/string.hh"
#include "acmacs-virus/host.hh"

// ----------------------------------------------------------------------

bool acmacs::virus::name::is_host(std::string_view source)
{
    using namespace std::string_view_literals;

    static std::array hosts{
        "TURKEY"sv,
        "DUCK"sv,
        "MUSCOVY DUCK"sv,
        "MALLARD"sv,
        "CHICKEN"sv,
        "GOOSE"sv,
        "PEACOCK"sv,
        "CAT"sv,
        "DOMESTIC"sv,
        "EQUINE"sv,
        "SWINE"sv,
        "UNKNOWN"sv,
        "SWAN"sv,
        "TIGER"sv,
        "WILLET"sv,
        "QUAIL"sv,
        "PELICAN"sv,
        "EGRET"sv,
        "PARTRIDGE"sv,
        "CURLEW"sv,
        "PIGEON"sv,
        "CANINE"sv,
        "TEAL"sv,
        "GULL"sv,
        "AVES"sv, // Aves is the class of birds (nominative plural of avis "bird" in Latin), and town in Portugal
        "LITTLE CUCKOO-DOVE"sv,
        "GREAT BLACK-HEADED GULL"sv,
        "GREY HERON"sv,
        "FOWL"sv,
        "DOMESTIC GOOSE"sv,
        "COMMON EIDER"sv,
        "AMERICAN GREEN-WINGED TEAL"sv,
        "ANAS PLATYRHYNCHOS"sv, // = mallard
        "AYTHYA FULIGULA"sv,    // = tufted duck
        "COCKATOO"sv,           // parrot
        "WILD DUCK"sv,
        "TUFTED DUCK"sv,
        "WHOOPER SWAN"sv,
        "WILD BIRD"sv,
        "WILD BIRD FECES"sv,
        "VILLAGE CHICKEN"sv,
        "AMERICAN BLACK DUCK"sv,
        "BLACK DUCK"sv,
        "RUDDY SHELDUCK"sv,
        "BLACK DUCK"sv,
        "DOMESTIC DUCK"sv,
        "RUDDY_SHELDUCK"sv,
        "SPOT-BILLED DUCK"sv,
        "AMERICAN WIGEON"sv,
        "BLACK-HEADED GULL"sv,
        "BLACK-TAILED GULL"sv,
        "HERRING GULL"sv,
        "LAUGHING GULL"sv,
        "RING-BILLED GULL"sv,
        "SLENDER-BILLED GULL"sv,
        "YELLOW-LEGGED GULL"sv,
        "JAPANESE QUAIL"sv,
        "JAPANESE WHITE-EYE"sv,
        "ORIENTAL WHITE STORK"sv,
        "BLUE-WINGED TEAL"sv,
        "RUDDY TURNSTONE"sv,
        "ENVIRONMENT"sv,
        "HERRING GULL"sv,
        "RUDDY TURNSTONE"sv,
        "BLUE WINGED TEAL"sv,
        "GREEN-WINGED TEAL"sv,
        "BRANT GOOSE"sv,
    };

    return std::find(std::begin(hosts), std::end(hosts), ::string::upper(source)) != std::end(hosts);

} // acmacs::virus::name::is_host

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
