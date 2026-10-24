/*
 * Drawing and sprite layer - reconstructed from the original.
 *
 * Low-level sprite helpers (mapped, not reconstructed here):
 *   setspr       CS:0x0804  (spr, src0, src1, w, h, 0, 0)
 *   setspr2      CS:0x0754  (spr, src0, src1, src2, w, h, 0, 0)
 *   drawspr      CS:0x07E4  (spr, x, y) -> collision mask
 *   movedrawspr  CS:0x07B1
 *   initmiscspr  CS:0x084F  (x, y, w, h)
 *   drawmiscspr  CS:0x08AD  (x, y, src0, src1, w, h)
 *   getis        CS:0x0876
 *
 * High-level functions:
 *   creatembspr  CS:0x1826   initmbspr   CS:0x1A03
 *   createdbfspr CS:0x20F6   initdbfspr  CS:0x2167
 *   drawmon      CS:0x1B84   drawmondie  CS:0x1DDB
 *   drawgold     CS:0x1E9D   drawbonus   CS:0x260E
 *   drawemerald  CS:0x208A   eraseemerald CS:0x20C0
 *   drawrightblob CS:0x21CC  drawleftblob CS:0x2214
 *   drawtopblob  CS:0x225C   drawbottomblob CS:0x22A2
 *   drawfurryblob CS:0x22E8  drawsquareblob CS:0x2334
 *   drawbackg    CS:0x237A   drawfire    CS:0x24C6
 *   drawdig      CS:0x263F
 *
 * The original keeps its animation state in DS globals:
 *   0x16CC / 0x16D8   per-monster leg position/delta (6 entries)
 *   0x2700 / 0x2702   digger leg position/delta
 *   0x2704            fireball leg position
 *
 * initlevel / drawscreen / initchars keep the original call order.
 */

#include "symbols.h"

static u16 levdone;            /* per-player field, DS:0x4FE4 + 8*curplayer */
static u16 levnotdrawn;        /* DS:0x5010 */

static s16 monleg[MONSTERS];   /* DS:0x16CC */
static s16 mondelta[MONSTERS]; /* DS:0x16D8 */
static s16 digleg;             /* DS:0x2700 */
static s16 digdelta;           /* DS:0x2702 */
static s16 fireleg;            /* DS:0x2704 */

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

extern void setspr(int spr, int src0, int src1, int w, int h, int a, int b);
extern void setspr2(int spr, int src0, int src1, int src2, int w, int h,
                    int a, int b);
extern int drawspr(int spr, int x, int y);
extern void movedrawspr(int spr, int x, int y);
extern void initmiscspr(int x, int y, int w, int h);
extern void drawmiscspr(int x, int y, int src0, int src1, int w, int h);
extern void getis(void);

void initlevel(void);
void drawscreen(void);
void initchars(void);
void creatembspr(void);
void initmbspr(void);
void createdbfspr(void);
void initdbfspr(void);
void drawbackg(int plan);
void drawemerald(int x, int y);
void eraseemerald(int x, int y);
void drawrightblob(int x, int y);
void drawleftblob(int x, int y);
void drawtopblob(int x, int y);
void drawbottomblob(int x, int y);
void drawfurryblob(int x, int y);
void drawsquareblob(int x, int y);
int drawmon(int mon, int nob, int hdir, int x, int y);
int drawmondie(int mon, int nob, int hdir, int x, int y);
int drawgold(int bag, int frame, int x, int y);
void drawbonus(int x, int y);
int drawfire(int x, int y, int frame);
int drawdig(int t, int x, int y, int f);

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

/* ---- sprite data initialization -------------------------------------- */

/* CS:0x1826 (file 0x1726) */
void creatembspr(void)
{
    static const int bagframe[7] = {
        0x1950, 0x1C68, 0x1CA4, 0x1CE0, 0x1D1C, 0x1D58, 0x1D94
    };
    static const int monframe[6] = {
        0x2598, 0x25D4, 0x2610, 0x264C, 0x2688, 0x26C4
    };
    int i;

    for (i = 1; i <= 7; i++)
        setspr2(i, 0x18D6, 0x1913, bagframe[i - 1], 4, 15, 0, 0);
    for (i = 8; i <= 13; i++)
        setspr2(i, 0x1DD0, 0x1E0D, monframe[i - 8], 4, 15, 0, 0);

    createdbfspr();

    for (i = 0; i < MONSTERS; i++) {
        monleg[i] = 0;
        mondelta[i] = 1;
    }
}

/* CS:0x1A03 (file 0x1903) */
void initmbspr(void)
{
    int i;

    for (i = 1; i <= 7; i++)
        setspr(i, 0x18D6, 0x1913, 4, 15, 0, 0);
    for (i = 8; i <= 13; i++)
        setspr(i, 0x1DD0, 0x1E0D, 4, 15, 0, 0);

    initdbfspr();
}

/* CS:0x20F6 (file 0x1FF6) */
void createdbfspr(void)
{
    digdelta = 1;
    digleg = 0;
    fireleg = 0;

    setspr2(0, 0x2BA2, 0x2BDF, 0x2C1C, 4, 15, 0, 0);
    setspr2(0x0E, 0x2AEC, 0x2B29, 0x2B66, 4, 15, 0, 0);
    setspr2(0x0F, 0x2A2A, 0x2A39, 0x2A48, 2, 7, 0, 0);
}

/* CS:0x2167 (file 0x2067) */
void initdbfspr(void)
{
    digdelta = 1;
    digleg = 0;
    fireleg = 0;

    setspr(0, 0x2BA2, 0x2BDF, 4, 15, 0, 0);
    setspr(0x0E, 0x2AEC, 0x2B29, 4, 15, 0, 0);
    setspr(0x0F, 0x2A2A, 0x2A39, 2, 7, 0, 0);
}

/* ---- emerald and blob draws ------------------------------------------ */

/* CS:0x208A (file 0x1F8A) */
void drawemerald(int x, int y)
{
    initmiscspr(x, y, 4, 10);
    drawmiscspr(x, y, 0x16E4, 0x170D, 4, 10);
    getis();
}

/* CS:0x20C0 (file 0x1FC0) */
void eraseemerald(int x, int y)
{
    initmiscspr(x, y, 4, 10);
    drawmiscspr(x, y, 0x175E, 0x1787, 4, 10);
    getis();
}

/* CS:0x21CC (file 0x20CC) */
void drawrightblob(int x, int y)
{
    initmiscspr(x + 16, y - 1, 2, 18);
    drawmiscspr(x + 16, y - 1, 0x2706, 0x272B, 2, 18);
    getis();
}

/* CS:0x2214 (file 0x2114) */
void drawleftblob(int x, int y)
{
    initmiscspr(x - 8, y - 1, 2, 18);
    drawmiscspr(x - 8, y - 1, 0x2750, 0x2775, 2, 18);
    getis();
}

/* CS:0x225C (file 0x215C) */
void drawtopblob(int x, int y)
{
    initmiscspr(x - 4, y - 6, 6, 6);
    drawmiscspr(x - 4, y - 6, 0x279A, 0x27BF, 6, 6);
    getis();
}

/* CS:0x22A2 (file 0x21A2) */
void drawbottomblob(int x, int y)
{
    initmiscspr(x - 4, y + 15, 6, 6);
    drawmiscspr(x - 4, y + 15, 0x27E4, 0x2809, 6, 6);
    getis();
}

/* CS:0x22E8 (file 0x21E8) */
void drawfurryblob(int x, int y)
{
    initmiscspr(x - 4, y + 15, 6, 8);
    drawmiscspr(x - 4, y + 15, 0x282E, 0x285F, 6, 8);
    getis();
}

/* CS:0x2334 (file 0x2234) */
void drawsquareblob(int x, int y)
{
    initmiscspr(x - 4, y + 17, 6, 6);
    drawmiscspr(x - 4, y + 17, 0x2890, 0x28B5, 6, 6);
    getis();
}

/* ---- background ------------------------------------------------------ */

/* CS:0x237A (file 0x227A) */
void drawbackg(int plan)
{
    static const int tile[8][2] = {
        { 0x28DA, 0x28EF }, { 0x2904, 0x2919 },
        { 0x292E, 0x2943 }, { 0x2958, 0x296D },
        { 0x2982, 0x2997 }, { 0x29AC, 0x29C1 },
        { 0x29D6, 0x29EB }, { 0x2A00, 0x2A15 }
    };
    int x, y;

    if (plan < 0 || plan > 7)
        return;
    for (y = 14; y < 200; y++)
        for (x = 0; x < 320; x++)
            drawmiscspr(x, y, tile[plan][0], tile[plan][1], 5, 4);
}

/* ---- gold, bonus, fireball ------------------------------------------- */

/* CS:0x1E9D (file 0x1D9D) */
int drawgold(int bag, int frame, int x, int y)
{
    static const int gold[7][2] = {
        { 0x18D6, 0x1913 }, { 0x198C, 0x19C9 }, { 0x1A06, 0x1A43 },
        { 0x1A80, 0x1ABD }, { 0x1AFA, 0x1B37 }, { 0x1B74, 0x1BB1 },
        { 0x1BEE, 0x1C2B }
    };

    if (frame < 0 || frame > 6)
        return 0;
    setspr(bag, gold[frame][0], gold[frame][1], 4, 15, 0, 0);
    return drawspr(bag, x, y);
}

/* CS:0x260E (file 0x250E) */
void drawbonus(int x, int y)
{
    setspr(0x0E, 0x2AEC, 0x2B29, 4, 15, 0, 0);
    movedrawspr(0x0E, x, y);
}

/* CS:0x24C6 (file 0x23C6) */
int drawfire(int x, int y, int frame)
{
    static const int fire0[3][2] = {
        { 0x2A2A, 0x2A39 }, { 0x2A56, 0x2A65 }, { 0x2A74, 0x2A83 }
    };
    static const int fire[3][2] = {
        { 0x2A92, 0x2AA1 }, { 0x2AB0, 0x2ABF }, { 0x2ACE, 0x2ADD }
    };

    switch (frame) {
    case 0:
        fireleg++;
        if (fireleg > 2)
            fireleg = 0;
        setspr(0x0F, fire0[fireleg][0], fire0[fireleg][1], 2, 7, 0, 0);
        break;
    case 1:
        setspr(0x0F, fire[0][0], fire[0][1], 2, 7, 0, 0);
        break;
    case 2:
        setspr(0x0F, fire[1][0], fire[1][1], 2, 7, 0, 0);
        break;
    case 3:
        setspr(0x0F, fire[2][0], fire[2][1], 2, 7, 0, 0);
        break;
    }
    return drawspr(0x0F, x, y);
}

/* ---- monsters -------------------------------------------------------- */

/* CS:0x1DDB (file 0x1CDB) */
int drawmondie(int mon, int nob, int hdir, int x, int y)
{
    if (nob)
        setspr(mon + 8, 0x1F3E, 0x1F7B, 4, 15, 0, 0);
    else if (hdir == DIR_RIGHT)
        setspr(mon + 8, 0x2216, 0x2253, 4, 15, 0, 0);
    else if (hdir == DIR_LEFT)
        setspr(mon + 8, 0x24EE, 0x2527, 4, 14, 0, 0);
    return drawspr(mon + 8, x, y);
}

/* CS:0x1B84 (file 0x1A84) */
int drawmon(int mon, int nob, int hdir, int x, int y)
{
    static const int hobbin[3][2] = {
        { 0x1DD0, 0x1E0D }, { 0x1E4A, 0x1E87 }, { 0x1EC4, 0x1F01 }
    };
    static const int nob_right[3][2] = {
        { 0x1FF4, 0x2031 }, { 0x20AA, 0x20E7 }, { 0x2160, 0x219D }
    };
    static const int nob_left[3][2] = {
        { 0x22CC, 0x2309 }, { 0x2382, 0x23BF }, { 0x2438, 0x2475 }
    };
    int leg;

    monleg[mon] += mondelta[mon];
    if (monleg[mon] == 2 || monleg[mon] == 0)
        mondelta[mon] = -mondelta[mon];
    if (monleg[mon] > 2)
        monleg[mon] = 2;
    if (monleg[mon] < 0)
        monleg[mon] = 0;
    leg = monleg[mon];

    if (nob) {
        if (leg >= 0 && leg <= 2)
            setspr(mon + 8, hobbin[leg][0], hobbin[leg][1], 4, 15, 0, 0);
    } else if (hdir == DIR_RIGHT) {
        if (leg >= 0 && leg <= 2)
            setspr(mon + 8, nob_right[leg][0], nob_right[leg][1], 4, 15, 0, 0);
    } else if (hdir == DIR_LEFT) {
        if (leg >= 0 && leg <= 2)
            setspr(mon + 8, nob_left[leg][0], nob_left[leg][1], 4, 15, 0, 0);
    }
    /* hdir UP/DOWN keep the previously configured frame. */

    return drawspr(mon + 8, x, y);
}

/* ---- digger ---------------------------------------------------------- */

/* CS:0x263F (file 0x253F) */
int drawdig(int t, int x, int y, int f)
{
    /* [direction][leg][f ? 1 : 0][src0/src1] */
    static const int walk[4][3][2][2] = {
        {   /* DIR_RIGHT */
            { { 0x2C58, 0x2C95 }, { 0x2DC6, 0x2E03 } },
            { { 0x2CD2, 0x2D0F }, { 0x2E40, 0x2E7D } },
            { { 0x2D4C, 0x2D89 }, { 0x2EBA, 0x2EF7 } }
        },
        {   /* DIR_UP */
            { { 0x3210, 0x324D }, { 0x33BA, 0x33F7 } },
            { { 0x328A, 0x32C7 }, { 0x3434, 0x3471 } },
            { { 0x3340, 0x337D }, { 0x34EA, 0x3527 } }
        },
        {   /* DIR_LEFT */
            { { 0x2F34, 0x2F71 }, { 0x30A2, 0x30DF } },
            { { 0x2FAE, 0x2FEB }, { 0x311C, 0x3159 } },
            { { 0x3028, 0x3065 }, { 0x3196, 0x31D3 } }
        },
        {   /* DIR_DOWN */
            { { 0x3564, 0x35A1 }, { 0x36D2, 0x370F } },
            { { 0x35DE, 0x361B }, { 0x374C, 0x3789 } },
            { { 0x3658, 0x3695 }, { 0x37C6, 0x3803 } }
        }
    };
    static const int death[6][2] = {
        { 0x3840, 0x387D }, { 0x38BA, 0x38F7 }, { 0x3934, 0x3971 },
        { 0x39AE, 0x39EB }, { 0x3A28, 0x3A65 }, { 0x3AA2, 0x3AE7 }
    };
    int d, leg;

    digleg += digdelta;
    if (digleg == 2 || digleg == 0)
        digdelta = -digdelta;
    if (digleg > 2)
        digleg = 2;
    if (digleg < 0)
        digleg = 0;

    if (t >= 10 && t <= 15) {
        setspr(0, death[t - 10][0], death[t - 10][1], 4, 15, 0, 0);
        return drawspr(0, x, y);
    }

    switch (t) {
    case DIR_RIGHT: d = 0; break;
    case DIR_UP:    d = 1; break;
    case DIR_LEFT:  d = 2; break;
    case DIR_DOWN:  d = 3; break;
    default:
        return 0;              /* the original returns without drawing */
    }

    leg = digleg;
    if (leg < 0 || leg > 2)
        return 0;
    setspr(0, walk[d][leg][f ? 1 : 0][0], walk[d][leg][f ? 1 : 0][1],
           4, 15, 0, 0);
    return drawspr(0, x, y);
}
