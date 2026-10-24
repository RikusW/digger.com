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
extern void draw_bonus_sprite(int x, int y);
extern void erase_sprite(int n);

void createbonus(void)
{
    bonusvisible = 1;
    draw_bonus_sprite(0x124, 0x12);
}

void erasebonus(void)
{
    if (bonusvisible) {
        bonusvisible = 0;
        erase_sprite(0x0E);
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
