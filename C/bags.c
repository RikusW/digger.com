/*
 * Bag module - reconstructed from the original.
 *
 * Bag records have stride 0x28 and are indexed 1..7 (entry 0 is unused):
 *   x        0x3E9A    y        0x3E9C    h        0x3E9E
 *   v        0x3EA0    xr       0x3EA2    yr       0x3EA4
 *   dir      0x3EA6    wobbling 0x3EA8    wt       0x3EAA
 *   unfallen 0x3EAC    exist    0x3EAE    gt       0x3EB0
 *   fallh    0x3EB2
 * The original keeps two saved copies: player 0 at 0x3C1A, player 1 at
 * 0x3D5A; `initbags` writes the active array and one saved copy, and
 * `drawbags` copies the saved copy back.
 *
 * Globals:
 *   pushcount  DS:0x3FDA
 *   goldtime   DS:0x3FDC (150 - levof10()*10)
 *
 * Functions:
 *   initbags      CS:0x3B31 (file 0x3A31)
 *   drawbags      CS:0x3C93 (file 0x3B93)
 *   dobags        CS:0x3E2D (file 0x3D2D)
 *   updatebag     CS:0x3FED (file 0x3EED)
 *   baghitground  CS:0x4296 (file 0x4196)
 *   pushbag       CS:0x433B (file 0x423B)
 *   getnmovingbags CS:0x47F8 (file 0x46F8)
 *   getgold       CS:0x4853 (file 0x4753)
 *   removebag     CS:0x4703 (file 0x4603)
 *   bagy          CS:0x4730 (file 0x4630)
 *   getbagdir     CS:0x4749 (file 0x4649)
 *   bagmask       CS:0x4774 (file 0x4674)
 *   removebags    CS:0x47B5 (file 0x46B5)
 *   pushbags      CS:0x466C (file 0x456C)
 *   pushudbags    CS:0x46B2 (file 0x45B2)
 */

#include <string.h>
#include "symbols.h"

typedef struct {
    s16 x, y, h, v, xr, yr, dir, wobbling, wt, unfallen, exist, gt, fallh;
    s16 pad[7];                /* stride is 0x28 (20 words) */
} bagrec;

static bagrec bagdat[8];       /* index 1..7, as in the original */
static bagrec bagdat1[8];      /* saved copy, DS:0x3C1A */
static bagrec bagdat2[8];      /* saved copy, DS:0x3D5A */

static s16 pushcount;          /* DS:0x3FDA */
static s16 goldtime;           /* DS:0x3FDC */

extern int levof10(void);
extern int levplan(void);
extern char getlevch(int x, int y, int level);
extern int curplayer_get(void);
extern int getfield(int h, int v);
extern int drawgold(int bag, int frame, int x, int y);
extern void incpenalty(void);
extern void erasespr(int spr);
extern void movedrawspr(int spr, int x, int y);
extern void drawtopblob(int x, int y);
extern int reversedir(int dir);
extern void soundbreak(void);
extern void soundfall(void);
extern void soundwobble(void);
extern void soundwobbleoff(void);
extern void soundfalloff(void);
extern void soundgold(void);
extern void drawsquareblob(int x, int y);
extern void drawfurryblob(int x, int y);
extern void eatfield(int x, int y, int dir);
extern void killemerald(int h, int v);
extern int diggery(void);
extern void killdigger(int death, int bag);
extern void squashmonsters(int bag, int mask);
extern int checkdiggerunderbag(int h, int v);
extern void checkmonscared(int h);
extern void scoregold(void);
extern void digresettime(void);
extern void mongold(void);

void initbags(void);
void drawbags(void);
void dobags(void);
void updatebag(int bag);
void baghitground(int bag);
int pushbag(int bag, int dir);
int pushbags(int dir, int mask);
int pushudbags(int mask);
void removebags(int mask);
void removebag(int bag);
int getnmovingbags(void);
void getgold(int bag);

/* CS:0x3B31 (file 0x3A31) */
void initbags(void)
{
    int bag, x, y;

    pushcount = 0;
    goldtime = 150 - levof10() * 10;

    for (bag = 1; bag <= 7; bag++)
        bagdat[bag].exist = 0;

    bag = 1;
    for (x = 0; x < 15; x++)
        for (y = 0; y < 10; y++)
            if (getlevch(x, y, levplan()) == 'B')
                if (bag < 8) {
                    bagdat[bag].exist = 1;
                    bagdat[bag].gt = 0;
                    bagdat[bag].fallh = 0;
                    bagdat[bag].dir = DIR_NONE;
                    bagdat[bag].wobbling = 0;
                    bagdat[bag].wt = 15;
                    bagdat[bag].unfallen = 1;
                    bagdat[bag].x = x * 20 + 12;
                    bagdat[bag].y = y * 18 + 18;
                    bagdat[bag].h = x;
                    bagdat[bag].v = y;
                    bagdat[bag].xr = 0;
                    bagdat[bag].yr = 0;
                    bag++;
                }

    if (curplayer_get() == 0)
        memcpy(bagdat1, bagdat, sizeof bagdat);
    else
        memcpy(bagdat2, bagdat, sizeof bagdat);
}

/* CS:0x3C93 (file 0x3B93) */
void drawbags(void)
{
    int bag;

    for (bag = 1; bag <= 7; bag++) {
        if (curplayer_get() == 0)
            memcpy(&bagdat[bag], &bagdat1[bag], sizeof(bagrec));
        else
            memcpy(&bagdat[bag], &bagdat2[bag], sizeof(bagrec));
        if (bagdat[bag].exist)
            movedrawspr(bag, bagdat[bag].x, bagdat[bag].y);
    }
}

/* CS:0x3E2D (file 0x3D2D) */
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
                else if (bagdat[bag].v < 9 && bagdat[bag].gt < goldtime - 10)
                    if (!(getfield(bagdat[bag].h, bagdat[bag].v + 1) & 0x2000))
                        bagdat[bag].gt = goldtime - 10;
            } else
                updatebag(bag);
        }

    for (bag = 1; bag <= 7; bag++) {
        if (bagdat[bag].dir == DIR_DOWN && bagdat[bag].exist)
            soundfalloffflag = 0;
        if (bagdat[bag].dir != DIR_DOWN && bagdat[bag].wobbling &&
            bagdat[bag].exist)
            soundwobbleoffflag = 0;
    }
    if (soundfalloffflag)
        soundfalloff();
    if (soundwobbleoffflag)
        soundwobbleoff();
}

/* CS:0x3FED (file 0x3EED) */
void updatebag(int bag)
{
    int x, h, xr, y, v, yr, wbl;

    x = bagdat[bag].x;
    h = bagdat[bag].h;
    xr = bagdat[bag].xr;
    y = bagdat[bag].y;
    v = bagdat[bag].v;
    yr = bagdat[bag].yr;

    switch (bagdat[bag].dir) {
    case DIR_NONE:
        if (y < 180 && xr == 0) {
            if (bagdat[bag].wobbling) {
                if (bagdat[bag].wt == 0) {
                    bagdat[bag].dir = DIR_DOWN;
                    soundfall();
                    break;
                }
                bagdat[bag].wt--;
                wbl = bagdat[bag].wt % 8;
                if (!(wbl & 1)) {
                    static const int wblanim[4] = { 1, 0, 2, 0 };
                    drawgold(bag, wblanim[wbl >> 1], x, y);
                    incpenalty();
                    soundwobble();
                }
            } else if ((getfield(h, v + 1) & 0xFDF) != 0xFDF) {
                if (!checkdiggerunderbag(h, v + 1))
                    bagdat[bag].wobbling = 1;
            }
        } else {
            bagdat[bag].wt = 15;
            bagdat[bag].wobbling = 0;
        }
        break;

    case DIR_RIGHT:
    case DIR_LEFT:
        if (xr == 0) {
            if (y < 180 && (getfield(h, v + 1) & 0xFDF) != 0xFDF) {
                bagdat[bag].dir = DIR_DOWN;
                bagdat[bag].wt = 0;
                soundfall();
            } else
                baghitground(bag);
        }
        break;

    case DIR_DOWN:
        if (yr == 0)
            bagdat[bag].fallh++;
        if (y >= 180)
            baghitground(bag);
        else if ((getfield(h, v + 1) & 0xFDF) == 0xFDF)
            if (yr == 0)
                baghitground(bag);
        checkmonscared(bagdat[bag].h);
        break;
    }

    if (bagdat[bag].dir != DIR_NONE) {
        if (bagdat[bag].dir != DIR_DOWN && pushcount != 0)
            pushcount--;
        else
            pushbag(bag, bagdat[bag].dir);
    }
}

/* CS:0x4296 (file 0x4196) */
void baghitground(int bag)
{
    int mask, i, bit;

    if (bagdat[bag].dir == DIR_DOWN && bagdat[bag].fallh > 1)
        bagdat[bag].gt = 1;
    else
        bagdat[bag].fallh = 0;
    bagdat[bag].dir = DIR_NONE;
    bagdat[bag].wt = 15;
    bagdat[bag].wobbling = 0;

    mask = drawgold(bag, 0, bagdat[bag].x, bagdat[bag].y);
    incpenalty();

    for (i = 1, bit = 2; i < 8; i++, bit <<= 1)
        if (mask & bit)
            removebag(i);
}

/* CS:0x433B (file 0x423B) */
int pushbag(int bag, int dir)
{
    int x, y, h, v, ox, oy, mask, push = 1;

    ox = x = bagdat[bag].x;
    oy = y = bagdat[bag].y;
    h = bagdat[bag].h;
    v = bagdat[bag].v;

    if (bagdat[bag].gt != 0) {
        getgold(bag);
        return 1;
    }

    /* A falling bag pushed sideways lands on whatever is below it. */
    if (bagdat[bag].dir == DIR_DOWN &&
        (dir == DIR_LEFT || dir == DIR_RIGHT)) {
        mask = drawgold(bag, 3, x, y);
        incpenalty();
        if (mask & COLL_DIGGER)
            if (diggery() >= y)
                killdigger(1, bag);
        if (mask & COLL_MON_BIT)
            squashmonsters(bag, mask);
        return 1;
    }

    if ((x == 292 && dir == DIR_RIGHT) ||
        (x == 12 && dir == DIR_LEFT) ||
        (y == 180 && dir == DIR_DOWN) ||
        (y == 18 && dir == DIR_UP))
        push = 0;

    if (!push)
        return 0;

    switch (dir) {
    case DIR_RIGHT:
        x += 4;
        break;
    case DIR_LEFT:
        x -= 4;
        break;
    case DIR_DOWN:
        if (bagdat[bag].unfallen) {
            bagdat[bag].unfallen = 0;
            drawsquareblob(x, y);
            drawtopblob(x, y + 21);
        } else
            drawfurryblob(x, y);
        eatfield(x, y, dir);
        killemerald(h, v);
        y += 6;
        break;
    }

    switch (dir) {
    case DIR_DOWN:
        mask = drawgold(bag, 3, x, y);
        incpenalty();
        if (mask & COLL_DIGGER)
            if (diggery() >= y)
                killdigger(1, bag);
        if (mask & COLL_MON_BIT)
            squashmonsters(bag, mask);
        break;

    case DIR_RIGHT:
    case DIR_LEFT:
        bagdat[bag].wt = 15;
        bagdat[bag].wobbling = 0;
        mask = drawgold(bag, 0, x, y);
        incpenalty();
        pushcount = 1;
        if ((mask & COLL_BAG_BIT) && !pushbags(dir, mask)) {
            x = ox;
            y = oy;
            drawgold(bag, 0, ox, oy);
            incpenalty();
            push = 0;
        }
        if ((mask & COLL_DIGGER) || (mask & COLL_MON_BIT)) {
            x = ox;
            y = oy;
            drawgold(bag, 0, ox, oy);
            incpenalty();
            push = 0;
        }
        break;
    }

    if (push)
        bagdat[bag].dir = dir;
    else
        bagdat[bag].dir = reversedir(dir);

    bagdat[bag].x = x;
    bagdat[bag].y = y;
    bagdat[bag].h = (x - 12) / 20;
    bagdat[bag].v = (y - 18) / 18;
    bagdat[bag].xr = (x - 12) % 20;
    bagdat[bag].yr = (y - 18) % 18;

    return push;
}

/* CS:0x466C (file 0x456C) */
int pushbags(int dir, int mask)
{
    int bag, bit, ok = 1;

    for (bag = 1, bit = 2; bag < 8; bag++, bit <<= 1)
        if (mask & bit)
            if (!pushbag(bag, dir))
                ok = 0;
    return ok;
}

/* CS:0x46B2 (file 0x45B2) */
int pushudbags(int mask)
{
    int bag, bit, ok = 1;

    for (bag = 1, bit = 2; bag < 8; bag++, bit <<= 1)
        if (mask & bit) {
            if (bagdat[bag].gt != 0)
                getgold(bag);
            else
                ok = 0;
        }
    return ok;
}

/* CS:0x47B5 (file 0x46B5) */
void removebags(int mask)
{
    int bag, bit;

    for (bag = 1, bit = 2; bag < 8; bag++, bit <<= 1)
        if (bagdat[bag].exist && (mask & bit))
            removebag(bag);
}

/* CS:0x4703 (file 0x4603) */
void removebag(int bag)
{
    if (bagdat[bag].exist) {
        bagdat[bag].exist = 0;
        erasespr(bag);
    }
}

/* CS:0x4730 (file 0x4630) */
int bagy(int bag)
{
    return bagdat[bag].y;
}

/* CS:0x4749 (file 0x4649) */
int getbagdir(int bag)
{
    if (bagdat[bag].exist)
        return bagdat[bag].dir;
    return -1;
}

/* CS:0x4774 (file 0x4674) */
int bagmask(void)
{
    int bag, bit, mask = 0;

    for (bag = 1, bit = 2; bag < 8; bag++, bit <<= 1)
        if (bagdat[bag].exist)
            mask |= bit;
    return mask;
}

/* CS:0x47F8 (file 0x46F8) */
int getnmovingbags(void)
{
    int bag, n = 0;

    for (bag = 1; bag <= 7; bag++)
        if (bagdat[bag].exist && bagdat[bag].gt < 10)
            if (bagdat[bag].gt != 0 || bagdat[bag].wobbling)
                n++;
    return n;
}

/* CS:0x4853 (file 0x4753) */
void getgold(int bag)
{
    int mask;

    mask = drawgold(bag, 6, bagdat[bag].x, bagdat[bag].y);
    incpenalty();
    if (mask & COLL_DIGGER) {
        scoregold();
        soundgold();
        digresettime();
    } else
        mongold();
    removebag(bag);
}

/* CS:0x3D2F (file 0x3C2F) */
void cleanupbags(void)
{
    int bag;

    soundfalloff();
    for (bag = 1; bag <= 7; bag++) {
        if (bagdat[bag].exist &&
            ((bagdat[bag].h == 7 && bagdat[bag].v == 9) ||
             bagdat[bag].xr != 0 || bagdat[bag].yr != 0 ||
             bagdat[bag].gt != 0 || bagdat[bag].fallh != 0 ||
             bagdat[bag].wobbling)) {
            bagdat[bag].exist = 0;
            erasespr(bag);
        }
        if (curplayer_get() == 0)
            memcpy(&bagdat1[bag], &bagdat[bag], sizeof(bagrec));
        else
            memcpy(&bagdat2[bag], &bagdat[bag], sizeof(bagrec));
    }
}
