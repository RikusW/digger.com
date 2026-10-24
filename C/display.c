/*
 * Score and menu display - reconstructed from the original.
 *
 *   cleartopline  CS:0x8A3B (file 0x893B)
 *   draw_number   CS:0x1617 (main.c)
 *   draw_digit    CS:0x1681 (main.c)
 *   drawscore     CS:0x5F2E (file 0x5E2E)
 *   drawhighscore CS:0x62D0 (file 0x61D0)
 *   drawlife      CS:0x201A (file 0x1F1A)
 *   drawlives     CS:0x6345 (file 0x6245)
 *   draw_highscores CS:0x6407 (file 0x6307)
 *   draw_menu     CS:0x646E (file 0x636E)
 *   menu_redraw   CS:0x8984 (file 0x8884)
 *   gameover      CS:0x610B (file 0x600B)
 */

#include "symbols.h"

extern u8 dsseg[0x10000];

extern int curplayer_get(void);
extern int nplayers_get(void);
extern int getlives(int which);
extern u32 score_get(int player);
extern void writestring(int s, int x, int y, int color);
extern void draw_number(u32 value, int x, int y, int digits, int color);
extern void drawmiscspr(int x, int y, int data, int mask, int w, int h);
extern void strcpy_ds(int dst, int src);
extern void strcat_ds(int dst, int src);
extern void format_score(int buf, u32 value, int pad);
extern void gclear(int color);
extern void gpal(int pal);
extern void ginten(int on);
extern void setretr(int on);
extern void delay(int n);
extern void addscore(int n);
extern void enter_initials(void);
extern void insert_score(u32 score);
extern u32 scorehigh_get(int i);
extern void savescores(void);
extern void setsoundt2(void);
extern void timer2(int v);
extern void stopint8(void);
extern void startint8(void);

void cleartopline(void);
void drawscore(int color);
void drawhighscore(void);
void drawlife(int frame, int x, int y);
void drawlives(void);
void draw_highscores(void);
void draw_menu(void);
void gameover(void);

/* CS:0x8A3B (file 0x893B) */
void cleartopline(void)
{
    writestring(0x506E, 0, 0, 3);        /* 26 spaces */
    writestring(0x5089, 0x134, 0, 3);    /* one space at the right edge */
}

/* CS:0x5F2E (file 0x5E2E) */
void drawscore(int color)
{
    if (curplayer_get() == 0)
        draw_number(score_get(0), 0, 0, 6, color);
    else
        draw_number(score_get(1),
                    score_get(1) < 100000u ? 0xEC : 0xF8, 0, 6, color);
}

/* CS:0x62D0 (file 0x61D0) */
void drawhighscore(void)
{
    draw_number(score_get(0), 0, 0, 6, 3);
    if (nplayers_get() == 2)
        draw_number(score_get(1),
                    score_get(1) < 100000u ? 0xEC : 0xF8, 0, 6, 3);
}

/* CS:0x201A (file 0x1F1A) */
void drawlife(int frame, int x, int y)
{
    static const int src[3][2] = {
        { 0x17B0, 0x17E1 }, { 0x1812, 0x1843 }, { 0x1874, 0x18A5 }
    };

    if (frame < 0 || frame > 2)
        return;
    drawmiscspr(x, y, src[frame][0], src[frame][1], 4, 12);
}

/* CS:0x6345 (file 0x6245) */
void drawlives(void)
{
    int i, n;

    n = getlives(1) - 1;
    for (i = 1; i < 5; i++) {
        drawlife(n > 0 ? 0 : 2, i * 20 + 0x3C, 0);
        n--;
    }

    if (nplayers_get() == 2) {
        n = getlives(2) - 1;
        for (i = 1; i < 5; i++) {
            drawlife(n > 0 ? 1 : 2, 0xF4 - i * 20, 0);
            n--;
        }
    }
}

/* CS:0x6407 (file 0x6307) */
void draw_highscores(void)
{
    if (nplayers_get() == 1) {
        writestring(0x4197, 0xDC, 0x19, 3);   /* "ONE" */
        writestring(0x419B, 0xC0, 0x27, 3);   /* " PLAYER " */
    } else {
        writestring(0x41A4, 0xDC, 0x19, 3);   /* "TWO" */
        writestring(0x41A8, 0xB8, 0x27, 3);   /* " PLAYERS" */
    }
}

/* CS:0x646E (file 0x636E) */
void draw_menu(void)
{
    int i, color = 2;

    writestring(0x41B1, 0x10, 0x19, 3);       /* "HIGH SCORES" */

    for (i = 1; i < 11; i++) {
        strcpy_ds(0x40E0, 0x41BD);            /* empty */
        strcat_ds(0x40E0, 0x4134 + i * 4);    /* initials */
        strcat_ds(0x40E0, 0x41BE);            /* "  " */
        format_score(0x40D6, scorehigh_get(i), 1);
        strcat_ds(0x40E0, 0x40D6);
        writestring(0x40E0, 0x10, i * 13 + 0x1F, color);
        color = 1;
    }
}

/* CS:0x610B (file 0x600B) */
void gameover(void)
{
    int i, j, k;
    u32 score = score_get(curplayer_get());

    addscore(0);

    if (score > scorehigh_get(10)) {
        gclear(0);
        drawhighscore();
        strcpy_ds(0x4FEE, 0x4166);      /* "PLAYER " */
        strcat_ds(0x4FEE, curplayer_get() == 0 ? 0x416E : 0x4170);
        writestring(0x4FEE, 0x6C, 0, 2);
        writestring(0x4172, 0x40, 0x28, 2);   /* " NEW HIGH SCORE " */
        enter_initials();
        insert_score(score);
        savescores();
        return;
    }

    cleartopline();
    writestring(0x4183, 0x68, 0, 3);    /* "GAME OVER" */
    setsoundt2();
    timer2(40);
    stopint8();

    for (i = 0; i < 20; i++) {
        for (j = 0; j < 8; j++) {
            setretr(1);
            gpal(1);
            ginten(1);
            setretr(0);
            for (k = 0; k < 0x6F; k++)
                ;
            gpal(0);
            ginten(0);
        }
        for (j = 0; j < 8; j++) {
            setretr(1);
            ginten(1);
            gpal(0);
            setretr(0);
            for (k = 0; k < 0x6F; k++)
                ;
            gpal(0);
            ginten(0);
        }
    }

    startint8();
    writestring(0x418D, 0x68, 0, 3);    /* blanks out "GAME OVER" */
    setretr(1);
}
