/*
 * initlevel / drawscreen / initchars - reconstructed from the original's
 * call order.
 *
 * initlevel  CS:0x89A4 (file 0x88A4)
 *   levdone = 0;               (per-player field at DS:0x4FE4 + 8*curplayer)
 *   makefield();               CS:0x6A48
 *   makeemfield();             CS:0x56E8
 *   initbags();                CS:0x3B31
 *   levnotdrawn = 1;           DS:0x5010
 *
 * drawscreen CS:0x89CD (file 0x88CD)
 *   creatembspr();  CS:0x1826
 *   drawstatics();  CS:0x6B31
 *   drawbags();     CS:0x3C93
 *   drawemeralds(); CS:0x5777
 *   initdigger();   CS:0x48A0
 *   initmonsters(); CS:0x2BC1
 *
 * initchars  CS:0x89E4 (file 0x88E4)
 *   initmbspr();    CS:0x1A03
 *   initdigger();   CS:0x48A0
 *   initmonsters(); CS:0x2BC1
 */

#include "symbols.h"

static u16 levdone;            /* per-player field, DS:0x4FE4 + 8*curplayer */
static u16 levnotdrawn;        /* DS:0x5010 */

extern void makefield(void);
extern void makeemfield(void);
extern void initbags(void);
extern void creatembspr(void);
extern void drawstatics(void);
extern void drawbags(void);
extern void drawemeralds(void);
extern void initdigger(void);
extern void initmonsters(void);
extern void initmbspr(void);

void initlevel(void)
{
    levdone = 0;
    makefield();
    makeemfield();
    initbags();
    levnotdrawn = 1;
}

void drawscreen(void)
{
    creatembspr();
    drawstatics();
    drawbags();
    drawemeralds();
    initdigger();
    initmonsters();
}

void initchars(void)
{
    initmbspr();
    initdigger();
    initmonsters();
}
