#pragma once

// ----------------------------------------------------------------------

#define FLU_A_HN_NUMBER_GOOD "\\d{1,2}"
#define FLU_A_HN_NUMBER_BAD "[\\-\\?x]"
#define FLU_A_H_GOOD "H" FLU_A_HN_NUMBER_GOOD
#define FLU_A_H_BAD "H" FLU_A_HN_NUMBER_BAD
#define FLU_A_N_GOOD "N" FLU_A_HN_NUMBER_GOOD "v?"
#define FLU_A_N_BAD "N" FLU_A_HN_NUMBER_BAD

// 3 good groups, H3N2 H3 N2 H?N2
#define FLU_A_SUBTYPE_1 "(" FLU_A_H_GOOD ")/?(" FLU_A_N_GOOD ")?|(?:" FLU_A_H_BAD ")/?(" FLU_A_N_GOOD ")"

// 1 good group, H3N? H?N? H?
#define FLU_A_SUBTYPE_2 "(?:(" FLU_A_H_GOOD ")/?(?:" FLU_A_N_BAD ")?|" FLU_A_H_BAD "/?(?:" FLU_A_N_BAD ")?)"

// 4 good groups, use "$1$2$3$4" to extract
#define FLU_A_SUBTYPE "(?:" FLU_A_SUBTYPE_1 "|" FLU_A_SUBTYPE_2 ")"

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
