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
        "AVES"sv,               // Aves is the class of birds (nominative plural of avis "bird" in Latin), and town in Portugal
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

    };

    return std::find(std::begin(hosts), std::end(hosts), ::string::upper(source)) != std::end(hosts);

} // acmacs::virus::name::is_host

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
