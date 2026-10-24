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
static u16 dig_mdir;           /* DS:0x3FEA */
static u16 dig_ddir;           /* DS:0x3FEC */
static u16 dig_dir;            /* DS:0x3FEE */
static u16 dig_bagtime;        /* DS:0x3FF0 */
static u16 dig_alive;          /* DS:0x3FF2 */
static u16 dig_visible;        /* DS:0x3FF4 */
static u16 dig_notfiring;      /* DS:0x3FF6 */
static u16 dig_rechargetime;   /* DS:0x3FF8 */
static u16 dig_fx;             /* DS:0x3FFA */
static u16 dig_fy;             /* DS:0x3FFC */
static u16 dig_fdir;           /* DS:0x3FFE */
static u16 dig_expsn;          /* DS:0x4000 */
static u16 emocttime;          /* DS:0x4016 */
static u16 bonusmode;          /* DS:0x4012 */
static u16 bonustimeleft;      /* DS:0x400E */
static u16 startbonustimeleft; /* DS:0x400C */

extern void drawdig(int t, int x, int y, int f);
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

void updatefire(void);
void drawexplosion(void);
void killfire(void);

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
