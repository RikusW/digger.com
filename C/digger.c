/*
 * dodigger - reconstructed from CS:0x4939 (file 0x04839).
 *
 * The original keeps the current digger's state in fixed data-segment globals
 * (one digger at a time).  The port uses digdat[DIGGERS]; its simultaneous
 * two-player loop is a port extension, not present in this original build.
 *
 * Field map (DS offsets):
 *   x            0x3FDE    mdir         0x3FEA
 *   y            0x3FE0    bagtime      0x3FF0
 *   alive        0x3FF2    digvisible   0x3FF4
 *   notfiring    0x3FF6    rechargetime 0x3FF8
 *   expsn        0x4000    emocttime    0x4016
 *   bonusmode    0x4012    bonustimeleft 0x400E
 *   startbonustimeleft 0x400C
 *
 * Sub-functions:
 *   drawdig        CS:0x263F   (called with t,x,y,notfiring&&!rechargetime)
 *   updatefire     CS:0x4A3E
 *   drawexplosion  CS:0x4E66
 *   updatedigger   CS:0x4F07
 *   diggerdie      CS:0x5218
 */

#include "symbols.h"

static u16 dig_x;              /* DS:0x3FDE */
static u16 dig_y;              /* DS:0x3FE0 */
static u16 dig_mdir;           /* DS:0x3FEA */
static u16 dig_bagtime;        /* DS:0x3FF0 */
static u16 dig_alive;          /* DS:0x3FF2 */
static u16 dig_visible;        /* DS:0x3FF4 */
static u16 dig_notfiring;      /* DS:0x3FF6 */
static u16 dig_rechargetime;   /* DS:0x3FF8 */
static u16 dig_expsn;          /* DS:0x4000 */
static u16 emocttime;          /* DS:0x4016 */
static u16 bonusmode;          /* DS:0x4012 */
static u16 bonustimeleft;      /* DS:0x400E */
static u16 startbonustimeleft; /* DS:0x400C */

extern void drawdig(int t, int x, int y, int f);
extern void updatefire(void);
extern void drawexplosion(void);
extern void updatedigger(void);
extern void diggerdie(void);
extern void incpenalty(void);
extern void soundbonus(void);
extern void soundbonusoff(void);
extern void music(int tune);
extern void endbonusmode(void);
extern void cga_set_intensity(int on);

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
