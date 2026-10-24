/*
 * dobags - reconstructed from CS:0x3E2D (file 0x03D2D).
 *
 * The original bag records have stride 0x28 and use these DS offsets:
 *   x       0x3E9A    y        0x3E9C
 *   h       0x3E9E    v        0x3EA0
 *   dir     0x3EA6    wobbling 0x3EA8
 *   exist   0x3EAE    gt       0x3EB0
 *   goldtime 0x3FDC
 *
 * The loop runs bag = 1..7 in the original; the port uses bagdat[0..6] but
 * the same fields and order.
 *
 * Sub-functions:
 *   soundbreak    CS:0x7497
 *   drawgold      CS:0x1E9D
 *   incpenalty    CS:0x8A32
 *   removebag     CS:0x4703
 *   getfield      CS:0x7207
 *   updatebag     CS:0x3FED
 */

#include "symbols.h"

typedef struct {
    s16 x, y, h, v, xr, yr, dir, wt, gt, fallh;
    u8 wobbling, unfallen, exist;
} bagrec;

static bagrec bagdat[8];       /* original: stride 0x28, index 1..7 */
static s16 goldtime;           /* DS:0x3FDC, set by initbags */

extern void soundbreak(void);
extern void drawgold(int n, int t, int x, int y);
extern void incpenalty(void);
extern void removebag(int bag);
extern int getfield(int x, int y);
extern void updatebag(int bag);
extern void soundfalloff(void);
extern void soundwobbleoff(void);

void dobags(void)
{
    int bag;
    int soundfalloffflag = 1, soundwobbleoffflag = 1;

    for (bag = 1; bag <= 7; bag++)
        if (bagdat[bag].exist) {
            if (bagdat[bag].gt != 0) {
                if (bagdat[bag].gt == 1) {
                    soundbreak();
                    drawgold(bag, 4, bagdat[bag].x, bagdat[bag].y);
                    incpenalty();
                }
                if (bagdat[bag].gt == 3) {
                    drawgold(bag, 5, bagdat[bag].x, bagdat[bag].y);
                    incpenalty();
                }
                if (bagdat[bag].gt == 5) {
                    drawgold(bag, 6, bagdat[bag].x, bagdat[bag].y);
                    incpenalty();
                }
                bagdat[bag].gt++;
                if (bagdat[bag].gt == goldtime)
                    removebag(bag);
                else
                    if (bagdat[bag].v < 9 && bagdat[bag].gt < goldtime - 10)
                        if (!(getfield(bagdat[bag].h, bagdat[bag].v + 1) & 0x2000))
                            bagdat[bag].gt = goldtime - 10;
            } else
                updatebag(bag);
        }

    for (bag = 1; bag <= 7; bag++) {
        if (bagdat[bag].dir == 6 && bagdat[bag].exist)   /* DIR_DOWN */
            soundfalloffflag = 0;
        if (bagdat[bag].dir != 6 && bagdat[bag].wobbling && bagdat[bag].exist)
            soundwobbleoffflag = 0;
    }
    if (soundfalloffflag)
        soundfalloff();
    if (soundwobbleoffflag)
        soundwobbleoff();
}
