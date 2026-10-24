/*
 * Original Digger CGA video layer - reconstructed from digger.com.
 *
 * The game uses standard CGA mode 4 (320x200x4, 2bpp, interleaved) but enters
 * it by writing the CGA registers directly, without INT 10h mode set:
 *   3D8=0x02 (video off, graphics, colour, 320x200)
 *   CRTC R0..R13 from DS:0x0255
 *   3D9=0x20
 *   clear framebuffer
 *   3D8=0x0A (video on)
 *
 * See CGA.md for the DOSBox failure analysis.  Addresses in the comments
 * are CS:IP values; CS = file offset + 0x100.
 */

#include "symbols.h"

/* Not runnable on a modern host; this is the original CGA framebuffer. */
#define CGA_MEM ((u8 *)(uintptr_t)0xB8000U)

u8 inportb(u16 port);
void outportb(u16 port, u8 value);

static u8 cga_retrace_flag = 0xFF; /* DS:0x03B3; the data byte starts 0xFF */
static u8 cga_write_mode;      /* DS:0x03B4: 0=replace, 0xFF=XOR */
static u8 cga_color_select;    /* DS:0x03B5: cached 3D9 */
static u8 cga_pixel_color;     /* DS:0x03B6 */
static u8 cga_r2 = 0x2C;       /* DS:0x0673; initial value is 0x2C */

static const u8 cga_crtc_table[14] = {
    /* R0  R1  R2  R3  R4  R5  R6  R7  R8  R9  R10 R11 R12 R13 */
    0x38,0x28,0x2C,0x0A,0x7F,0x06,0x64,0x70,0x02,0x01,0x20,0x00,0x00,0x00
};

/* CS:0x0418 (file 0x0318) - write/read B800:0000. */
int cga_probe(void)
{
    CGA_MEM[0] = 0x55;
    return CGA_MEM[0] == 0x55;
}

/* CS:0x050B (file 0x040B) - wait for CGA vertical retrace bit (3DA bit 3). */
void cga_wait_retrace(void)
{
    if (!cga_retrace_flag)
        return;
    while (!(inportb(0x3DA) & 0x08))
        ;
}

/* CS:0x048A (file 0x038A) - video off, graphics mode. */
void cga_mode_off(void)
{
    cga_wait_retrace();
    outportb(0x3D8, 0x02);
}

/* CS:0x04A0 (file 0x03A0) - video on, graphics mode. */
void cga_mode_on(void)
{
    cga_wait_retrace();
    outportb(0x3D8, 0x0A);
}

static void cga_write_crtc(u8 index, u8 value)
{
    outportb(0x3D4, index);
    outportb(0x3D5, value);
}

/*
 * CS:0x04AE/0x04B9 (file 0x03AE/0x03B9) - clear the framebuffer to a 2-bit
 * colour.  The original replicates the colour into a 16-bit word (8 pixels)
 * and writes 0x2000 words (16KB) while the video is off.
 */
void cga_clear(int color)
{
    u16 pat = (u8)(color & 3);
    u16 i;

    pat |= (u16)(pat << 2);
    pat |= (u16)(pat << 4);
    pat |= (u16)(pat << 8);

    cga_mode_off();
    for (i = 0; i < 0x2000; i++)
        ((u16 *)CGA_MEM)[i] = pat;
    cga_mode_on();
}

/*
 * CS:0x0429 (file 0x0329) - initialise CGA mode 4 directly.
 * Note the PCjr H-sync value R2=0x2C (IBM CGA mode 4 uses 0x2D) and the
 * disabled cursor registers R10=0x20/R11=0x00.
 */
void cga_init(void)
{
    u8 i;

    cga_mode_off();
    for (i = 0; i < 14; i++)
        cga_write_crtc(i, cga_crtc_table[i]);
    cga_r2 = cga_crtc_table[2];

    outportb(0x3D9, 0x20);
    cga_color_select = 0x20;
    cga_clear(0);
    cga_mode_on();
}

/* CS:0x04E7 (file 0x03E7) - 3D9 bit 5: palette select. */
void cga_palette(int pal)
{
    cga_color_select = (u8)((cga_color_select & ~0x20) |
                            ((pal & 1) << 5));
    cga_wait_retrace();
    outportb(0x3D9, cga_color_select);
}

/* CS:0x0520 (file 0x0420) - 3D9 bits 0-3: background colour. */
void cga_set_background(int color)
{
    cga_color_select = (u8)((cga_color_select & 0xF0) |
                            (color & 0x0F));
    cga_wait_retrace();
    outportb(0x3D9, cga_color_select);
}

/* CS:0x054B/0x0556 (file 0x044B/0x0456) - 3D9 bit 4: intensity. */
void cga_set_intensity(int on)
{
    if (on)
        cga_color_select |= 0x10;
    else
        cga_color_select &= (u8)~0x10;
    cga_wait_retrace();
    outportb(0x3D9, cga_color_select);
}

/*
 * CGA 320x200 framebuffer address (from CS:0x05CF, file 0x04CF):
 *   addr = (y & ~1) * 40 + (y & 1) * 0x2000 + (x >> 2)
 * Even scanlines are in bank 0 (B800:0000), odd scanlines in bank 1
 * (B800:2000).  Each byte holds four 2-bit pixels, leftmost in bits 7-6.
 */
static u16 cga_pixel_addr(int x, int y)
{
    u16 addr = (u16)((y & ~1) * 40);
    if (y & 1)
        addr = (u16)(addr + 0x2000);
    addr = (u16)(addr + (x >> 2));
    return addr;
}

/* CS:0x05EB (file 0x04EB) - apply the cached pixel colour/write mode. */
static void cga_pixel_apply(int x, int y)
{
    u16 addr;
    u8 shift, mask, val;

    if (x < 0 || x >= 320 || y < 0 || y >= 200)
        return;

    addr = cga_pixel_addr(x, y);
    shift = (u8)((3 - (x & 3)) * 2);
    mask = (u8)(3 << shift);
    val = (u8)((cga_pixel_color & 3) << shift);

    if (cga_write_mode == 0)
        CGA_MEM[addr] = (u8)((CGA_MEM[addr] & ~mask) | val);
    else
        CGA_MEM[addr] ^= val;
}

/* CS:0x057A (file 0x047A) - replace a pixel. */
void cga_putpixel(int x, int y, int color)
{
    cga_pixel_color = (u8)(color & 3);
    cga_write_mode = 0;
    cga_pixel_apply(x, y);
}

/* CS:0x0595 (file 0x0495) - XOR a pixel. */
void cga_xor_pixel(int x, int y, int color)
{
    cga_pixel_color = (u8)(color & 3);
    cga_write_mode = 0xFF;
    cga_pixel_apply(x, y);
}

/*
 * CS:0x0628/0x0655 (file 0x0528/0x0555) - block put/XOR.  Five arguments:
 * two corner coordinates plus colour; the worker at CS:0x0682 computes the
 * width/height.  The body is not reconstructed yet.
 */

/*
 * CS:0x1128/0x1155 (file 0x1028/0x1055) - horizontal shift by rewriting
 * CRTC R2 in the keyboard handler.  The original keeps R2 in 0x2A..0x2E.
 */
void cga_shift_inc(void)
{
    cga_r2++;
    if (cga_r2 >= 0x2F)
        cga_r2 = 0x2E;
    cga_write_crtc(2, cga_r2);
}

void cga_shift_dec(void)
{
    cga_r2--;
    if (cga_r2 <= 0x29)
        cga_r2 = 0x2A;
    cga_write_crtc(2, cga_r2);
}

/*
 * CS:0x17EB (file 0x16EB) - title-screen RLE blit into the 16KB framebuffer.
 * 0xFE introduces [count][value]; count=0 means a 256-byte run in the
 * original's do/while loop.
 */
void cga_blit_rle(const u8 *src)
{
    u16 di = 0;

    while (di < 0x4000) {
        u8 b = *src++;
        if (b == 0xFE) {
            u8 count = *src++;
            u8 val = *src++;
            do {
                CGA_MEM[di++] = val;
            } while (--count && di < 0x4000);
        } else {
            CGA_MEM[di++] = b;
        }
    }
}

/* ---- game-level wrappers -------------------------------------------- */

extern u8 dsseg[0x10000];

/* CS:0x0540 (file 0x0440) */
void setretr(int on)
{
    cga_retrace_flag = (u8)on;
}

/* CS:0x04AE (file 0x03AE) */
void gclear(int color)
{
    cga_clear(color);
}

/* CS:0x04E7 (file 0x03E7) */
void gpal(int pal)
{
    cga_palette(pal);
}

/* CS:0x0520 (file 0x0420) */
void gbackground(int color)
{
    cga_set_background(color);
}

/* CS:0x054B (file 0x044B) */
void ginten(int on)
{
    cga_set_intensity(on);
}

/* CS:0x17EB (file 0x16EB) */
void blit_title(void)
{
    cga_blit_rle(&dsseg[0x0DAC]);
}

/* ---- port I/O primitives (C runtime helpers 0x8DDD/0x8DE8) ----------- */

#if defined(__i386__) || defined(__x86_64__)
u8 inportb(u16 port)
{
    u8 v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

void outportb(u16 port, u8 value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
#else
u8 inportb(u16 port)
{
    (void)port;
    return 0;
}

void outportb(u16 port, u8 value)
{
    (void)port;
    (void)value;
}
#endif
