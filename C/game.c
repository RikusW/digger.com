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
 * CS:0x5FA1 (file 0x5EA1) - score_add, probable reconstruction.
 *
 * Structure observed in the disassembly:
 *   - choose the player's fields (0x40C6/0x40C8 for one path,
 *     0x40CA/0x40CC for the other);
 *   - add the 16-bit argument as a 32-bit value;
 *   - if the result exceeds 999999 (0x000F423F), reset it to 0;
 *   - compare against the next-bonus threshold (0x40CE/0x40D0 or
 *     0x40D2/0x40D4), and on crossing add 0x4E20 (20000) to that threshold;
 *   - update the on-screen number;
 *   - call the penalty function three times.
 *
 * This matches the port's scores.c:addscore(), including the three
 * incpenalty() calls and the 999999 wrap.  The exact field names for the
 * 0x40C2/0x40C4 pair are not yet proved.
 */

/*
 * Fixed-score wrappers (CS:0x68A2..0x68DA):
 *
 *   0x68A2 -> score_add(250)   scorekill / scoreoctave
 *   0x68B0 -> score_add(25)    scoreemerald
 *   0x68BE -> score_add(250)   scoreoctave / scorekill
 *   0x68CC -> score_add(500)   scoregold
 *   0x68DA -> score_add(1000)  scorebonus
 *
 * The port's scores.c has exactly these five values.  Its extra
 * scorekill2() (125+125) is a simultaneous-mode addition.
 */

/*
 * CS:0x7933 (file 0x7833) - soundddie is reconstructed in sound.c.
 */
