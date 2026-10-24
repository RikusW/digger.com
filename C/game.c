/*
 * Game loop, score storage, and bonus-mode fragments reconstructed from
 * digger.com.
 *
 * Addresses are CS:IP; the original code was compiled with a 16-bit small
 * model where the local frame is below BP, so arguments start at BP+N+4.
 */

#include "symbols.h"

extern void drawlives(void);

/* ---- score event wrappers -------------------------------------------- */

/*
 * CS:0x68A2..0x68DA add 250 / 25 / 250 / 500 / 1000.  These match the port's
 * scorekill, scoreemerald, scoreoctave, scoregold, and scorebonus wrappers.
 * The port's scorekill2 (125+125) is a simultaneous-mode addition.
 */

/* ---- score storage: INT 13h, sector 7 -------------------------------- */

/*
 * There is no INT 21h (DOS file I/O) anywhere in the binary.  High scores are
 * stored in a raw 512-byte disk sector:
 *
 *   drive 0, head 0, cylinder 0x27 (39), sector 7, 1 sector
 *   buffer ES:BX = DS:0x0046
 *
 * read_sector7  CS:0x028F (file 0x018F): INT 13h AH=2
 * write_sector7 CS:0x02B8 (file 0x01B8): INT 13h AH=4 (verify), then AH=3
 * (write), retrying up to 3 times via DS:0x0247.
 *
 * The sector buffer layout is the port's .sco format:
 *
 *   byte 0: 's'
 *   ten entries: [3 initials][2 bytes][6 score digits]
 *
 * loadscores  CS:0x5DED (file 0x05CED):
 *   read_sector7();
 *   for entry i=1..10:
 *     copy 3 bytes from buffer[p]      -> DS:0x4134 + i*4
 *     p += 2
 *     copy 6 bytes from buffer[p]      -> DS:0x40D6 (highbuf)
 *     scorehigh[i] = atoi(highbuf)     -> DS:0x4108 + i*4
 *
 * savescores  CS:0x6521 (file 0x06421):
 *   buffer[0..] = 's' header / entries from DS:0x4134 and DS:0x4108;
 *   write_sector7();
 *
 * The embedded block at DS:0x4690 (112 bytes, first entry score 1200) has no
 * direct code reference.  It looks like a default sector image / legacy
 * high-score table; the port uses a file and starts from zeros if absent.
 * The sector-2 read at CS:0x0321 plus the check at CS:0x02F3 is the disk
 * signature/copy-protection check, not score I/O.
 */

/* ---- bonus mode ------------------------------------------------------- */

/*
 * createbonus CS:0x5510 (file 0x05410):
 *   bonusvisible = 1;              (DS:0x4010)
 *   draw the bonus sprite (call 0x250E with 0x124/0x12)
 *
 * initbonusmode CS:0x5528 (file 0x05428):
 *   bonusmode = 1;                 (DS:0x4012)
 *   erasebonus();
 *   ginten(1);
 *   bonustimeleft = 250 - levof10()*20;   (DS:0x400E)
 *   startbonustimeleft = 20;              (DS:0x400C)
 *   DS:0x4014 = 1;                 (port: digdat[i].msc=1)
 *
 * endbonusmode CS:0x5562 (file 0x05462):
 *   bonusmode = 0; ginten(0);
 *
 * erasebonus CS:0x5575 (file 0x05475):
 *   if (bonusvisible) { bonusvisible=0; erase sprite 0x0E; }
 *   ginten(0);
 *
 * The 250-20*level formula is original; the port matches it exactly.  The
 * flash loop lives in dodigger (port digger.c); the T1 jingle is 5.4 s at the
 * 72.8 Hz soundint rate, so a long bonus mode can outlast it.
 */
static u16 bonusvisible;        /* DS:0x4010 */
static u16 bonusmode;           /* DS:0x4012 */
static u16 startbonustimeleft;  /* DS:0x400C */
static u16 bonustimeleft;       /* DS:0x400E */
static u16 bonus_msc;           /* DS:0x4014; probable */

extern void cga_set_intensity(int on);   /* ginten(), CGA path */
extern int levof10(void);
extern void drawbonus(int x, int y);
extern void erasespr(int n);

void createbonus(void)
{
    bonusvisible = 1;
    drawbonus(0x124, 0x12);
}

void erasebonus(void)
{
    if (bonusvisible) {
        bonusvisible = 0;
        erasespr(0x0E);
    }
    cga_set_intensity(0);
}

void initbonusmode(void)
{
    bonusmode = 1;
    erasebonus();
    cga_set_intensity(1);
    bonustimeleft = (u16)(250 - levof10() * 20);
    startbonustimeleft = 20;
    bonus_msc = 1;
}

void endbonusmode(void)
{
    bonusmode = 0;
    cga_set_intensity(0);
}

/* ---- main game loop --------------------------------------------------- */

/*
 * The port's game() maps directly onto the original at CS:0x86F1
 * (file 0x085F1); the per-frame update order is:
 *
 *   penalty = 0;                  (DS:PI 0x5014; incpenalty at CS:0x8A32)
 *   dodigger();                   (CS:0x4939, verified: bonus block at 0x4912)
 *   domonsters();                 (CS:0x2C90, verified structure)
 *   dobags();                     (CS:0x3E2D, probable)
 *   if (penalty < 8) delay((8-penalty)*5); else incmont(penalty-8);
 *   testpause();                  (CS:0x8B55)
 *   checklevdone();               (CS:0x89F2)
 *
 * domonsters (CS:0x2C90) matches the port's domonsters loop exactly:
 * six monsters, stride 0x28 at DS:0x3B1C, hnt > 10-levof10(), the
 * randno(15-levof10()) double-move, and monai() at CS:0x2E75.
 * monai contains the port's randno(levof10()+5)==1 direction swap.
 *
 * Post-level path:
 *   getnmovingbags() loop with dobags/dodigger/domonsters;
 *   soundstop (CS:0x71D7), erasebonus (CS:0x5575), cleanupbags, savefield,
 *   erasemonsters; if levdone soundlevdone (CS:0x7310);
 *   countem (CS:0x5A94); increment level; initlevel (CS:0x89A4).
 *
 * Other mapped game-state functions: checklevdone CS:0x89F2, initlevel
 * CS:0x89A4, drawscreen CS:0x89CD, initchars CS:0x89E4, initscores
 * CS:0x5DE0, readdir/input CS:0x5BB9.
 */

/* ---- RNG ------------------------------------------------------------- */

/*
 * The original has one random routine, CS:0x177F, used by both the game and
 * the sound engine.  It is reconstructed in rng.c: a 256-byte permutation
 * table at DS:0x0CA7, a counter at DS:0x0DA7, and the BIOS tick byte at
 * 0x40:0x006C.  The port's randno/randnos 32-bit LCG is a replacement.
 */

/* ---- per-frame game loop -------------------------------------------- */

/*
 * CS:0x86F1 (file 0x85F1).  This function runs a whole life/level: it is
 * entered once per life, draws the level the first time, then loops until
 * the digger dies or the level is completed.  The main life loop calls it
 * again after a death; in that case levnotdrawn is already 0 and only
 * initchars() runs, re-seating the digger and monsters on the existing field.
 */

extern u8 dsseg[0x10000];

extern void hang(void);
extern void drawscreen(void);
extern void initchars(void);
extern void cleartopline(void);       /* CS:0x8A3B */
extern void strcpy_ds(int dst, int src);  /* CS:0x8D2D */
extern void strcat_ds(int dst, int src);  /* CS:0x8DF4 */
extern void writestring(int s, int x, int y, int color);
extern void drawscore(int color);     /* CS:0x5F2E */
extern void drawhighscore(void);      /* CS:0x62D0 */
extern void addscore(int n);
extern void initscores(void);
extern void drawlives(void);
extern void music(int tune);
extern void soundstopall(void);
extern void musicoff(void);
extern void readdir(void);
extern void dodigger(void);
extern void domonsters(void);
extern void dobags(void);
extern void delay(int n);
extern void incmont(int n);
extern void testpause(void);
extern void checklevdone(void);
extern void erasediggers(void);
extern int getnmovingbags(void);
extern void killfire(void);
extern void erasebonus(void);
extern void cleanupbags(void);
extern void savefield(void);
extern void erasemonsters(void);
extern void soundlevdone(void);
extern int countem(void);
extern void initlevel(void);
extern void gameover(void);           /* CS:0x610B */

static s16 g_curplayer;               /* DS:0x500C */
static s16 g_penalty;                 /* DS:0x5014 */
static s16 g_lives[2] = { 3, 0 };     /* DS:0x4FE0 / 0x4FE8 */
static s16 g_level[2] = { 1, 1 };     /* DS:0x4FE2 / 0x4FEA */
static s16 g_dead[2];                 /* DS:0x4FDE / 0x4FE6 */
static s16 g_levdone[2];              /* DS:0x4FE4 / 0x4FEC */
static s16 g_levnotdrawn;             /* DS:0x5010 */
static s16 g_banner;                  /* DS:0x5012 */
static s16 g_checkbyte;               /* DS:0x5016 */
extern volatile u8 keypressed;        /* DS:0x0658, input.c */

void game(void)
{
    int i, n;

    /* The sector-2 signature must still be valid. */
    if (g_checkbyte != 0x45)
        hang();

    if (!g_levnotdrawn) {
        initchars();
        goto frame;
    }

    g_levnotdrawn = 0;
    drawscreen();

    if (!g_banner)
        goto frame;

    g_banner = 0;
    strcpy_ds(0x4FEE, 0x5059);            /* "PLAYER " */
    strcat_ds(0x4FEE, g_curplayer == 0 ? 0x5061 : 0x5063);  /* "1"/"2" */
    cleartopline();

    for (i = 0; i < 15; i++) {
        writestring(0x4FEE, 0x6C, 0, 1);
        drawscore(1);
        delay(20);
        writestring(0x4FEE, 0x6C, 0, 2);
        drawscore(2);
        delay(20);
        writestring(0x4FEE, 0x6C, 0, 3);
        drawscore(3);
        delay(20);
    }
    drawhighscore();
    addscore(0);

frame:
    keypressed = 0;
    writestring(0x5065, 0x6C, 0, 3);      /* blank score line */
    initscores();
    drawlives();
    music(1);

    for (;;) {
        readdir();

        if (g_dead[g_curplayer] == 0 && g_levdone[g_curplayer] == 0) {
            g_penalty = 0;
            dodigger();
            domonsters();
            dobags();

            if (g_penalty < 8) {
                n = (8 - g_penalty) * 5;
                while (n > 0) {
                    delay(1);
                    n--;
                }
            } else
                incmont(g_penalty - 8);

            testpause();
            checklevdone();
            continue;
        }
        break;
    }

    /* Level over (death or completion): let the bags settle, then clean up. */
    erasediggers();
    musicoff();

    n = 20;
    for (;;) {
        if (getnmovingbags() == 0 && n == 0)
            break;
        if (n != 0)
            n--;
        g_penalty = 0;
        dobags();
        dodigger();
        domonsters();
        if (g_penalty < 8) {
            n = (8 - g_penalty) * 5;
            while (n > 0) {
                delay(1);
                n--;
            }
        }
    }

    soundstopall();
    killfire();
    erasebonus();
    cleanupbags();
    savefield();
    erasemonsters();

    if (g_levdone[g_curplayer])
        soundlevdone();

    if (countem() == 0) {
        g_level[g_curplayer]++;
        if (g_level[g_curplayer] > 1000)
            g_level[g_curplayer] = 1000;
        initlevel();
    }

    if (g_dead[g_curplayer]) {
        g_lives[g_curplayer]--;
        drawlives();
        if (g_lives[g_curplayer] == 0)
            gameover();
    }

    if (g_levdone[g_curplayer]) {
        g_level[g_curplayer]++;
        if (g_level[g_curplayer] > 1000)
            g_level[g_curplayer] = 1000;
        initlevel();
    }
}

/* ---- score state and wrappers --------------------------------------- */

extern void draw_number(u32 value, int x, int y, int digits, int color);
extern int getlives(int which);
extern void addlife(int which);
extern void drawlives(void);
extern void incpenalty(void);

static u32 score0;              /* DS:0x40C6/0x40C8 */
static u32 score1;              /* DS:0x40CA/0x40CC */
static u32 nextbonus0 = 20000;  /* DS:0x40CE/0x40D0 */
static u32 nextbonus1 = 20000;  /* DS:0x40D2/0x40D4 */
static s16 msc = 1;             /* DS:0x4014, doubles during bonus mode */

/* Views of the shared score/life state for the display layer. */
u32 score_get(int player)
{
    return player ? score1 : score0;
}

int game_lives(int player)
{
    return g_lives[player];
}

int game_dead(int player)
{
    return g_dead[player];
}

void game_set_dead(int player, int dead)
{
    g_dead[player] = (s16)dead;
}

void game_init_player(int player, int level, int lives)
{
    g_level[player] = (s16)level;
    g_lives[player] = (s16)lives;
}

/* CS:0x8B2A (file 0x8A2A) */
void setdead(int dead)
{
    g_dead[g_curplayer] = (s16)dead;
}

/* CS:0x5FA1 (file 0x5EA1) */
void addscore(int n)
{
    if (g_curplayer == 0) {
        score0 += (u32)(s16)n;
        if (score0 > 999999u)
            score0 = 0;
        draw_number(score0, 0, 0, 6, 1);
        if (score0 >= nextbonus0) {
            if (getlives(1) < 5) {
                addlife(1);
                drawlives();
            }
            nextbonus0 += 20000;
        }
    } else {
        score1 += (u32)(s16)n;
        if (score1 > 999999u)
            score1 = 0;
        draw_number(score1, score1 < 100000u ? 0xEC : 0xF8, 0, 6, 1);
        if (score1 >= nextbonus1) {
            if (getlives(2) < 5) {
                addlife(2);
                drawlives();
            }
            nextbonus1 += 20000;
        }
    }
    incpenalty();
    incpenalty();
    incpenalty();
}

/* CS:0x689F (file 0x679F) */
void scorekill(void)
{
    addscore(250);
}

/* CS:0x68AD (file 0x67AD) */
void scoreemerald(void)
{
    addscore(25);
}

/* CS:0x68BB (file 0x67BB) */
void scoreoctave(void)
{
    addscore(250);
}

/* CS:0x68C9 (file 0x67C9) */
void scoregold(void)
{
    addscore(500);
}

/* CS:0x68D7 (file 0x67D7) */
void scorebonus(void)
{
    addscore(1000);
}

/* CS:0x5598 (file 0x5498) */
void sceatm(void)
{
    addscore(msc * 200);
    msc <<= 1;
}

/* CS:0x5DE0 (file 0x5CE0) */
void initscores(void)
{
    addscore(0);
}

/* CS:0x89F2 (file 0x88F2) */
extern int monleft(void);
extern int digalive(void);

void checklevdone(void)
{
    if (countem() != 0 && monleft() != 0) {
        g_levdone[g_curplayer] = 0;
        return;
    }
    if (!digalive()) {
        g_levdone[g_curplayer] = 0;
        return;
    }
    g_levdone[g_curplayer] = 1;
}
