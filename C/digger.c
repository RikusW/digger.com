/*
 * dodigger / updatefire / drawexplosion / killfire - reconstructed from the
 * original.
 *
 * The original keeps the current digger's state in fixed data-segment globals
 * (one digger at a time).  The port uses digdat[DIGGERS]; its simultaneous
 * two-player loop is a port extension, not present in this original build.
 *
 * Field map (DS offsets):
 *   x            0x3FDE    y            0x3FE0
 *   h            0x3FE2    v            0x3FE4
 *   rx           0x3FE6    ry           0x3FE8
 *   mdir         0x3FEA    ddir         0x3FEC  (key direction, port local)
 *   dir          0x3FEE    bagtime      0x3FF0
 *   alive        0x3FF2    digvisible   0x3FF4
 *   notfiring    0x3FF6    rechargetime 0x3FF8
 *   fx           0x3FFA    fy           0x3FFC
 *   fdir         0x3FFE    expsn        0x4000
 *   deathstage   0x4002    deathbag     0x4004
 *   emocttime    0x4016    bonusmode    0x4012
 *   bonustimeleft 0x400E   startbonustimeleft 0x400C
 *   msc          0x4014
 *
 * Sub-functions:
 *   drawdig        CS:0x263F   (called with t,x,y,notfiring&&!rechargetime)
 *   updatefire     CS:0x4A3E
 *   drawexplosion  CS:0x4E66
 *   killfire       CS:0x4EE3
 *   updatedigger   CS:0x4F07
 *   diggerdie      CS:0x5218
 */

#include "symbols.h"

static u16 dig_x;              /* DS:0x3FDE */
static u16 dig_y;              /* DS:0x3FE0 */
static u16 dig_h;              /* DS:0x3FE2 */
static u16 dig_v;              /* DS:0x3FE4 */
static u16 dig_rx;             /* DS:0x3FE6 */
static u16 dig_ry;             /* DS:0x3FE8 */
static s16 dig_mdir;           /* DS:0x3FEA */
static s16 dig_ddir;           /* DS:0x3FEC */
static s16 dig_dir;            /* DS:0x3FEE */
static u16 dig_bagtime;        /* DS:0x3FF0 */
static u16 dig_alive;          /* DS:0x3FF2 */
static u16 dig_visible;        /* DS:0x3FF4 */
static u16 dig_notfiring;      /* DS:0x3FF6 */
static u16 dig_rechargetime;   /* DS:0x3FF8 */
static u16 dig_fx;             /* DS:0x3FFA */
static u16 dig_fy;             /* DS:0x3FFC */
static u16 dig_fdir;           /* DS:0x3FFE */
static u16 dig_expsn;          /* DS:0x4000 */
static u16 dig_deathstage;     /* DS:0x4002 */
static u16 dig_deathbag;       /* DS:0x4004 */
static u16 dig_deathani;       /* DS:0x4006 */
static u16 dig_deathtime;      /* DS:0x4008 */
static u16 emocttime;          /* DS:0x4016 */
static u16 bonusmode;          /* DS:0x4012 */
static u16 bonustimeleft;      /* DS:0x400E */
static u16 startbonustimeleft; /* DS:0x400C */

extern int drawdig(int t, int x, int y, int f);
extern void updatedigger(void);
extern void diggerdie(void);
extern void incpenalty(void);
extern void soundbonus(void);
extern void soundbonusoff(void);
extern void music(int tune);
extern void endbonusmode(void);
extern void cga_set_intensity(int on);

extern int levof10(void);
extern int getfirepflag(void);
extern void movedrawspr(int spr, int x, int y);
extern void erasespr(int spr);
extern int ggetpix(int x, int y);
extern int drawfire(int x, int y, int frame);
extern void killmon(int mon);
extern void scorekill(void);
extern void soundfire(void);
extern void soundfireoff(void);
extern void soundexplode(void);

extern void readdir(void);
extern int getdir(void);
extern int reversedir(int dir);
extern int hitemerald(int x, int y, int rx, int ry, int dir);
extern void eatfield(int x, int y, int dir);
extern void drawrightblob(int x, int y);
extern void drawleftblob(int x, int y);
extern void drawtopblob(int x, int y);
extern void drawbottomblob(int x, int y);
extern void soundeatm(void);
extern void sceatm(void);
extern void soundddie(void);
extern void musicoff(void);
extern void setdead(int dead);
extern int getbagdir(int bag);
extern int bagy(int bag);
extern int musicflag;          /* DS:0x0669 */
extern int bagmask(void);
extern int pushbags(int dir, int mask);
extern int pushudbags(int mask);
extern int killmonsters(int mask);
extern int bonusmode_get(void);
extern void scoreemerald(void);
extern void scorebonus(void);
extern void soundem(void);
extern void soundemerald(int start);
extern void initbonusmode(void);

void updatefire(void);
void drawexplosion(void);
void killfire(void);
int checkdiggerunderbag(int h, int v);

void dodigger(void)
{
    if (dig_expsn)
        drawexplosion();
    else
        updatefire();

    if (dig_visible) {
        if (dig_alive) {
            if (dig_bagtime) {
                drawdig(dig_mdir, dig_x, dig_y,
                        dig_notfiring && !dig_rechargetime);
                incpenalty();
                dig_bagtime--;
            } else
                updatedigger();
        } else
            diggerdie();
    }

    if (bonusmode && dig_alive) {
        if (bonustimeleft) {
            bonustimeleft--;
            if (startbonustimeleft || bonustimeleft < 20) {
                startbonustimeleft--;
                if (bonustimeleft & 1) {
                    cga_set_intensity(0);
                    soundbonus();
                } else {
                    cga_set_intensity(1);
                    soundbonus();
                }
                if (startbonustimeleft == 0) {
                    music(0);
                    soundbonusoff();
                    cga_set_intensity(1);
                }
            }
        } else {
            endbonusmode();
            soundbonusoff();
            music(1);
        }
    }

    if (bonusmode && !dig_alive) {
        endbonusmode();
        soundbonusoff();
        music(1);
    }

    if (emocttime > 0)
        emocttime--;
}

/* CS:0x4A3E (file 0x493E) */
void updatefire(void)
{
    int pix = 0;
    int coll, i, bit;

    if (dig_notfiring) {
        if (dig_rechargetime != 0) {
            dig_rechargetime--;
            return;
        }
        if (!getfirepflag())
            return;
        if (!dig_alive)
            return;

        dig_rechargetime = levof10() * 3 + 60;
        dig_notfiring = 0;

        switch (dig_dir) {
        case DIR_RIGHT:
            dig_fx = dig_x + 8;
            dig_fy = dig_y + 4;
            break;
        case DIR_UP:
            dig_fx = dig_x;
            dig_fy = dig_y + 4;
            break;
        case DIR_LEFT:
            dig_fx = dig_x + 4;
            dig_fy = dig_y;
            break;
        case DIR_DOWN:
            dig_fx = dig_x + 4;
            dig_fy = dig_y + 8;
            break;
        }

        dig_fdir = dig_dir;
        movedrawspr(15, dig_fx, dig_fy);
        soundfire();
        return;
    }

    switch (dig_fdir) {
    case DIR_RIGHT:
        dig_fx += 8;
        pix = ggetpix(dig_fx, dig_fy + 4) |
              ggetpix(dig_fx + 4, dig_fy + 4);
        break;
    case DIR_UP:
        dig_fy -= 7;
        pix = (ggetpix(dig_fx + 4, dig_fy)     & 0xC0) |
              (ggetpix(dig_fx + 4, dig_fy + 1) & 0xC0) |
              (ggetpix(dig_fx + 4, dig_fy + 2) & 0xC0) |
              (ggetpix(dig_fx + 4, dig_fy + 3) & 0xC0) |
              (ggetpix(dig_fx + 4, dig_fy + 4) & 0xC0) |
              (ggetpix(dig_fx + 4, dig_fy + 5) & 0xC0) |
              (ggetpix(dig_fx + 4, dig_fy + 6) & 0xC0);
        break;
    case DIR_LEFT:
        dig_fx -= 8;
        pix = ggetpix(dig_fx, dig_fy + 4) |
              ggetpix(dig_fx + 4, dig_fy + 4);
        break;
    case DIR_DOWN:
        dig_fy += 7;
        pix = (ggetpix(dig_fx, dig_fy)     & 0x03) |
              (ggetpix(dig_fx, dig_fy + 1) & 0x03) |
              (ggetpix(dig_fx, dig_fy + 2) & 0x03) |
              (ggetpix(dig_fx, dig_fy + 3) & 0x03) |
              (ggetpix(dig_fx, dig_fy + 4) & 0x03) |
              (ggetpix(dig_fx, dig_fy + 5) & 0x03) |
              (ggetpix(dig_fx, dig_fy + 6) & 0x03);
        break;
    }

    coll = drawfire(dig_fx, dig_fy, 0);
    incpenalty();

    /* Hit monsters. */
    if (coll & COLL_MON_BIT) {
        bit = 0x0100;
        for (i = 0; i < MONSTERS; i++, bit <<= 1)
            if (coll & bit) {
                killmon(i);
                scorekill();
                dig_expsn = 1;
            }
    }

    /* Hit anything else (bags, the bonus, the other fireball). */
    if (coll & 0x40FE)
        dig_expsn = 1;

    /* Move or explode at the edge / on impact. */
    switch (dig_fdir) {
    case DIR_RIGHT:
        if (dig_fx > 296)
            dig_expsn = 1;
        else if (pix != 0 && coll == 0) {
            dig_expsn = 1;
            dig_fx -= 8;
            drawfire(dig_fx, dig_fy, 0);
        }
        break;
    case DIR_UP:
        if (dig_fy < 15)
            dig_expsn = 1;
        else if (pix != 0 && coll == 0) {
            dig_expsn = 1;
            dig_fy += 7;
            drawfire(dig_fx, dig_fy, 0);
        }
        break;
    case DIR_LEFT:
        if (dig_fx < 16)
            dig_expsn = 1;
        else if (pix != 0 && coll == 0) {
            dig_expsn = 1;
            dig_fx += 8;
            drawfire(dig_fx, dig_fy, 0);
        }
        break;
    case DIR_DOWN:
        if (dig_fy > 183)
            dig_expsn = 1;
        else if (pix != 0 && coll == 0) {
            dig_expsn = 1;
            dig_fy -= 7;
            drawfire(dig_fx, dig_fy, 0);
        }
        break;
    }
}

/* CS:0x4E66 (file 0x4D66) */
void drawexplosion(void)
{
    switch (dig_expsn) {
    case 1:
        soundexplode();
        drawfire(dig_fx, dig_fy, 1);
        incpenalty();
        dig_expsn = 2;
        break;
    case 2:
        drawfire(dig_fx, dig_fy, 2);
        incpenalty();
        dig_expsn = 3;
        break;
    case 3:
        drawfire(dig_fx, dig_fy, 3);
        incpenalty();
        dig_expsn = 4;
        break;
    default:
        killfire();
        dig_expsn = 0;
    }
}

/* CS:0x4EE3 (file 0x4DE3) */
void killfire(void)
{
    if (!dig_notfiring) {
        dig_notfiring = 1;
        erasespr(15);
        soundfireoff();
    }
}

/* CS:0x4F07 (file 0x4E07) */
void updatedigger(void)
{
    int dir, ddir, diggerox, diggeroy, mask, push, nmon;

    readdir();
    dir = getdir();
    if (dir == DIR_RIGHT || dir == DIR_UP || dir == DIR_LEFT || dir == DIR_DOWN)
        ddir = dir;
    else
        ddir = DIR_NONE;

    if (dig_rx == 0 && (ddir == DIR_UP || ddir == DIR_DOWN))
        dig_dir = dig_mdir = ddir;
    if (dig_ry == 0 && (ddir == DIR_RIGHT || ddir == DIR_LEFT))
        dig_dir = dig_mdir = ddir;

    if (dir == DIR_NONE)
        dig_mdir = DIR_NONE;
    else
        dig_mdir = dig_dir;

    if ((dig_x == 292 && dig_mdir == DIR_RIGHT) ||
        (dig_x == 12 && dig_mdir == DIR_LEFT) ||
        (dig_y == 180 && dig_mdir == DIR_DOWN) ||
        (dig_y == 18 && dig_mdir == DIR_UP))
        dig_mdir = DIR_NONE;

    diggerox = dig_x;
    diggeroy = dig_y;

    if (dig_mdir != DIR_NONE)
        eatfield(diggerox, diggeroy, dig_mdir);

    switch (dig_mdir) {
    case DIR_RIGHT:
        drawrightblob(dig_x, dig_y);
        dig_x += 4;
        break;
    case DIR_UP:
        drawtopblob(dig_x, dig_y);
        dig_y -= 3;
        break;
    case DIR_LEFT:
        drawleftblob(dig_x, dig_y);
        dig_x -= 4;
        break;
    case DIR_DOWN:
        drawbottomblob(dig_x, dig_y);
        dig_y += 3;
        break;
    }

    if (hitemerald((dig_x - 12) / 20, (dig_y - 18) / 18,
                   (dig_x - 12) % 20, (dig_y - 18) % 18, dig_mdir)) {
        scoreemerald();
        soundem();
        /* The original passes the emerald cooldown as a start flag;
         * soundemerald steps the scale itself. */
        soundemerald(emocttime);
        emocttime = 9;
    }

    mask = drawdig(dig_dir, dig_x, dig_y,
                   dig_notfiring && dig_rechargetime == 0);
    incpenalty();

    /* Push a bag.  The original only initialises `push` on the horizontal
     * path; on the vertical path the success value is left in the local.
     * Initialise it to 1 here so the behaviour is defined. */
    push = 1;
    if ((mask & COLL_BAG_BIT) & bagmask()) {
        if (dig_mdir == DIR_RIGHT || dig_mdir == DIR_LEFT) {
            push = pushbags(dig_mdir, mask);
            dig_bagtime++;
        } else if (!pushudbags(mask))
            push = 0;
        if (!push) {
            dig_x = diggerox;
            dig_y = diggeroy;
            drawdig(dig_mdir, dig_x, dig_y,
                    dig_notfiring && dig_rechargetime == 0);
            incpenalty();
            dig_dir = reversedir(dig_mdir);
        }
    }

    if ((mask & COLL_MON_BIT) && bonusmode_get()) {
        nmon = killmonsters(mask);
        while (nmon != 0) {
            soundeatm();
            sceatm();
            nmon--;
        }
    }

    if (mask & 0x4000) {       /* bonus sprite */
        scorebonus();
        initbonusmode();
    }

    dig_h = (dig_x - 12) / 20;
    dig_rx = (dig_x - 12) % 20;
    dig_v = (dig_y - 18) / 18;
    dig_ry = (dig_y - 18) % 18;
}

/* CS:0x5602 (file 0x5502) */
int checkdiggerunderbag(int h, int v)
{
    if (dig_mdir == DIR_UP || dig_mdir == DIR_DOWN)
        if ((dig_x - 12) / 20 == h)
            if ((dig_y - 18) / 18 == v || (dig_y - 18) / 18 + 1 == v)
                return 1;
    return 0;
}

/* CS:0x5218 (file 0x5118) */
void diggerdie(void)
{
    int mask;

    switch (dig_deathstage) {
    case 1:
        if (bagy(dig_deathbag) + 6 > dig_y)
            dig_y = bagy(dig_deathbag) + 6;
        drawdig(15, dig_x, dig_y, 0);
        incpenalty();
        if (getbagdir(dig_deathbag) + 1 == 0) {
            soundddie();
            dig_deathtime = 5;
            dig_deathstage = 2;
            dig_deathani = 0;
            dig_y -= 6;
        }
        break;

    case 2:
        if (dig_deathtime != 0) {
            dig_deathtime--;
            break;
        }
        switch (dig_deathani) {
        case 0:
            music(2);
            mask = drawdig(14, dig_x, dig_y, 0);
            incpenalty();
            if (mask & COLL_MON_BIT)
                killmonsters(mask);
            dig_deathani = 1;
            dig_deathtime = 2;
            break;
        case 1:
            drawdig(13, dig_x, dig_y, 0);
            incpenalty();
            dig_deathtime = 2;
            dig_deathani = 2;
            break;
        case 2:
            drawdig(12, dig_x, dig_y, 0);
            incpenalty();
            dig_deathani = 3;
            dig_deathtime = 2;
            break;
        case 3:
            drawdig(11, dig_x, dig_y, 0);
            incpenalty();
            dig_deathani = 4;
            dig_deathtime = 2;
            break;
        case 4:
            drawdig(10, dig_x, dig_y, 0);
            incpenalty();
            dig_deathstage = 4;
            if (musicflag)
                dig_deathtime = 60;
            else
                dig_deathtime = 10;
            break;
        }
        break;

    case 3:
        dig_deathstage = 5;
        dig_deathani = 0;
        dig_deathtime = 0;
        break;

    case 4:
        if (dig_deathtime != 0)
            dig_deathtime--;
        else
            setdead(1);
        break;

    case 5:
        /* Sinking into the ground: the arc is 3,5,6,6,5,3,0. */
        switch (dig_deathani) {
        case 0:
            drawdig(15, dig_x, dig_y - 3, 0);
            incpenalty();
            dig_deathani = 1;
            soundddie();
            break;
        case 1:
            drawdig(15, dig_x, dig_y - 5, 0);
            incpenalty();
            dig_deathani = 2;
            break;
        case 2:
            drawdig(15, dig_x, dig_y - 6, 0);
            incpenalty();
            dig_deathani = 3;
            break;
        case 3:
            drawdig(15, dig_x, dig_y - 6, 0);
            incpenalty();
            dig_deathani = 4;
            break;
        case 4:
            drawdig(15, dig_x, dig_y - 5, 0);
            incpenalty();
            dig_deathani = 5;
            break;
        case 5:
            drawdig(15, dig_x, dig_y - 3, 0);
            incpenalty();
            dig_deathani = 6;
            break;
        case 6:
            drawdig(15, dig_x, dig_y, 0);
            musicoff();
            incpenalty();
            dig_deathtime = 5;
            dig_deathani = 0;
            dig_deathstage = 2;
            break;
        }
        break;
    }
}

/* ---- accessors and small state transitions --------------------------- */

/* CS:0x5650 / 0x5658 / 0x5660 (files 0x5550/0x5558/0x5560) */
int diggerx(void)
{
    return dig_x;
}

int diggery(void)
{
    return dig_y;
}

int digalive(void)
{
    return dig_alive;
}

/* CS:0x569B (file 0x559B) */
void digresettime(void)
{
    dig_bagtime = 0;
}

/* CS:0x55B5 (file 0x54B5) */
int bonusmode_get(void)
{
    return bonusmode;
}

/* CS:0x55BD (file 0x54BD) */
int reversedir(int dir)
{
    switch (dir) {
    case DIR_RIGHT: return DIR_LEFT;
    case DIR_UP:    return DIR_DOWN;
    case DIR_LEFT:  return DIR_RIGHT;
    case DIR_DOWN:  return DIR_UP;
    }
    return dir;
}

/* DS:0x40BC - the current fire-key state maintained by the input layer. */
static s16 firepflag;          /* DS:0x40BC */

extern u8 cga_mem[0x4000];

/* CS:0x5DD8 (file 0x5CD8) */
int getfirepflag(void)
{
    return firepflag;
}

void setfirepflag(int v)
{
    firepflag = (s16)v;
}

/* CS:0x56A6 (file 0x55A6) */
int ggetpix(int x, int y)
{
    /* The original reads the CGA framebuffer directly:
     *   addr = (y & ~1) * 40 + (y & 1) * 0x2000 + (x >> 2)
     * and extracts the 2-bit pixel at (3 - (x & 3)) * 2. */
    int addr = (y & ~1) * 40 + ((y & 1) ? 0x2000 : 0) + (x >> 2);
    return (cga_mem[addr] >> ((3 - (x & 3)) * 2)) & 3;
}

/* CS:0x5668 (file 0x5568) */
void killdigger(int death, int bag)
{
    if (dig_deathstage == 2 || dig_deathstage == 4 || dig_deathstage == 3)
        return;
    musicoff();
    dig_alive = 0;
    dig_deathstage = (u16)death;
    dig_deathbag = (u16)bag;
}

/* CS:0x48A0 (file 0x47A0) */
void initdigger(void)
{
    dig_h = 7;
    dig_v = 9;
    dig_x = dig_h * 20 + 12;
    dig_y = dig_v * 18 + 18;
    dig_ddir = DIR_LEFT;
    dig_mdir = DIR_LEFT;
    dig_dir = DIR_RIGHT;
    dig_rx = 0;
    dig_ry = 0;
    dig_bagtime = 0;
    dig_alive = 1;
    dig_deathstage = 1;
    dig_visible = 1;
    movedrawspr(0, dig_x, dig_y);
    dig_notfiring = 1;
    dig_expsn = 0;
    emocttime = 0;
    bonusmode = 0;
    dig_rechargetime = 0;
}

/* CS:0x4926 (file 0x4826) */
void erasediggers(void)
{
    erasespr(0);
    dig_visible = 0;
}
