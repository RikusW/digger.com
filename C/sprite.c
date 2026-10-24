/*
 * Sprite engine - reconstructed from the low-level routines.
 *
 * The original keeps 16 moving sprites plus one "misc" slot (index 16).
 * Each property is a 16-word array (indices 0..15), with a 17-entry
 * dirty/recursion pair:
 *
 *   0x03CF  sprrdrwf[17]   needs redraw
 *   0x03F1  sprrecf[17]    visited during setrdrwflgs()
 *   0x0413  sprenf[16]     on screen
 *   0x0433  sprch[16]      live data source (CS address)
 *   0x0455  sprmask[16]    live mask source (CS address)
 *   0x0477  sprbuf[16]     saved-background buffer
 *   0x0497  sprx[16]       0x04B9 spry[16]
 *   0x04DB  sprwid[16]     0x04FD sprhei[16]
 *   0x051F  sprbwid[16]    0x053F sprbhei[16]
 *   0x055F  sprnch[16]     data source
 *   0x057F  sprnmask[16]   mask source
 *   0x05A3  sprnwid[16]    0x05C3 sprnhei[16]
 *   0x05E3  sprnbwid[16]   0x0603 sprnbhei[16]
 *   0x059F/0x05A1          new x/y passed to drawspr()
 *   0x0625                 pointer to the 16-entry draw-order table
 *   0x0627                 default order: 0,1,2,...,15
 *
 * Functions:
 *   setspr       CS:0x0804   setspr2      CS:0x0754
 *   movedrawspr  CS:0x07B1   erasespr     CS:0x07D3
 *   drawspr      CS:0x07E4 -> CS:0x0997
 *   initmiscspr  CS:0x084F -> CS:0x0A32
 *   initmiscspr2 CS:0x0881 -> CS:0x0A3C (returns the redraw mask)
 *   drawmiscspr  CS:0x08AD -> CS:0x0CF3
 *   getis        CS:0x0876 -> CS:0x0CBA + CS:0x0D13
 *   clearrdrwf   CS:0x0A65   clearrecf    CS:0x0A7C
 *   setrdrwflgs  CS:0x0A90
 *   collide      CS:0x0B05   bcollide     CS:0x0B6E   bcollides CS:0x0D48
 *   putis        CS:0x0D30   putims       CS:0x0D13
 *
 * The pixel primitives are mapped, not reconstructed:
 *   ggeti(x, y, buf, w, h)             CS:0x0CD2
 *   gputi(x, y, buf, w, h)             CS:0x0BFF
 *   gputim(x, y, data, mask, w, h)     CS:0x0C6D
 *   gputich(x, y, data, mask, w, h)    CS:0x0C1E
 */

#include "symbols.h"

#define SPRITES 16
#define MISCSPR 16

static u16 sprrdrwf[SPRITES + 1];      /* DS:0x03CF */
static u16 sprrecf[SPRITES + 1];       /* DS:0x03F1 */
static u16 sprenf[SPRITES];            /* DS:0x0413 */
static u16 sprch[SPRITES];             /* DS:0x0433 */
static u16 sprmask[SPRITES];           /* DS:0x0455 */
static u16 sprbuf[SPRITES];            /* DS:0x0477 */
static u16 sprx[SPRITES];              /* DS:0x0497 */
static u16 spry[SPRITES];              /* DS:0x04B9 */
static u16 sprwid[SPRITES];            /* DS:0x04DB */
static u16 sprhei[SPRITES];            /* DS:0x04FD */
static u16 sprbwid[SPRITES];           /* DS:0x051F */
static u16 sprbhei[SPRITES];           /* DS:0x053F */

static u16 sprnch[SPRITES];            /* DS:0x055F */
static u16 sprnmask[SPRITES];          /* DS:0x057F */
static u16 sprnwid[SPRITES];           /* DS:0x05A3 */
static u16 sprnhei[SPRITES];           /* DS:0x05C3 */
static u16 sprnbwid[SPRITES];          /* DS:0x05E3 */
static u16 sprnbhei[SPRITES];          /* DS:0x0603 */

static u16 sprnewx;                    /* DS:0x059F */
static u16 sprnewy;                    /* DS:0x05A1 */

static u16 sprorder[SPRITES] = {       /* DS:0x0627 */
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};
static u16 *sprorderptr;               /* DS:0x0625 */

void initmiscspr(int x, int y, int w, int h);
int initmiscspr2(int x, int y, int w, int h);
void drawmiscspr(int x, int y, int data, int mask, int w, int h);
void getis(void);
void putis(void);
void putims(void);
void clearrdrwf(void);
void clearrecf(void);
void setrdrwflgs(int n);
int collide(int bx, int si);
int bcollide(int bx, int si);
void bcollides(int spr);
void setorder(u16 *p);
void setspr(int spr, int data, int mask, int w, int h, int bw, int bh);
void setspr2(int spr, int data, int mask, int buf, int w, int h,
             int bw, int bh);
void movedrawspr(int spr, int x, int y);
void erasespr(int spr);
int drawspr(int spr, int x, int y);

static u8 csseg[0x10000];      /* the loaded .COM image; sprite sources are CS offsets */
u8 dsseg[0x10000];             /* data segment; glyphs and strings are DS offsets */
static u8 cga_mem[0x4000];     /* B800 framebuffer */
static u8 sprmaskbyte;          /* DS:0x0623 */

static u16 pixel_addr(int x, int y)
{
    return (u16)(((y & ~1) * 40) + ((y & 1) ? 0x2000 : 0) + (x >> 2));
}

/* CS:0x0CD2 (file 0x0BD2) */
static void ggeti(int x, int y, int buf, int w, int h)
{
    int row;

    for (row = 0; row < h; row++) {
        u16 dst = pixel_addr(x, y + row);
        int i;
        for (i = 0; i < w; i++)
            csseg[(u16)(buf + row * w + i)] = cga_mem[(u16)(dst + i)];
    }
}

/* CS:0x0BFF (file 0x0AFF) */
static void gputi(int x, int y, int buf, int w, int h)
{
    int row;

    for (row = 0; row < h; row++) {
        u16 dst = pixel_addr(x, y + row);
        int i;
        for (i = 0; i < w; i++)
            cga_mem[(u16)(dst + i)] = csseg[(u16)(buf + row * w + i)];
    }
}

/* CS:0x0CF3 (file 0x0BF3) */
static void gputim(int x, int y, int data, int mask, int w, int h)
{
    int row;

    for (row = 0; row < h; row++) {
        u16 dst = pixel_addr(x, y + row);
        int i;
        for (i = 0; i < w; i++)
            cga_mem[(u16)(dst + i)] =
                (u8)((cga_mem[(u16)(dst + i)] &
                      csseg[(u16)(mask + row * w + i)]) |
                     csseg[(u16)(data + row * w + i)]);
    }
}

/* CS:0x0C1E (file 0x0B1E) - used by the title/text glyph path.  The glyph
 * data lives in the data segment; the colour byte (DS:0x0623) is the 2-bit
 * colour replicated across all four pixels of a byte. */
void gputich(int x, int y, int data, int w, int h, int color)
{
    int row;

    sprmaskbyte = (u8)((color & 3) | ((color & 3) << 2) |
                       ((color & 3) << 4) | ((color & 3) << 6));
    for (row = 0; row < h; row++) {
        u16 dst = pixel_addr(x, y + row);
        int i;
        for (i = 0; i < w; i++)
            cga_mem[(u16)(dst + i)] =
                (u8)(dsseg[(u16)(data + row * w + i)] & sprmaskbyte);
    }
}

/* Collision lists filled by bcollides(): five sprite types. */
int first[5];
int coll[SPRITES];

/* CS:0x083C (file 0x073C) */
void setorder(u16 *p)
{
    sprorderptr = p ? p : sprorder;
}

/* CS:0x0804 (file 0x0704) */
void setspr(int spr, int data, int mask, int w, int h, int bw, int bh)
{
    sprnch[spr] = (u16)data;
    sprnmask[spr] = (u16)mask;
    sprnwid[spr] = (u16)w;
    sprnhei[spr] = (u16)h;
    sprnbwid[spr] = (u16)bw;
    sprnbhei[spr] = (u16)bh;
}

/* CS:0x0754 (file 0x0654) */
void setspr2(int spr, int data, int mask, int buf, int w, int h,
             int bw, int bh)
{
    sprnch[spr] = sprch[spr] = (u16)data;
    sprnmask[spr] = sprmask[spr] = (u16)mask;
    sprbuf[spr] = (u16)buf;
    sprnwid[spr] = sprwid[spr] = (u16)w;
    sprnhei[spr] = sprhei[spr] = (u16)h;
    sprnbwid[spr] = sprbwid[spr] = (u16)bw;
    sprnbhei[spr] = sprbhei[spr] = (u16)bh;
    sprenf[spr] = 0;
}

/* CS:0x07B1 (file 0x06B1) */
void movedrawspr(int spr, int x, int y)
{
    sprx[spr] = (u16)x & 0xFFFC;
    spry[spr] = (u16)y;

    sprch[spr] = sprnch[spr];
    sprmask[spr] = sprnmask[spr];
    sprwid[spr] = sprnwid[spr];
    sprhei[spr] = sprnhei[spr];
    sprbwid[spr] = sprnbwid[spr];
    sprbhei[spr] = sprnbhei[spr];

    clearrdrwf();
    setrdrwflgs(spr);
    putis();
    ggeti(sprx[spr], spry[spr], sprbuf[spr], sprwid[spr], sprhei[spr]);
    sprenf[spr] = 1;
    sprrdrwf[spr] = 1;
    putims();
    bcollides(spr);
}

/* CS:0x07D3 (file 0x06D3) */
void erasespr(int spr)
{
    if (!sprenf[spr])
        return;
    gputi(sprx[spr], spry[spr], sprbuf[spr], sprwid[spr], sprhei[spr]);
    sprenf[spr] = 0;
    clearrdrwf();
    setrdrwflgs(spr);
    putims();
}

/* CS:0x07E4 (file 0x06E4) -> CS:0x0997 */
int drawspr(int spr, int x, int y)
{
    int ox, oy, ow, oh;

    sprnewx = (u16)x & 0xFFFC;
    sprnewy = (u16)y;

    clearrdrwf();
    setrdrwflgs(spr);

    ox = sprx[spr];
    oy = spry[spr];
    ow = sprwid[spr];
    oh = sprhei[spr];

    sprx[spr] = sprnewx;
    spry[spr] = sprnewy;
    sprwid[spr] = sprnwid[spr];
    sprhei[spr] = sprnhei[spr];
    clearrecf();
    setrdrwflgs(spr);

    sprwid[spr] = ow;
    sprhei[spr] = oh;
    spry[spr] = oy;
    sprx[spr] = ox;
    sprrdrwf[spr] = 1;

    putis();

    sprx[spr] = sprnewx;
    spry[spr] = sprnewy;
    sprch[spr] = sprnch[spr];
    sprmask[spr] = sprnmask[spr];
    sprwid[spr] = sprnwid[spr];
    sprhei[spr] = sprnhei[spr];
    sprbwid[spr] = sprnbwid[spr];
    sprbhei[spr] = sprnbhei[spr];

    ggeti(sprx[spr], spry[spr], sprbuf[spr], sprwid[spr], sprhei[spr]);
    putims();
    bcollides(spr);
    return 0;
}

/* CS:0x084F (file 0x074F) -> CS:0x0A32 */
void initmiscspr(int x, int y, int w, int h)
{
    sprx[MISCSPR] = (u16)x;
    spry[MISCSPR] = (u16)y;
    sprwid[MISCSPR] = (u16)w;
    sprhei[MISCSPR] = (u16)h;
    clearrdrwf();
    setrdrwflgs(MISCSPR);
    putis();
}

/* CS:0x0881 (file 0x0781) -> CS:0x0A3C */
int initmiscspr2(int x, int y, int w, int h)
{
    int i, mask = 0;

    sprx[MISCSPR] = (u16)x;
    spry[MISCSPR] = (u16)y;
    sprwid[MISCSPR] = (u16)(w >> 2) + 1;
    sprhei[MISCSPR] = (u16)h;
    clearrdrwf();
    setrdrwflgs(MISCSPR);
    putis();

    for (i = 0; i < SPRITES; i++)
        if (sprrdrwf[i])
            mask |= 1 << i;
    return mask;
}

/* CS:0x08AD (file 0x07AD) -> CS:0x0CF3 */
void drawmiscspr(int x, int y, int data, int mask, int w, int h)
{
    sprx[MISCSPR] = (u16)x & 0xFFFC;
    spry[MISCSPR] = (u16)y;
    sprch[MISCSPR] = (u16)data;
    sprnmask[MISCSPR] = (u16)mask;
    sprwid[MISCSPR] = (u16)w;
    sprhei[MISCSPR] = (u16)h;
    gputim(sprx[MISCSPR], spry[MISCSPR], data, mask, w, h);
}

/* CS:0x0876 (file 0x0776) */
void getis(void)
{
    int i;

    for (i = 0; i < SPRITES; i++)
        if (sprrdrwf[i])
            ggeti(sprx[i], spry[i], sprbuf[i], sprwid[i], sprhei[i]);
    putims();
}

/* CS:0x0A65 (file 0x0965) */
void clearrdrwf(void)
{
    int i;

    clearrecf();
    for (i = 0; i <= SPRITES; i++)
        sprrdrwf[i] = 0;
}

/* CS:0x0A7C (file 0x097C) */
void clearrecf(void)
{
    int i;

    for (i = 0; i <= SPRITES; i++)
        sprrecf[i] = 0;
}

/* CS:0x0A90 (file 0x0990) */
void setrdrwflgs(int n)
{
    int i;

    if (sprrecf[n])
        return;
    sprrecf[n] = 1;

    for (i = 0; i < SPRITES; i++)
        if (sprenf[i] && i != n) {
            /* The original retries the overlap test with the candidate
             * shifted by +320/-2 and -640/+4 to catch the screen edges. */
            if (collide(i, n)) {
                sprrdrwf[i] = 1;
                setrdrwflgs(i);
            } else {
                sprx[i] += 320;
                spry[i] -= 2;
                if (collide(i, n)) {
                    sprrdrwf[i] = 1;
                    setrdrwflgs(i);
                }
                sprx[i] -= 640;
                spry[i] += 4;
                if (collide(i, n)) {
                    sprrdrwf[i] = 1;
                    setrdrwflgs(i);
                }
                sprx[i] += 320;
                spry[i] -= 2;
            }
        }
}

/* CS:0x0B05 (file 0x0A05) */
int collide(int bx, int si)
{
    if (sprx[bx] >= sprx[si]) {
        if (sprx[bx] > (sprwid[si] << 2) + sprx[si] - 1)
            return 0;
    } else {
        if (sprx[si] > (sprwid[bx] << 2) + sprx[bx] - 1)
            return 0;
    }
    if (spry[bx] >= spry[si])
        return spry[bx] <= sprhei[si] + spry[si] - 1;
    return spry[si] <= sprhei[bx] + spry[bx] - 1;
}

/* CS:0x0B6E (file 0x0A6E) */
int bcollide(int bx, int si)
{
    if (sprx[bx] >= sprx[si]) {
        if (sprx[bx] + sprbwid[bx] >
            (sprwid[si] << 2) + sprx[si] - sprbwid[si] - 1)
            return 0;
    } else {
        if (sprx[si] + sprbwid[si] >
            (sprwid[bx] << 2) + sprx[bx] - sprbwid[bx] - 1)
            return 0;
    }
    if (spry[bx] >= spry[si])
        return spry[bx] + sprbhei[bx] <=
               sprhei[si] + spry[si] - sprbhei[si] - 1;
    return spry[si] + sprbhei[si] <=
           sprhei[bx] + spry[bx] - sprbhei[bx] - 1;
}

/* CS:0x0D30 (file 0x0C30) */
void putis(void)
{
    int i;

    for (i = 0; i < SPRITES; i++)
        if (sprrdrwf[i])
            gputi(sprx[i], spry[i], sprbuf[i], sprwid[i], sprhei[i]);
}

/* CS:0x0D13 (file 0x0C13) */
void putims(void)
{
    int i, spr;

    for (i = 0; i < SPRITES; i++) {
        spr = sprorderptr ? sprorderptr[i] : i;
        if (sprrdrwf[spr])
            gputim(sprx[spr], spry[spr], sprch[spr], sprmask[spr],
                   sprwid[spr], sprhei[spr]);
    }
}

/* Edge-wrapped bcollide() used by bcollides(): the candidate sprite is
 * retried shifted by one screen width in each direction. */
static int bcollide_wrap(int spc, int spr)
{
    if (bcollide(spc, spr))
        return 1;
    sprx[spc] += 320;
    spry[spc] -= 2;
    if (bcollide(spc, spr)) {
        sprx[spc] -= 320;
        spry[spc] += 2;
        return 1;
    }
    sprx[spc] -= 640;
    spry[spc] += 4;
    if (bcollide(spc, spr)) {
        sprx[spc] += 640;
        spry[spc] -= 4;
        return 1;
    }
    sprx[spc] += 320;
    spry[spc] -= 2;
    return 0;
}

/* CS:0x0D48 (file 0x0C48) */
void bcollides(int spr)
{
    static const int firstt[5] = { 14, 1, 8, 15, 0 };
    static const int lastt[5] = { 15, 8, 14, 16, 1 };
    int type, spc, next;

    for (type = 0; type < 5; type++)
        first[type] = -1;
    for (next = 0; next < SPRITES; next++)
        coll[next] = -1;

    for (type = 0; type < 5; type++) {
        next = -1;
        for (spc = firstt[type]; spc < lastt[type]; spc++)
            if (sprenf[spc] && spc != spr) {
                if (!bcollide_wrap(spc, spr))
                    continue;
                if (next == -1)
                    first[type] = next = spc;
                else
                    coll[next = (coll[next] = spc)] = -1;
            }
    }
}
