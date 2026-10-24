/*
 * Level data and getlevch - reconstructed from digger.com.
 *
 * The original stores eight separate 150-byte arrays at the DS offsets
 * below; this file flattens them into one array.  The bytes are
 * byte-identical to the port's leveldat (main.c).
 */

#include "symbols.h"

#define MHEIGHT 10
#define MWIDTH  15

static const char level_data[8][MHEIGHT][MWIDTH] = {
    { /* level 1, DS:0x4B0E / file 0xD87E */
        "S   B     HHHHS",
        "V  CC  C  V B  ",
        "VB CC  C  V    ",
        "V  CCB CB V CCC",
        "V  CC  C  V CCC",
        "HH CC  C  V CCC",
        " V    B B V    ",
        " HHHH     V    ",
        "C   V     V   C",
        "CC  HHHHHHH  CC",
    },
    { /* level 2, DS:0x4BA4 / file 0xD914 */
        "SHHHHH  B B  HS",
        " CC  V       V ",
        " CC  V CCCCC V ",
        "BCCB V CCCCC V ",
        "CCCC V       V ",
        "CCCC V B  HHHH ",
        " CC  V CC V    ",
        " BB  VCCCCV CC ",
        "C    V CC V CC ",
        "CC   HHHHHH    ",
    },
    { /* level 3, DS:0x4C3A / file 0xD9AA */
        "SHHHHB B BHHHHS",
        "CC  V C C V BB ",
        "C   V C C V CC ",
        " BB V C C VCCCC",
        "CCCCV C C VCCCC",
        "CCCCHHHHHHH CC ",
        " CC  C V C  CC ",
        " CC  C V C     ",
        "C    C V C    C",
        "CC   C H C   CC",
    },
    { /* level 4, DS:0x4CD0 / file 0xDA40 */
        "SHBCCCCBCCCCBHS",
        "CV  CCCCCCC  VC",
        "CHHH CCCCC HHHC",
        "C  V  CCC  V  C",
        "   HHH C HHH   ",
        "  B  V B V  B  ",
        "  C  VCCCV  C  ",
        " CCC HHHHH CCC ",
        "CCCCC CVC CCCCC",
        "CCCCC CHC CCCCC",
    },
    { /* level 5, DS:0x4D66 / file 0xDAD6 */
        "SHHHHHHHHHHHHHS",
        "VBCCCCBVCCCCCCV",
        "VCCCCCCV CCBC V",
        "V CCCC VCCBCCCV",
        "VCCCCCCV CCCC V",
        "V CCCC VBCCCCCV",
        "VCCBCCCV CCCC V",
        "V CCBC VCCCCCCV",
        "VCCCCCCVCCCCCCV",
        "HHHHHHHHHHHHHHH",
    },
    { /* level 6, DS:0x4DFC / file 0xDB6C */
        "SHHHHHHHHHHHHHS",
        "VCBCCV V VCCBCV",
        "VCCC VBVBV CCCV",
        "VCCCHH V HHCCCV",
        "VCC V CVC V CCV",
        "VCCHH CVC HHCCV",
        "VC V CCVCC V CV",
        "VCHHBCCVCCBHHCV",
        "VCVCCCCVCCCCVCV",
        "HHHHHHHHHHHHHHH",
    },
    { /* level 7, DS:0x4E92 / file 0xDC02 */
        "SHCCCCCVCCCCCHS",
        " VCBCBCVCBCBCV ",
        "BVCCCCCVCCCCCVB",
        "CHHCCCCVCCCCHHC",
        "CCV CCCVCCC VCC",
        "CCHHHCCVCCHHHCC",
        "CCCCV CVC VCCCC",
        "CCCCHH V HHCCCC",
        "CCCCCV V VCCCCC",
        "CCCCCHHHHHCCCCC",
    },
    { /* level 8, DS:0x4F28 / file 0xDC98 */
        "HHHHHHHHHHHHHHS",
        "V CCBCCCCCBCC V",
        "HHHCCCCBCCCCHHH",
        "VBV CCCCCCC VBV",
        "VCHHHCCCCCHHHCV",
        "VCCBV CCC VBCCV",
        "VCCCHHHCHHHCCCV",
        "VCCCC V V CCCCV",
        "VCCCCCV VCCCCCV",
        "HHHHHHHHHHHHHHH",
    },
};

/*
 * CS:0x8001 (file 0x7F01).
 * The original switches on level 1..8 and returns
 * level_data[level-1][y*15 + x].  For a level outside 1..8 the
 * jump table falls through without setting a defined return value;
 * this reconstruction returns 0.
 */
char getlevch(int x, int y, int level)
{
    switch (level) {
    case 1: return level_data[0][y][x];
    case 2: return level_data[1][y][x];
    case 3: return level_data[2][y][x];
    case 4: return level_data[3][y][x];
    case 5: return level_data[4][y][x];
    case 6: return level_data[5][y][x];
    case 7: return level_data[6][y][x];
    case 8: return level_data[7][y][x];
    }
    return 0;
}

/*
 * Port difference: main.c:getlevch() adds
 *
 *   if ((l==3 || l==4) && !levfflag && diggers==2
 *       && y==9 && (x==6 || x==8))
 *       return 'H';
 *
 * before the table lookup.  That simultaneous-mode special case is
 * not present in the original function.
 */
