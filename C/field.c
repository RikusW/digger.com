/*
 * Field and emerald helpers - reconstructed from the original.
 *
 * The play field is 15 x 10 16-bit cells at DS:0x4438 (row stride 0x1E):
 *   bits 0..4   horizontal sub-cell pieces
 *   bits 6..11  vertical sub-cell pieces
 *   bit 0x2000  cell is solid/empty
 * `eatfield` clears the sub-cell bits covered by the moving object; when no
 * sub-cell bits remain it also clears 0x2000.
 *
 * The emerald map is 15 x 10 bytes at DS:0x4018 (row stride 0x0F).  The
 * per-player mask is the word at DS:0x40AE.
 *
 * Functions:
 *   eatfield            CS:0x6DFE (file 0x6CFE)
 *   hitemerald          CS:0x57F1 (file 0x56F1)
 *   killemerald         CS:0x5AE0 (file 0x59E0)
 *   countem             CS:0x5A94 (file 0x5994)
 */

#include "symbols.h"

#define FIELD_W 15
#define FIELD_H 10

static u16 field[FIELD_H][FIELD_W];   /* DS:0x4438 */
static u16 field0[FIELD_H][FIELD_W];  /* DS:0x41E0, saved field, player 0 */
static u16 field1[FIELD_H][FIELD_W];  /* DS:0x430C, saved field, player 1 */
static u8 emfield[FIELD_H][FIELD_W];  /* DS:0x4018 */
static u16 emmask;                    /* DS:0x40AE */

extern void incpenalty(void);
extern void drawemerald(int x, int y);
extern void eraseemerald(int x, int y);
extern int curplayer_get(void);
extern int levplan(void);
extern char getlevch(int x, int y, int level);
extern void setretr(int on);
extern void gpal(int p);
extern void ginten(int on);
extern void drawbackg(int plan);
extern void drawbottomblob(int x, int y);
extern void drawtopblob(int x, int y);
extern void drawrightblob(int x, int y);
extern void drawleftblob(int x, int y);

void makefield(void);
void savefield(void);
void drawfield(void);
void makeemfield(void);
void drawemeralds(void);
void drawstatics(void);

/* CS:0x6DFE (file 0x6CFE) */
void eatfield(int x, int y, int dir)
{
    int h = (x - 12) / 20;
    int xr = (x - 12) % 20;
    int v = (y - 18) / 18;
    int yr = (y - 18) % 18;
    int xr4 = xr / 4;
    int yr3 = yr / 3;

    incpenalty();

    switch (dir) {
    case DIR_RIGHT:
        h++;
        switch (xr4) {
        case 0: field[v][h] &= 0xFFFE; break;
        case 1: field[v][h] &= 0xFFFD; break;
        case 2: field[v][h] &= 0xFFFB; break;
        case 3: field[v][h] &= 0xFFF7; break;
        case 4: field[v][h] &= 0xFFEF; break;
        }
        if ((field[v][h] & 0x1F) == 0)
            field[v][h] &= 0xDFFF;
        break;

    case DIR_UP:
        if (--yr3 < 0) {
            yr3 += 6;
            v--;
        }
        switch (yr3) {
        case 0: field[v][h] &= 0xFFBF; break;
        case 1: field[v][h] &= 0xFF7F; break;
        case 2: field[v][h] &= 0xFEFF; break;
        case 3: field[v][h] &= 0xFDFF; break;
        case 4: field[v][h] &= 0xFBFF; break;
        case 5: field[v][h] &= 0xF7FF; break;
        }
        if ((field[v][h] & 0xFC0) == 0)
            field[v][h] &= 0xDFFF;
        break;

    case DIR_LEFT:
        if (--xr4 < 0) {
            xr4 += 5;
            h--;
        }
        switch (xr4) {
        case 0: field[v][h] &= 0xFFFE; break;
        case 1: field[v][h] &= 0xFFFD; break;
        case 2: field[v][h] &= 0xFFFB; break;
        case 3: field[v][h] &= 0xFFF7; break;
        case 4: field[v][h] &= 0xFFEF; break;
        }
        if ((field[v][h] & 0x1F) == 0)
            field[v][h] &= 0xDFFF;
        break;

    case DIR_DOWN:
        v++;
        switch (yr3) {
        case 0: field[v][h] &= 0xFFBF; break;
        case 1: field[v][h] &= 0xFF7F; break;
        case 2: field[v][h] &= 0xFEFF; break;
        case 3: field[v][h] &= 0xFDFF; break;
        case 4: field[v][h] &= 0xFBFF; break;
        case 5: field[v][h] &= 0xF7FF; break;
        }
        if ((field[v][h] & 0xFC0) == 0)
            field[v][h] &= 0xDFFF;
        break;
    }
}

/* CS:0x57F1 (file 0x56F1) */
int hitemerald(int x, int y, int rx, int ry, int dir)
{
    static const int embox[8] = { 8, 12, 12, 9, 16, 12, 6, 9 };
    int hit = 0;
    int r;

    if (dir != DIR_RIGHT && dir != DIR_UP && dir != DIR_LEFT && dir != DIR_DOWN)
        return 0;

    if (dir == DIR_RIGHT && rx != 0)
        x++;
    if (dir == DIR_DOWN && ry != 0)
        y++;

    r = (dir == DIR_RIGHT || dir == DIR_LEFT) ? rx : ry;

    if (emfield[y][x] & emmask) {
        if (r == embox[dir]) {
            drawemerald(x * 20 + 12, y * 18 + 21);
            incpenalty();
        }
        if (r == embox[dir + 1]) {
            eraseemerald(x * 20 + 12, y * 18 + 21);
            incpenalty();
            hit = 1;
            emfield[y][x] &= (u8)~emmask;
        }
    }
    return hit;
}

/* CS:0x5AE0 (file 0x59E0) */
void killemerald(int x, int y)
{
    if (emfield[y + 1][x] & emmask) {
        emfield[y + 1][x] &= (u8)~emmask;
        eraseemerald(x * 20 + 12, (y + 1) * 18 + 21);
    }
}

/* CS:0x5A94 (file 0x5994) */
int countem(void)
{
    int x, y, n = 0;

    for (x = 0; x < FIELD_W; x++)
        for (y = 0; y < FIELD_H; y++)
            if (emfield[y][x] & emmask)
                n++;
    return n;
}

/* CS:0x6A48 (file 0x6948) */
void makefield(void)
{
    int x, y, c;

    for (x = 0; x < FIELD_W; x++)
        for (y = 0; y < FIELD_H; y++) {
            field[y][x] = 0xFFFF;
            c = getlevch(x, y, levplan());
            if (c == 'S' || c == 'V')
                field[y][x] &= 0xD03F;
            if (c == 'S' || c == 'H')
                field[y][x] &= 0xDFE0;
            if (curplayer_get() == 0)
                field0[y][x] = field[y][x];
            else
                field1[y][x] = field[y][x];
        }
}

/* CS:0x6BCA (file 0x6ACA) */
void savefield(void)
{
    int x, y;

    for (x = 0; x < FIELD_W; x++)
        for (y = 0; y < FIELD_H; y++)
            if (curplayer_get() == 0)
                field0[y][x] = field[y][x];
            else
                field1[y][x] = field[y][x];
}

/* CS:0x6B31 (file 0x6A31) */
void drawstatics(void)
{
    int x, y;

    for (x = 0; x < FIELD_W; x++)
        for (y = 0; y < FIELD_H; y++)
            field[y][x] = (curplayer_get() == 0) ? field0[y][x]
                                                  : field1[y][x];

    setretr(1);
    gpal(0);
    ginten(0);
    drawbackg(levplan());
    drawfield();
}

/* CS:0x6C3E (file 0x6B3E) */
void drawfield(void)
{
    int x, y, xp, yp;

    for (x = 0; x < FIELD_W; x++)
        for (y = 0; y < FIELD_H; y++)
            if ((field[y][x] & 0x2000) == 0) {
                xp = x * 20 + 12;
                yp = y * 18 + 18;
                if ((field[y][x] & 0xFC0) != 0xFC0) {
                    field[y][x] &= 0xD03F;
                    drawbottomblob(xp, yp - 15);
                    drawbottomblob(xp, yp - 12);
                    drawbottomblob(xp, yp - 9);
                    drawbottomblob(xp, yp - 6);
                    drawbottomblob(xp, yp - 3);
                    drawtopblob(xp, yp + 3);
                }
                if ((field[y][x] & 0x1F) != 0x1F) {
                    field[y][x] &= 0xDFE0;
                    drawrightblob(xp - 16, yp);
                    drawrightblob(xp - 12, yp);
                    drawrightblob(xp - 8, yp);
                    drawrightblob(xp - 4, yp);
                    drawleftblob(xp + 4, yp);
                }
                if (x < 14)
                    if ((field[y][x + 1] & 0xFDF) != 0xFDF)
                        drawrightblob(xp, yp);
                if (y < 9)
                    if ((field[y + 1][x] & 0xFDF) != 0xFDF)
                        drawbottomblob(xp, yp);
            }
}

/* CS:0x56E8 (file 0x55E8) */
void makeemfield(void)
{
    int x, y, c;

    emmask = (curplayer_get() == 0) ? 1 : 2;

    for (x = 0; x < FIELD_W; x++)
        for (y = 0; y < FIELD_H; y++) {
            c = getlevch(x, y, levplan());
            if (c == 'C')
                emfield[y][x] |= (u8)emmask;
            else
                emfield[y][x] &= (u8)~emmask;
        }
}

/* CS:0x5777 (file 0x5677) */
void drawemeralds(void)
{
    int x, y;

    emmask = (curplayer_get() == 0) ? 1 : 2;

    for (x = 0; x < FIELD_W; x++)
        for (y = 0; y < FIELD_H; y++)
            if (emfield[y][x] & emmask)
                drawemerald(x * 20 + 12, y * 18 + 21);
}

/* CS:0x7207 (file 0x7107) */
int getfield(int h, int v)
{
    return field[v][h];
}
