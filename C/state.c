/*
 * Game-state accessors reconstructed from digger.com.
 *
 * These routines live in the high code region (CS:0x8A32..CS:0x8B55) and map
 * almost one-for-one to main.c/digger.c in the port.  The original addresses
 * are in the comments; the DS offsets are in symbols.h.
 */

#include "symbols.h"

/* Named views of the original data-segment fields. */
static u16 curplayer_g;    /* DS:0x500C */
static u16 nplayers_g;     /* DS:0x500E (probable) */
static u16 penalty_g;      /* DS:0x5014 */
static u16 level_g[2];     /* DS:0x4FE2, 0x4FEA (stride 8, level at +4) */
static u16 lives_a;        /* DS:0x4FE0 */
static u16 lives_b;        /* DS:0x4FE8 */

extern void drawlives(void);

/* CS:0x8A32 (file 0x8932) - matches port incpenalty(). */
void incpenalty(void)
{
    penalty_g++;
}

/* CS:0x8ABD (file 0x89BD) - matches port levno(). */
int levno(void)
{
    return level_g[curplayer_g];
}

/* CS:0x8A65 (file 0x8965) - matches port levplan(). */
int levplan(void)
{
    int l = levno();
    if (l > 8)
        l = (l % 4) + 5;          /* idiv 4; dx + 5  ==  (l & 3) + 5 */
    return l;
}

/* CS:0x8A8D (file 0x898D) - matches port levof10(). */
int levof10(void)
{
    int l = level_g[curplayer_g];
    return l > 10 ? 10 : l;
}

/*
 * CS:0x8AF0 (file 0x89F0) - getlives(argument).
 * The original returns DS:0x4FE0 for argument 1 and DS:0x4FE8 otherwise.
 * The argument-to-player mapping in the port needs checking.
 */
int getlives(int which)
{
    return which == 1 ? lives_a : lives_b;
}

/* CS:0x8B0C (file 0x8A0C) - addlife(argument); matches port addlife(). */
void addlife(int which)
{
    if (which == 1)
        lives_a++;
    else
        lives_b++;
    drawlives();                  /* call 0x790B */
}

/* CS:0x8B45 (file 0x8A45) - the port uses the curplayer global directly. */
int curplayer_get(void)
{
    return curplayer_g;
}

void set_nplayers(int n)
{
    nplayers_g = (u16)n;
}

void set_curplayer(int c)
{
    curplayer_g = (u16)c;
}

/* CS:0x8984 (file 0x8884) - toggle one/two players from the menu. */
void menu_redraw(void)
{
    nplayers_g = (nplayers_g == 1) ? 2 : 1;
}

/* CS:0x8B4D (file 0x8A4D) - probable nplayers getter. */
int nplayers_get(void)
{
    return nplayers_g;
}

/*
 * CS:0x8B55 (file 0x8A55) - testpause(), probable.
 *
 * Checks the key at DS:0x0658 for 0x39/0xB9; on pause it calls soundpause
 * (CS:0x7CB3), sets timer 2 to 40, calls setsoundt2, prints the pause text at
 * DS:0x508B, waits for a key and restores the screen.  Not reconstructed
 * line-by-line yet.
 */

/* CS:0x8B55 (file 0x8A55) */
extern volatile u8 keypressed;
extern void soundpause(void);
extern void soundpauseoff(void);
extern void sett2val(u16 v);
extern void setsoundt2(void);
extern void cleartopline(void);
extern void writestring(int s, int x, int y, int color);
extern void drawhighscore(void);
extern void addscore(int n);
extern void drawlives(void);
extern void delay(int n);

void testpause(void)
{
    u8 k = keypressed;

    if (k == 0x39 || k == 0xB9) {
        soundpause();
        sett2val(40);
        setsoundt2();
        cleartopline();
        writestring(0x508B, 0x50, 0, 1);      /* "PRESS ANY KEY" */
        while (keypressed == 0x39)
            ;
        while (keypressed & 0x80)
            ;
        cleartopline();
        drawhighscore();
        addscore(0);
        drawlives();
        delay(200);
        keypressed = 0;
    } else
        soundpauseoff();
}
