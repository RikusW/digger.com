/*
 * Start-up, title screen, menu and the life loop.
 *
 *   main          CS:0x82A6 (file 0x81A6)
 *   game          CS:0x86F1 (per-frame loop; reconstructed in game.c notes)
 *   drawglyph     CS:0x08E5 (file 0x07E5)
 *   drawchar      CS:0x11E4 (file 0x10E4)
 *   writestring   CS:0x11A4 (file 0x10A4)
 *   diskcheck     CS:0x026C (file 0x016C)
 *   disk_signature CS:0x02F3 (file 0x01F3)
 *   read_sector2  CS:0x0321 (file 0x0221)
 *   hang          CS:0x08BE0 -> 0x08BEE (file 0x08AE0 -> 0x08AEE)
 *
 * The title text font is a table of 64 glyph pointers for characters
 * 0x20..0x5F.  Each glyph is [width in bytes][height][2bpp rows] in the
 * data segment; drawglyph picks a colour by masking every byte with the
 * 2-bit colour replicated four times (DS:0x0623).  Only space, '.', the
 * digits, A-Z and '_' have glyphs.
 *
 * The data segment also holds the string table:
 *   0x5018 "D I G G E R"   0x5024 twelve spaces   0x5031 "NOBBIN"
 *   0x5038 "HOBBIN"        0x503F "DIGGER"        0x5046 "GOLD"
 *   0x504B "EMERALD"       0x5053 "BONUS"         0x5059 "PLAYER "
 *   0x5061 "1"             0x5063 "2"             0x5065 spaces
 *
 * The alternate sprite draw order used by the title is at DS:0x4FBE:
 *   14,13,7,6,5,4,3,2,1,12,11,10,9,8,15,0
 */

#include "symbols.h"

extern u8 dsseg[0x10000];

/* Per-player game state (DS:0x4FE0 + 8*player, 0x4FE2 + 8*player, ...). */
static s16 curplayer;          /* DS:0x500C */
static s16 nplayers;           /* DS:0x500E */
static s16 levnotdrawn0;       /* DS:0x5010 */
static s16 levnotdrawn1;       /* DS:0x5012 */
static s16 level0;             /* DS:0x4FE2 */
static s16 level1;             /* DS:0x4FEA */
static s16 lives0;             /* DS:0x4FE0 */
static s16 lives1;             /* DS:0x4FE8 */
static s16 dead0;              /* DS:0x4FDE */
static s16 dead1;              /* DS:0x4FE6 */
static s16 checkbyte;          /* DS:0x5016 */
static s16 menuflag;           /* DS:0x0665 */
static volatile u8 keypressed; /* DS:0x0658 */

/* externs */
extern void setretr(int on);
extern void cga_init(void);
extern void gpal(int pal);
extern void ginten(int on);
extern void gclear(int color);
extern void cga_mode_off(void);
extern void cga_mode_on(void);
extern void blit_title(void);          /* CS:0x17EB */
extern void install_keyboard(void);    /* CS:0x0E06 */
extern void input_init(void);          /* CS:0x5B3F */
extern int readkey(void);              /* CS:0x5C5E */
extern void loadscores(void);
extern void initscores(void);          /* CS:0x5EEA */
extern void initsound(void);           /* CS:0x7D10 */
extern void playmusic(int n);          /* CS:0x72D7 */
extern void setorder(u16 *p);
extern void creatembspr(void);
extern void initmbspr(void);
extern void initlevel(void);
extern void game(void);                /* CS:0x86F1 */
extern void delay(int n);              /* CS:0x023F */
extern void movedrawspr(int spr, int x, int y);
extern int drawmon(int mon, int nob, int hdir, int x, int y);
extern int drawdig(int t, int x, int y, int f);
extern int drawgold(int bag, int frame, int x, int y);
extern void drawemerald(int x, int y);
extern void drawbonus(int x, int y);
extern void draw_highscores(void);     /* CS:0x6407 */
extern void draw_menu(void);           /* CS:0x646E */
extern void menu_redraw(void);         /* CS:0x8984 */
extern void gputich(int x, int y, int data, int w, int h, int color);

void main_title(void);
void drawglyph(int x, int y, int data, int color);
void drawchar(int ch, int x, int y, int color);
void writestring(int s, int x, int y, int color);
int diskcheck(void);
int disk_signature(void);
void read_sector2(void);
void hang(void);

/* Glyph pointers for characters 0x20..0x5F (0 = no glyph). */
static const u16 glyph[64] = {
    0x0C80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0C32, 0,
    0x0AAC, 0x0AD3, 0x0AFA, 0x0B21, 0x0B48, 0x0B6F, 0x0B96, 0x0BBD,
    0x0BE4, 0x0C0B,
    0, 0, 0, 0, 0, 0, 0,
    0x06B6, 0x06DD, 0x0704, 0x072B, 0x0752, 0x0779, 0x07A0, 0x07C7,
    0x07EE, 0x0815, 0x083C, 0x0863, 0x088A, 0x08B1, 0x08D8, 0x08FF,
    0x0926, 0x094D, 0x0974, 0x099B, 0x09C2, 0x09E9, 0x0A10, 0x0A37,
    0x0A5E, 0x0A85,
    0, 0, 0, 0, 0x0C59
};

/* CS:0x08E5 (file 0x07E5) */
void drawglyph(int x, int y, int data, int color)
{
    int w = dsseg[data];
    int h = dsseg[data + 1];

    gputich(x, y, data + 2, w, h, color);
}

/* CS:0x11E4 (file 0x10E4) */
void drawchar(int ch, int x, int y, int color)
{
    int idx = ch - 0x20;

    if (idx < 0 || idx >= 64)
        return;
    if (glyph[idx] == 0)
        return;
    drawglyph(x, y, glyph[idx], color);
}

/* CS:0x11A4 (file 0x10A4) */
void writestring(int s, int x, int y, int color)
{
    int i = 0;

    while (dsseg[s + i] != 0) {
        drawchar(dsseg[s + i], x, y, color);
        i++;
        x += 12;
    }
}

/* ---- copy protection ------------------------------------------------- */

/* CS:0x0321 (file 0x0221) */
void read_sector2(void)
{
    /* INT 13h AH=2, drive 0, head 0, cylinder 0x27, sector 2, 1 sector,
     * buffer DS:0x0046. */
}

/* CS:0x02F3 (file 0x01F3) */
int disk_signature(void)
{
    /* read_sector2();
     * if (carry || buffer[0] != 0x0E) return 0;
     * if (BIOS 0x40:0x42..0x44 != 0x40,0x20,0x20) return 0;
     * return 1; */
    return 1;
}

/* CS:0x026C (file 0x016C) */
int diskcheck(void)
{
    if (disk_signature() && disk_signature())
        return 0x45;
    return 0;
}

/* CS:0x08BE0 (file 0x08AE0) */
void hang(void)
{
    for (;;)
        ;
}

/* ---- start-up, title and life loop ----------------------------------- */

/* CS:0x82A6 (file 0x81A6) */
void main_title(void)
{
    int startflag = 0;         /* [bp+4] */
    int frame = 0;             /* [bp+6] */
    int animx = 0;             /* [bp+0xa] */
    int i, y;

    setretr(0);
    cga_init();
    setretr(1);
    gpal(0);
    ginten(0);

    install_keyboard();
    input_init();

    checkbyte = 0;
    for (i = 0; i < 10; i++) {
        checkbyte = (s16)diskcheck();
        if (checkbyte == 0x45)
            break;
    }
    if (checkbyte != 0x45)
        hang();

    loadscores();
    initsound();
    nplayers = 1;

restart:
    playmusic(0);
    setorder((u16 *)0x4FBE);   /* alternate title draw order */
    creatembspr();
    input_init();
    gclear(0);
    cga_mode_off();
    blit_title();
    writestring(0x5018, 0x64, 0, 3);   /* "D I G G E R" */
    draw_highscores();
    draw_menu();
    cga_mode_on();

    startflag = 0;
    menuflag = 0;
    frame = 0;

    for (;;) {
        if (startflag)
            break;

        startflag = readkey();

        if (menuflag) {
            menu_redraw();
            draw_highscores();
            keypressed = 0;
            menuflag = 0;
        }

        if (frame == 0)
            for (y = 0x36; y < 0xAE; y += 12)
                writestring(0x5024, 0xA4, y, 0);   /* clear menu lines */

        /* Scripted title animation: monsters, digger and pickups run
         * across the screen at fixed frame numbers. */
        if (frame == 0x32) {
            movedrawspr(9, 0x124, 0x3F);
            animx = 0x124;
        }
        if (frame > 0x32 && frame <= 0x4D) {
            animx -= 4;
            drawmon(0, 1, DIR_LEFT, animx, 0x3F);
        }
        if (frame > 0x4D)
            drawmon(0, 1, DIR_RIGHT, 0xB8, 0x3F);

        if (frame == 0x53)
            writestring(0x5031, 0xD8, 0x40, 2);    /* "NOBBIN" */

        if (frame == 0x5A) {
            movedrawspr(9, 0x124, 0x52);
            drawmon(0, 1, DIR_LEFT, 0x124, 0x52);
            animx = 0x124;
        }
        if (frame > 0x5A && frame <= 0x75) {
            animx -= 4;
            drawmon(0, 1, DIR_LEFT, animx, 0x52);
        }
        if (frame > 0x75)
            drawmon(0, 1, DIR_RIGHT, 0xB8, 0x52);

        if (frame == 0x7B)
            writestring(0x5038, 0xD8, 0x53, 2);    /* "HOBBIN" */

        if (frame == 0x82) {
            movedrawspr(0, 0x124, 0x65);
            drawdig(DIR_LEFT, 0x124, 0x65, 1);
            animx = 0x124;
        }
        if (frame > 0x82 && frame <= 0x9D) {
            animx -= 4;
            drawdig(DIR_LEFT, animx, 0x65, 1);
        }
        if (frame > 0x9D)
            drawdig(DIR_RIGHT, 0xB8, 0x65, 1);

        if (frame == 0xA3)
            writestring(0x503F, 0xD8, 0x66, 2);    /* "DIGGER" */

        if (frame == 0xB2) {
            movedrawspr(1, 0xB8, 0x78);
            drawgold(1, 0, 0xB8, 0x78);
        }
        if (frame == 0xB7)
            writestring(0x5046, 0xD8, 0x79, 2);    /* "GOLD" */

        if (frame == 0xC6)
            drawemerald(0xB8, 0x8D);
        if (frame == 0xCB)
            writestring(0x504B, 0xD8, 0x8C, 2);    /* "EMERALD" */

        if (frame == 0xDA)
            drawbonus(0xB8, 0x9E);
        if (frame == 0xDF)
            writestring(0x5053, 0xD8, 0x9F, 2);    /* "BONUS" */

        frame++;
        if (frame > 0xFA)
            frame = 0;

        /* Pace the animation; any ordinary key press cuts the wait short
         * and returns to the top of the menu loop. */
        for (i = 0; i < 0x2D; i++) {
            delay(1);
            if (keypressed != 0 && (keypressed & 0x80) == 0)
                goto next_frame;
        }
next_frame:
        ;
    }

    /* Start the game. */
    level0 = 1;
    lives0 = 3;
    if (nplayers == 2) {
        level1 = 1;
        lives1 = 3;
    } else
        lives1 = 0;

    gclear(0);
    curplayer = 0;
    initlevel();
    curplayer = 1;
    initlevel();
    initscores();
    levnotdrawn0 = 1;
    if (nplayers == 2)
        levnotdrawn1 = 1;
    curplayer = 0;

    for (;;) {
        if (lives0 == 0 && lives1 == 0)
            goto restart;

        if (curplayer == 0)
            dead0 = 0;
        else
            dead1 = 0;

        while (((curplayer == 0) ? dead0 : dead1) == 0 &&
               ((curplayer == 0) ? lives0 : lives1) != 0) {
            initmbspr();
            game();
        }

        if (curplayer == 0) {
            if (lives1 == 0)
                continue;
            curplayer = 1;
        } else {
            if (lives0 == 0)
                continue;
            curplayer = 0;
        }
        levnotdrawn0 = 1;
        levnotdrawn1 = 1;
    }
}
