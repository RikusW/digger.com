/*
 * High-score table and storage - reconstructed from the original.
 *
 *   strcpy_ds     CS:0x8D2D (file 0x8C2D)
 *   strcat_ds     CS:0x8DF4 (file 0x8CF4)
 *   format_score  CS:0x68E5 (file 0x67E5)
 *   atoi_ds       CS:0x69C3 (file 0x68C3)
 *   read_sector7  CS:0x028F (file 0x018F)
 *   write_sector7 CS:0x02B8 (file 0x01B8)
 *   loadscores    CS:0x5DED (file 0x5CED)
 *   savescores    CS:0x6521 (file 0x6421)
 *   enter_initials CS:0x65B5 (file 0x64B5)
 *   insert_score  CS:0x6807 (file 0x6707)
 *
 * Score entries live at DS:0x4108 (32-bit, indices 1..10; index 0 unused)
 * and the initials at DS:0x4134 (4 bytes each).  The disk sector buffer at
 * DS:0x0046 holds 's' followed by ten entries of
 * [3 initials][2 bytes][6 score digits].
 */

#include "symbols.h"

extern u8 dsseg[0x10000];

extern void writestring(int s, int x, int y, int color);
extern void setsoundt2(void);
extern void timer2(int v);
extern void stopint8(void);
extern volatile u8 keypressed;
extern void delay(int n);

void loadscores(void);
void savescores(void);
void enter_initials(void);
void insert_score(u32 score);
u32 scorehigh_get(int i);
void format_score(int buf, u32 value, int pad);
int atoi_ds(int src);

/* CS:0x8D2D (file 0x8C2D) */
void strcpy_ds(int dst, int src)
{
    while ((dsseg[dst++] = dsseg[src++]) != 0)
        ;
}

/* CS:0x8DF4 (file 0x8CF4) */
void strcat_ds(int dst, int src)
{
    while (dsseg[dst] != 0)
        dst++;
    strcpy_ds(dst, src);
}

/* DS:0x4108 + 4*i */
u32 scorehigh_get(int i)
{
    int o = 0x4108 + i * 4;
    return (u32)dsseg[o] | ((u32)dsseg[o + 1] << 8) |
           ((u32)dsseg[o + 2] << 16) | ((u32)dsseg[o + 3] << 24);
}

static void scorehigh_set(int i, u32 v)
{
    int o = 0x4108 + i * 4;
    dsseg[o] = (u8)v;
    dsseg[o + 1] = (u8)(v >> 8);
    dsseg[o + 2] = (u8)(v >> 16);
    dsseg[o + 3] = (u8)(v >> 24);
}

/* CS:0x69C3 (file 0x68C3) */
int atoi_ds(int src)
{
    int v = 0;

    while (dsseg[src] >= '0' && dsseg[src] <= '9')
        v = v * 10 + (dsseg[src++] - '0');
    return v;
}

/* CS:0x68E5 (file 0x67E5) - write the decimal value to a DS string,
 * suppressing leading zeros; a zero value becomes "0" when pad is set. */
void format_score(int buf, u32 value, int pad)
{
    char tmp[12];
    int n = 0, i;

    if (value == 0) {
        if (pad) {
            dsseg[buf] = '0';
            dsseg[buf + 1] = 0;
        } else
            dsseg[buf] = 0;
        return;
    }
    while (value != 0) {
        tmp[n++] = (char)('0' + value % 10);
        value /= 10;
    }
    for (i = 0; i < n; i++)
        dsseg[buf + i] = (u8)tmp[n - 1 - i];
    dsseg[buf + n] = 0;
}

/* CS:0x028F (file 0x018F): INT 13h AH=2, drive 0, head 0, cylinder 0x27,
 * sector 7, one sector, buffer DS:0x0046. */
void read_sector7(void)
{
    /* bios_disk(0x02, 0, 0, 0x27, 7, 1, &dsseg[0x46]); */
}

/* CS:0x02B8 (file 0x01B8): INT 13h AH=4 verify then AH=3 write, three
 * retries tracked at DS:0x0247. */
void write_sector7(void)
{
    /* bios_disk(0x04, ...); bios_disk(0x03, ...); */
}

/* CS:0x5DED (file 0x5CED) */
void loadscores(void)
{
    int i, j, p = 1;

    read_sector7();

    for (i = 1; i <= 10; i++) {
        for (j = 0; j < 3; j++)
            dsseg[0x4134 + i * 4 + j] = dsseg[0x46 + p++];
        p += 2;
        for (j = 0; j < 6; j++)
            dsseg[0x40D6 + j] = dsseg[0x46 + p++];
        scorehigh_set(i, (u32)atoi_ds(0x40D6));
    }

    if (dsseg[0x46] != 's') {
        for (i = 0; i <= 10; i++) {
            scorehigh_set(i, 0);
            strcpy_ds(0x4134 + i * 4, 0x4162);
        }
    }
}

/* CS:0x6521 (file 0x6421) */
void savescores(void)
{
    int i, p = 1;

    strcpy_ds(0x46, 0x41C1);             /* "s" header */

    for (i = 1; i <= 10; i++) {
        strcpy_ds(0x40E0, 0x41BD);
        strcat_ds(0x40E0, 0x4134 + i * 4);
        strcat_ds(0x40E0, 0x41BE);
        format_score(0x40D6, scorehigh_get(i), 1);
        strcat_ds(0x40E0, 0x40D6);
        /* Copy initials, two padding bytes and six digits into the sector. */
        dsseg[0x46 + p++] = dsseg[0x4134 + i * 4];
        dsseg[0x46 + p++] = dsseg[0x4134 + i * 4 + 1];
        dsseg[0x46 + p++] = dsseg[0x4134 + i * 4 + 2];
        p += 2;
        for (int j = 0; j < 6; j++)
            dsseg[0x46 + p++] = dsseg[0x40D6 + j];
    }

    write_sector7();
}

/* CS:0x65B5 (file 0x64B5) - prompt for three initials. */
void enter_initials(void)
{
    static const char prompt1[] = "ENTER YOUR INITIALS";
    int i;
    u8 ch;

    (void)prompt1;
    writestring(0x41C3, 0x64, 0x46, 3);
    writestring(0x41CE, 0x64, 0x5A, 3);
    writestring(0x41D8, 0x80, 0x82, 3);
    strcpy_ds(0x4134, 0x4162);

    setsoundt2();
    timer2(40);
    stopint8();

    for (i = 0; i < 3; i++) {
        do {
            ch = keypressed;
            delay(1);
        } while (ch == 0 || (ch & 0x80) != 0);
        keypressed = 0;
        dsseg[0x4134 + i] = ch;
        dsseg[0x4134 + i + 1] = 0;
        writestring(0x4134, 0x64, 0x46, 3);
    }
}

/* CS:0x6807 (file 0x6707) - insert the current score into the table. */
void insert_score(u32 score)
{
    int at = 10, i;

    /* Locate the first entry the new score beats. */
    for (i = 10; i > 1; i--)
        if (score >= scorehigh_get(i)) {
            at = i;
            break;
        }

    for (i = 10; i > at; i--) {
        scorehigh_set(i, scorehigh_get(i - 1));
        strcpy_ds(0x4134 + i * 4, 0x4134 + (i - 1) * 4);
    }
    scorehigh_set(at, score);
    strcpy_ds(0x4134 + at * 4, 0x4134);
}
