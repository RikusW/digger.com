/*
 * Monster module - reconstructed from the original.
 *
 * Monster records have stride 0x28.  Confirmed field offsets:
 *   flag   0x3B1C    x      0x3B1E    y      0x3B20
 *   h      0x3B22    v      0x3B24    xr     0x3B26
 *   yr     0x3B28    dir    0x3B2A    hdir   0x3B2C
 *   t      0x3B2E    alive  0x3B30    nob    0x3B32
 *   hnt    0x3B34    death  0x3B36    bag    0x3B38
 *   dtime  0x3B3A    stime  0x3B3C
 * The original carries more state than the port's struct: alive/nob sit
 * between t and hnt, and death/bag/dtime/stime follow hnt.  The last three
 * words of each record (0x3B3E..0x3B43) are not referenced by the code.
 *
 * Functions:
 *   initmonsters   CS:0x2BC1  (file 0x2AC1)
 *   erasemonsters  CS:0x2C58  (file 0x2B58)
 *   domonsters     CS:0x2C90  (file 0x2B90)
 *   createmonster  CS:0x2DD3  (file 0x2CD3)
 *   monai          CS:0x2E75  (file 0x2D75)
 *   mondie         CS:0x3682  (file 0x3582)
 *   nmononscr      CS:0x3ABB  (file 0x39BB)
 *   incmont        CS:0x3AEF  (file 0x39EF)
 *   mongold        CS:0x3B26  (file 0x3A26)
 *   monster_collisions CS:0x3795 (file 0x3695)
 *   collision_penalties CS:0x3767 (file 0x3667)
 *   fieldclear     CS:0x3815  (file 0x3715)
 *
 * Called helpers that live in other modules:
 *   movemonster/drawmon   CS:0x1B84   drawmondie      CS:0x1DDB
 *   drawrightblob         CS:0x21CC   drawleftblob    CS:0x2214
 *   drawtopblob           CS:0x225C   drawbottomblob  CS:0x22A2
 *   eatfield              CS:0x6DFE   hitemerald      CS:0x57F1
 *   bagmask               CS:0x4774   pushbags        CS:0x466C
 *   pushudbags            CS:0x46B2   removebags      CS:0x47B5
 *   bagy                  CS:0x4730   getbagdir       CS:0x4749
 *   killmon               CS:0x39A9   killdigger      CS:0x5668
 *   sceatm                CS:0x5598   soundeatm       CS:0x7891
 *   scorekill             CS:0x689F   scoreoctave?    CS:0x68AD
 *   diggerx               CS:0x5650   diggery         CS:0x5658
 *   digalive              CS:0x5660   reversedir      CS:0x55BD
 *   bonusmode getter      CS:0x55B5   rand            CS:0x177F
 */

#include <stdlib.h>
#include "symbols.h"

typedef struct {
    s16 flag, x, y, h, v, xr, yr, dir, hdir, t, alive, nob, hnt,
        death, bag, dtime, stime;
    s16 pad[3];                /* stride is 0x28 words of 2 bytes */
} monrec;

static monrec mondat[MONSTERS];

/* Monster scheduling globals. */
static s16 nextmonster;        /* DS:0x3C0E */
static s16 totalmonsters;      /* DS:0x3C10 */
static s16 maxmononscr;        /* DS:0x3C12 */
static s16 nextmontime;        /* DS:0x3C14 */
static s16 mongaptime;         /* DS:0x3C16 */
static s16 unbonusflag;        /* DS:0x3C18 */
static s16 mongotgold;         /* DS:0x3C0C, set by mongold() */

extern int levof10(void);
extern int rand_no(int n);
extern int diggerx(void);
extern int diggery(void);
extern int digalive(void);
extern int reversedir(int dir);
extern int bonusmode_get(void);
extern void createbonus(void);

extern int fieldclear(int dir, int h, int v);
extern int getfield(int h, int v);
extern int drawmon(int mon, int nob, int hdir, int x, int y);
extern int drawmondie(int mon, int nob, int hdir, int x, int y);
extern void drawrightblob(int x, int y);
extern void drawleftblob(int x, int y);
extern void drawtopblob(int x, int y);
extern void drawbottomblob(int x, int y);
extern void eatfield(int x, int y, int dir);
extern void hitemerald(int x, int y, int rx, int ry, int dir);
extern int bagmask(void);
extern int pushbags(int dir, int mask);
extern int pushudbags(int mask);
extern void removebags(int mask);
extern int bagy(int bag);
extern int getbagdir(int bag);
extern void killmon(int mon);
extern void killdigger(int death, int bag);
extern void sceatm(void);
extern void soundeatm(void);
extern void scorekill(void);
extern void incpenalty(void);
extern void erasespr(int spr);
extern void movedrawspr(int spr, int x, int y);

void initmonsters(void);
void erasemonsters(void);
void domonsters(void);
void createmonster(void);
void monai(int mon);
void mondie(int mon);
int nmononscr(void);
int monleft(void);
void incmont(int n);
void mongold(void);
int killmonsters(int mask);
void squashmonster(int mon, int death, int bag);
void squashmonsters(int bag, int mask);
void checkmonscared(int h);

static void monster_collisions(int mon, int mask);
static void collision_penalties(int mask);

/* CS:0x2BC1 (file 0x2AC1) */
void initmonsters(void)
{
    int i;

    for (i = 0; i < MONSTERS; i++)
        mondat[i].flag = 0;

    nextmonster = 0;
    mongaptime = 45 - (levof10() << 1);
    totalmonsters = levof10() + 5;

    switch (levof10()) {
    case 1:
        maxmononscr = 3;
        break;
    case 2: case 3: case 4: case 5: case 6: case 7:
        maxmononscr = 4;
        break;
    case 8: case 9: case 10:
        maxmononscr = 5;
        break;
    }

    nextmontime = 10;
    unbonusflag = 1;
}

/* CS:0x2C58 (file 0x2B58) */
void erasemonsters(void)
{
    int i;

    for (i = 0; i < MONSTERS; i++)
        if (mondat[i].flag)
            erasespr(i + 8);   /* FIRSTMONSTER = sprite 8 in the original */
}

/* CS:0x3ABB (file 0x39BB) */
int nmononscr(void)
{
    int i, n = 0;

    for (i = 0; i < MONSTERS; i++)
        if (mondat[i].flag)
            n++;
    return n;
}

/* CS:0x3AEF (file 0x39EF) */
void incmont(int n)
{
    int m;

    if (n > MONSTERS)
        n = MONSTERS;
    for (m = 1; m < n; m++)
        mondat[m].t++;
}

/* CS:0x3B26 (file 0x3A26) */
void mongold(void)
{
    mongotgold = 1;
}

/* CS:0x3AA5 (file 0x39A5) */
int monleft(void)
{
    return nmononscr() + totalmonsters - nextmonster;
}

/* CS:0x3A40 (file 0x3940) */
int killmonsters(int mask)
{
    int m, bit, n = 0;

    for (m = 0, bit = 0x0100; m < MONSTERS; m++, bit <<= 1)
        if (mask & bit) {
            killmon(m);
            n++;
        }
    return n;
}

/* CS:0x3A7C (file 0x397C) */
void squashmonster(int mon, int death, int bag)
{
    mondat[mon].alive = 0;
    mondat[mon].death = death;
    mondat[mon].bag = bag;
}

/* CS:0x39E7 (file 0x38E7) */
void squashmonsters(int bag, int mask)
{
    int m, bit;

    for (m = 0, bit = 0x0100; m < MONSTERS; m++, bit <<= 1)
        if (mask & bit)
            if (mondat[m].y >= bagy(bag))
                squashmonster(m, 1, bag);
}

/* CS:0x3962 (file 0x3862) */
void checkmonscared(int h)
{
    int m;

    for (m = 0; m < MONSTERS; m++)
        if (mondat[m].h == h && mondat[m].dir == DIR_UP)
            mondat[m].dir = DIR_DOWN;
}

/* CS:0x2C90 (file 0x2B90) */
void domonsters(void)
{
    int i;

    if (nextmontime > 0)
        nextmontime--;
    else {
        if (nextmonster < totalmonsters && nmononscr() < maxmononscr &&
            digalive() && !bonusmode_get())
            createmonster();
        if (unbonusflag && nextmonster == totalmonsters && nextmontime == 0)
            if (digalive()) {
                unbonusflag = 0;
                createbonus();
            }
    }

    for (i = 0; i < MONSTERS; i++)
        if (mondat[i].flag) {
            if (mondat[i].hnt > 10 - levof10()) {
                if (mondat[i].nob) {
                    mondat[i].hnt = 0;
                    mondat[i].nob = 0;
                }
            }
            if (mondat[i].alive)
                if (mondat[i].t == 0) {
                    monai(i);
                    if (mondat[i].alive && rand_no(15 - levof10()) == 0)
                        if (mondat[i].nob)
                            monai(i);
                } else
                    mondat[i].t--;
            else
                mondie(i);
        }
}

/* CS:0x2DD3 (file 0x2CD3) */
void createmonster(void)
{
    int i;

    for (i = 0; i < MONSTERS; i++)
        if (!mondat[i].flag) {
            mondat[i].flag = 1;
            mondat[i].alive = 1;
            mondat[i].t = 0;
            mondat[i].nob = 1;
            mondat[i].hnt = 0;
            mondat[i].h = 14;
            mondat[i].v = 0;
            mondat[i].x = 292;
            mondat[i].y = 18;
            mondat[i].xr = 0;
            mondat[i].yr = 0;
            mondat[i].dir = DIR_LEFT;
            mondat[i].hdir = DIR_LEFT;
            nextmonster++;
            nextmontime = mongaptime;
            mondat[i].stime = 5;
            movedrawspr(i + 8, mondat[i].x, mondat[i].y);
            break;
        }
}

/* CS:0x2E75 (file 0x2D75) - the whole per-monster update.  When xr/yr are
 * nonzero the monster is between cells, so the direction-selection section
 * is skipped and execution resumes at the wall check. */
void monai(int mon)
{
    int monox = mondat[mon].x;
    int monoy = mondat[mon].y;
    int mdirp1, mdirp2, mdirp3, mdirp4, dir, t, coll, push;

    if (mondat[mon].xr == 0 && mondat[mon].yr == 0) {

        /* Turn a hobbin back into a nobbin if its time is up. */
        if (mondat[mon].hnt > 30 + (levof10() << 1))
            if (!mondat[mon].nob) {
                mondat[mon].hnt = 0;
                mondat[mon].nob = 1;
            }

        /* Chase the (single) digger. */
        if (abs(diggery() - monoy) > abs(diggerx() - monox)) {
            if (diggery() < monoy) { mdirp1 = DIR_UP;    mdirp4 = DIR_DOWN; }
                              else { mdirp1 = DIR_DOWN;  mdirp4 = DIR_UP; }
            if (diggerx() < monox) { mdirp2 = DIR_LEFT;  mdirp3 = DIR_RIGHT; }
                              else { mdirp2 = DIR_RIGHT; mdirp3 = DIR_LEFT; }
        } else {
            if (diggerx() < monox) { mdirp1 = DIR_LEFT;  mdirp4 = DIR_RIGHT; }
                              else { mdirp1 = DIR_RIGHT; mdirp4 = DIR_LEFT; }
            if (diggery() < monoy) { mdirp2 = DIR_UP;    mdirp3 = DIR_DOWN; }
                              else { mdirp2 = DIR_DOWN;  mdirp3 = DIR_UP; }
        }

        /* In bonus mode, run away from the digger. */
        if (bonusmode_get()) {
            t = mdirp1; mdirp1 = mdirp4; mdirp4 = t;
            t = mdirp2; mdirp2 = mdirp3; mdirp3 = t;
        }

        /* Don't reverse direction unless there is no alternative. */
        dir = reversedir(mondat[mon].dir);
        if (dir == mdirp1) { mdirp1 = mdirp2; mdirp2 = mdirp3;
                             mdirp3 = mdirp4; mdirp4 = dir; }
        if (dir == mdirp2) { mdirp2 = mdirp3; mdirp3 = mdirp4;
                             mdirp4 = dir; }
        if (dir == mdirp3) { mdirp3 = mdirp4; mdirp4 = dir; }

        /* Random element on levels 1-5. */
        if (rand_no(levof10() + 5) == 1)
            if (levof10() < 6) {
                t = mdirp1; mdirp1 = mdirp3; mdirp3 = t;
            }

        /* Pick the first clear direction in priority order. */
        if (fieldclear(mdirp1, mondat[mon].h, mondat[mon].v))
            dir = mdirp1;
        else if (fieldclear(mdirp2, mondat[mon].h, mondat[mon].v))
            dir = mdirp2;
        else if (fieldclear(mdirp3, mondat[mon].h, mondat[mon].v))
            dir = mdirp3;
        else if (fieldclear(mdirp4, mondat[mon].h, mondat[mon].v))
            dir = mdirp4;

        /* Hobbins ignore the field: they go where they want. */
        if (!mondat[mon].nob)
            dir = mdirp1;

        /* A direction change costs a time penalty. */
        if (mondat[mon].dir != dir)
            mondat[mon].t++;

        mondat[mon].dir = dir;
    }

    /* Stop at the edge of the screen. */
    if ((mondat[mon].x == 292 && mondat[mon].dir == DIR_RIGHT) ||
        (mondat[mon].x == 12 && mondat[mon].dir == DIR_LEFT) ||
        (mondat[mon].y == 180 && mondat[mon].dir == DIR_DOWN) ||
        (mondat[mon].y == 18 && mondat[mon].dir == DIR_UP))
        mondat[mon].dir = DIR_NONE;

    /* Hobbins dig in a straight horizontal line. */
    if (mondat[mon].dir == DIR_LEFT || mondat[mon].dir == DIR_RIGHT)
        mondat[mon].hdir = mondat[mon].dir;

    if (!mondat[mon].nob)
        eatfield(mondat[mon].x, mondat[mon].y, mondat[mon].dir);

    /* Draw the new tunnel and move. */
    switch (mondat[mon].dir) {
    case DIR_RIGHT:
        if (!mondat[mon].nob)
            drawrightblob(mondat[mon].x, mondat[mon].y);
        mondat[mon].x += 4;
        break;
    case DIR_UP:
        if (!mondat[mon].nob)
            drawtopblob(mondat[mon].x, mondat[mon].y);
        mondat[mon].y -= 3;
        break;
    case DIR_LEFT:
        if (!mondat[mon].nob)
            drawleftblob(mondat[mon].x, mondat[mon].y);
        mondat[mon].x -= 4;
        break;
    case DIR_DOWN:
        if (!mondat[mon].nob)
            drawbottomblob(mondat[mon].x, mondat[mon].y);
        mondat[mon].y += 3;
        break;
    }

    /* Hobbins eat emeralds. */
    if (!mondat[mon].nob)
        hitemerald((mondat[mon].x - 12) / 20, (mondat[mon].y - 18) / 18,
                   (mondat[mon].x - 12) % 20, (mondat[mon].y - 18) % 18,
                   mondat[mon].dir);

    /* If the digger is gone, don't move. */
    if (!digalive()) {
        mondat[mon].x = monox;
        mondat[mon].y = monoy;
    }

    /* If the monster has just started, don't move yet. */
    if (mondat[mon].stime != 0) {
        mondat[mon].stime--;
        mondat[mon].x = monox;
        mondat[mon].y = monoy;
    }

    /* Increase the hobbin counter. */
    if (!mondat[mon].nob && mondat[mon].hnt < 100)
        mondat[mon].hnt++;

    /* Draw the monster and collect sprite collisions. */
    push = 1;
    coll = drawmon(mon, mondat[mon].nob, mondat[mon].hdir,
                   mondat[mon].x, mondat[mon].y);
    incpenalty();

    /* Collision with another monster. */
    if (coll & COLL_MON_BIT) {
        mondat[mon].t++;
        monster_collisions(mon, coll & COLL_MON_BIT);
        collision_penalties(coll & COLL_MON_BIT);
    }

    /* Collision with a bag. */
    if ((coll & COLL_BAG_BIT) & bagmask()) {
        mondat[mon].t++;
        mongotgold = 0;
        if (mondat[mon].dir == DIR_RIGHT || mondat[mon].dir == DIR_LEFT) {
            push = pushbags(mondat[mon].dir, coll);
            mondat[mon].t++;
        } else if (!pushudbags(coll))
            push = 0;
        if (mongotgold)
            mondat[mon].t = 0;
        if (!mondat[mon].nob && mondat[mon].hnt > 1)
            removebags(coll);
    }

    /* Increase the hobbin cross counter. */
    if (mondat[mon].nob && (coll & COLL_MON_BIT) && digalive())
        mondat[mon].hnt++;

    /* If the bag pushed the monster back, undo the move. */
    if (!push) {
        mondat[mon].x = monox;
        mondat[mon].y = monoy;
        drawmon(mon, mondat[mon].nob, mondat[mon].hdir,
                mondat[mon].x, mondat[mon].y);
        incpenalty();
        if (mondat[mon].nob)
            mondat[mon].hnt++;
        if ((mondat[mon].dir == DIR_UP || mondat[mon].dir == DIR_DOWN) &&
            mondat[mon].nob)
            mondat[mon].dir = reversedir(mondat[mon].dir);
    }

    /* Collision with the digger. */
    if ((coll & COLL_DIGGER) && digalive()) {
        if (bonusmode_get()) {
            killmon(mon);
            sceatm();
            soundeatm();
        } else
            killdigger(3, 0);
    }

    /* Update the field coordinates and remainders. */
    mondat[mon].h = (mondat[mon].x - 12) / 20;
    mondat[mon].v = (mondat[mon].y - 18) / 18;
    mondat[mon].xr = (mondat[mon].x - 12) % 20;
    mondat[mon].yr = (mondat[mon].y - 18) % 18;
}

/* CS:0x3795 (file 0x3695) - two monsters on the same square may not keep
 * moving the same way. */
static void monster_collisions(int mon, int mask)
{
    int m, bit;

    for (m = 0, bit = 0x0100; m < MONSTERS; m++, bit <<= 1)
        if (mask & bit)
            if (mondat[mon].dir == mondat[m].dir &&
                mondat[m].stime == 0 && mondat[mon].stime == 0)
                mondat[m].dir = reversedir(mondat[m].dir);
}

/* CS:0x3767 (file 0x3667) - a penalty per colliding monster. */
static void collision_penalties(int mask)
{
    int m, bit;

    for (m = 0, bit = 0x0100; m < MONSTERS; m++, bit <<= 1)
        if (mask & bit)
            incpenalty();
}

/* CS:0x3682 (file 0x3582) */
void mondie(int mon)
{
    switch (mondat[mon].death) {
    case 1:
        if (bagy(mondat[mon].bag) + 6 > mondat[mon].y)
            mondat[mon].y = bagy(mondat[mon].bag);
        drawmondie(mon, mondat[mon].nob, mondat[mon].hdir,
                   mondat[mon].x, mondat[mon].y);
        incpenalty();
        if (getbagdir(mondat[mon].bag) == -1) {
            mondat[mon].dtime = 1;
            mondat[mon].death = 4;
        }
        break;
    case 4:
        if (mondat[mon].dtime != 0)
            mondat[mon].dtime--;
        else {
            killmon(mon);
            scorekill();
        }
    }
}

/* CS:0x3815 (file 0x3715) */
int fieldclear(int dir, int x, int y)
{
    switch (dir) {
    case DIR_RIGHT:
        if (x < 14)
            if ((getfield(x + 1, y) & 0x2000) == 0)
                if ((getfield(x + 1, y) & 1) == 0 ||
                    (getfield(x, y) & 0x10) == 0)
                    return 1;
        break;
    case DIR_UP:
        if (y > 0)
            if ((getfield(x, y - 1) & 0x2000) == 0)
                if ((getfield(x, y - 1) & 0x800) == 0 ||
                    (getfield(x, y) & 0x40) == 0)
                    return 1;
        break;
    case DIR_LEFT:
        if (x > 0)
            if ((getfield(x - 1, y) & 0x2000) == 0)
                if ((getfield(x - 1, y) & 0x10) == 0 ||
                    (getfield(x, y) & 1) == 0)
                    return 1;
        break;
    case DIR_DOWN:
        if (y < 9)
            if ((getfield(x, y + 1) & 0x2000) == 0)
                if ((getfield(x, y + 1) & 0x40) == 0 ||
                    (getfield(x, y) & 0x800) == 0)
                    return 1;
    }
    return 0;
}
