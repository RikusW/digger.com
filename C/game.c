/*
 * Game-state fragments reconstructed from digger.com.
 *
 * Only the score/bonus initialisation is far enough along to record here.
 * The rest of the game logic (digger, monster, bags, drawing, input) is still
 * to be mapped; see TODO.md.
 */

#include "symbols.h"

/*
 * CS:0x5EEA (file 0x5DEA) - probable score/lives initialisation.
 *
 * Observed stores (all 32-bit fields are little-endian pairs):
 *
 *   0x40C2/0x40C4 = 0
 *   0x40C6/0x40C8 = 0
 *   0x40CA/0x40CC = 0
 *   0x40CE/0x40D0 = 20000 (0x4E20)
 *   0x40D2/0x40D4 = 20000 (0x4E20)
 *
 * The port uses scdat[2] = { score, nextbs } plus bonusscore = 20000
 * (scores.c).  The two 20000 fields are almost certainly the two players'
 * next-bonus thresholds; the exact field order is not yet proved.
 *
 * Not written as a C function because the field layout is still uncertain.
 */

/*
 * CS:0x5FA1 (file 0x5EA1) - probable addscore/addlife.
 *
 * Adds to a 32-bit counter at 0x40C6/0x40C8, compares against 0x423F, performs
 * score display work, and at CS:0x6034 adds 0x4E20 (20000) to the 32-bit
 * counter at 0x40CE/0x40D0.  This corresponds to the port's extra-life bonus
 * logic; function not yet decompiled.
 */

/*
 * CS:0x7933 (file 0x7833) - soundddie is reconstructed in sound.c.
 */
