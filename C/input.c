/*
 * Keyboard/mouse input layer - reconstructed from the original.
 *
 *   install_keyboard  CS:0x0E06 (file 0x0D06)
 *   restore_keyboard  CS:0x0E2F (file 0x0D2F)
 *   keyboard_isr      CS:0x0E48 (file 0x0D48)
 *   key dispatch      CS:0x0E74 (file 0x0D74), 21-entry table DS:0x0676
 *   recompute dir     CS:0x1061 (file 0x0F61)
 *   Ctrl+arrow shift  CS:0x1128 / 0x1155
 *   delay             CS:0x023F (file 0x013F)
 *   input_init        CS:0x5B3F (file 0x5A3F)
 *   readdir           CS:0x5BB9 (file 0x5AB9)
 *   readkey           CS:0x5C5E (file 0x5B5E)
 *   getdir            CS:0x5D71 (file 0x5C71)
 *
 * The ISR reads the scan code from port 0x60, acknowledges through port
 * 0x61, stores it in DS:0x0658 (keypressed), dispatches on the 21-entry
 * table (arrow/keypad keys, F1, F7, F9, Ctrl, Alt, Del, Esc), sends EOI to
 * port 0x20 and IRETs.
 */

#include "symbols.h"

u8 keypressed;                 /* DS:0x0658 */

static s16 mouse_active;       /* DS:0x40BA */
static s16 dir_hold;           /* DS:0x0659 */
static s16 dir_edge;           /* DS:0x065B */
static s16 dir_state;          /* DS:0x065D */
static s16 fire_hold;          /* DS:0x0661 */
static s16 fire_edge;          /* DS:0x0663 */
static s16 escape_flag;        /* DS:0x0665 */
static u16 sound_toggle;       /* DS:0x0667 */
static u16 music_toggle;       /* DS:0x0669 */
static s16 mouse_x;            /* DS:0x066B */
static s16 mouse_y;            /* DS:0x066D */
static s16 mouse_button;       /* DS:0x066F */
static s16 mouse_button2;      /* DS:0x0671 */

static u8 key_left, key_right, key_up, key_down;  /* DS:0x064B..0x064E */
static u8 key_1, key_3, key_7, key_5;             /* DS:0x064F..0x0652 */
static u8 ctrl_down, alt_down, del_down;          /* DS:0x0653..0x0655 */
static u8 f9_latch, f7_latch;                     /* DS:0x0656/0x0657 */

static s16 cur_dir;            /* DS:0x40B0 */
static s16 mouse_xmin;         /* DS:0x40B2 */
static s16 mouse_ymin;         /* DS:0x40B4 */
static s16 mouse_xmax;         /* DS:0x40B6 */
static s16 mouse_ymax;         /* DS:0x40B8 */

static u16 old_int9_off, old_int9_seg;  /* DS:0x0647/0x0649 */

extern u8 inportb(u16 port);
extern void outportb(u16 port, u8 value);
extern void setfirepflag(int v);
extern void cga_shift_inc(void);   /* CS:0x1128 */
extern void cga_shift_dec(void);   /* CS:0x1155 */

void sample_mouse(void);
void install_keyboard(void);
void restore_keyboard(void);
void input_init(void);
void readdir(void);
int readkey(void);
int getdir(void);
void delay(int n);

/* CS:0x023F (file 0x013F) */
void delay(int n)
{
    int i, j;

    for (i = 0; i < n; i++)
        for (j = 0; j < 100; j++)
            ;
}

/* CS:0x0DBB (file 0x0CBB) - read the joystick/mouse port.  Each pass takes
 * bit 0 as an X count, bit 1 as a Y count and bit 2 as the loop bit; the
 * two button bits are active low. */
void sample_mouse(void)
{
    int i, x = 0, y = 0;
    u8 al;

    mouse_button = 0;
    mouse_button2 = 0;
    al = inportb(0x201);
    if (!(al & 0x10))
        mouse_button = 0xFFFF;
    if (!(al & 0x20))
        mouse_button2 = 0xFFFF;

    outportb(0x201, 0);
    for (i = 0; i < 256; i++) {
        al = inportb(0x201);
        x += al & 1;
        y += (al >> 1) & 1;
    }
    mouse_x = (s16)x;
    mouse_y = (s16)y;
}

/* CS:0x1061 (file 0x0F61) - recompute the direction from all held keys.
 * Later tests override earlier ones, so right wins over left etc. */
static void recompute_dir(void)
{
    dir_hold = -1;
    if (key_up)    { dir_hold = DIR_UP;    dir_edge = DIR_UP; }
    if (key_down)  { dir_hold = DIR_DOWN;  dir_edge = DIR_DOWN; }
    if (key_left)  { dir_hold = DIR_LEFT;  dir_edge = DIR_LEFT; }
    if (key_right) { dir_hold = DIR_RIGHT; dir_edge = DIR_RIGHT; }
    if (key_1)     { dir_hold = 1; dir_edge = 1; }
    if (key_3)     { dir_hold = 3; dir_edge = 3; }
    if (key_7)     { dir_hold = 7; dir_edge = 7; }
    if (key_5)     { dir_hold = 5; dir_edge = 5; }
}

/* CS:0x1128/0x1155 (file 0x1028/0x1055) - Ctrl+Left/Right shifts the CRTC
 * R2 value; without Ctrl this does nothing. */
static void ctrl_arrow_shift(int left)
{
    if (ctrl_down != 0xFF)
        return;
    if (left && key_left == 0xFF)
        cga_shift_inc();
    if (!left && key_right == 0xFF)
        cga_shift_dec();
}

/* CS:0x1100 (file 0x1000) - Ctrl+Alt+Del warm reboot. */
static void ctrl_alt_del(void)
{
    if (ctrl_down == 0xFF && alt_down == 0xFF && del_down == 0xFF) {
        /* The original jumps to FFFF:0000 and resets the machine. */
        for (;;)
            ;
    }
}

/* CS:0x0E48 (file 0x0D48) */
static void keyboard_isr(void)
{
    u8 sc = inportb(0x60);

    /* Acknowledge the keyboard controller. */
    outportb(0x61, (u8)(inportb(0x61) | 0x80));
    outportb(0x61, (u8)(inportb(0x61) & 0x7F));

    keypressed = sc;

    switch (sc) {
    case 0x4B:                            /* keypad 4 / left make */
        key_left = 0xFF;
        dir_hold = dir_edge = DIR_LEFT;
        dir_state = 1;
        ctrl_arrow_shift(1);
        break;
    case 0xCB:                            /* left break */
        key_left = 0;
        if (dir_hold == DIR_LEFT)
            recompute_dir();
        if (dir_state == 1)
            dir_state = key_right ? 2 : 0;
        break;
    case 0x4D:                            /* right make */
        key_right = 0xFF;
        dir_hold = dir_edge = DIR_RIGHT;
        dir_state = 2;
        ctrl_arrow_shift(0);
        break;
    case 0xCD:                            /* right break */
        key_right = 0;
        if (dir_hold == DIR_RIGHT)
            recompute_dir();
        if (dir_state == 2)
            dir_state = key_left ? 1 : 0;
        break;
    case 0x48:                            /* up make */
        key_up = 0xFF;
        dir_hold = dir_edge = DIR_UP;
        break;
    case 0xC8:                            /* up break */
        key_up = 0;
        if (dir_hold == DIR_UP)
            recompute_dir();
        break;
    case 0x50:                            /* down make */
        key_down = 0xFF;
        dir_hold = dir_edge = DIR_DOWN;
        break;
    case 0xD0:                            /* down break */
        key_down = 0;
        if (dir_hold == DIR_DOWN)
            recompute_dir();
        break;
    case 0x3B:                            /* F1 make: fire */
        fire_edge = 0xFFFF;
        fire_hold = 0xFFFF;
        break;
    case 0xBB:                            /* F1 break */
        fire_hold = 0;
        break;
    case 0x1D:                            /* Ctrl make */
        ctrl_down = 0xFF;
        break;
    case 0x9D:                            /* Ctrl break */
        ctrl_down = 0;
        break;
    case 0x38:                            /* Alt make */
        alt_down = 0xFF;
        break;
    case 0xB8:                            /* Alt break */
        alt_down = 0;
        break;
    case 0x53:                            /* Del make */
        del_down = 0xFF;
        ctrl_alt_del();
        break;
    case 0xD3:                            /* Del break */
        del_down = 0;
        break;
    case 0x41:                            /* F7 make: toggle music */
        if (f7_latch == 0xFF) {
            music_toggle = (u16)~music_toggle;
            f7_latch = 0;
        }
        break;
    case 0xC1:                            /* F7 break */
        f7_latch = 0xFF;
        break;
    case 0x43:                            /* F9 make: toggle sound */
        if (f9_latch == 0xFF) {
            sound_toggle = (u16)~sound_toggle;
            f9_latch = 0;
        }
        break;
    case 0xC3:                            /* F9 break */
        f9_latch = 0xFF;
        break;
    case 0x01:                            /* Esc */
        escape_flag = 0xFFFF;
        break;
    }

    outportb(0x20, 0x20);                 /* EOI */
}

/* CS:0x0E06 (file 0x0D06) */
void install_keyboard(void)
{
    old_int9_off = *(u16 *)(uintptr_t)0x24;
    old_int9_seg = *(u16 *)(uintptr_t)0x26;
    *(u16 *)(uintptr_t)0x24 = (u16)(uintptr_t)keyboard_isr;
    *(u16 *)(uintptr_t)0x26 = 0;          /* replaced by the CS at run time */
}

/* CS:0x0E2F (file 0x0D2F) */
void restore_keyboard(void)
{
    *(u16 *)(uintptr_t)0x24 = old_int9_off;
    *(u16 *)(uintptr_t)0x26 = old_int9_seg;
}

/* CS:0x5B3F (file 0x5A3F) - mouse/joystick detection and reset. */
void input_init(void)
{
    int i, n;

    mouse_active = 1;
    dir_hold = -1;
    dir_edge = -1;
    setfirepflag(0);

    for (i = 0; i < 10; i++) {
        outportb(0x201, 0);
        if ((inportb(0x201) & 3) != 0)
            mouse_active = 0;
        for (n = 0; n < 0xFF; n++) {
            if ((inportb(0x201) & 3) != 0)
                break;
        }
        if (n > 0xFA)
            mouse_active = 0;
    }
}

/* CS:0x5BB9 (file 0x5AB9) */
void readdir(void)
{
    int i;

    cur_dir = dir_edge;
    if (dir_hold != -1)
        cur_dir = dir_hold;
    dir_edge = -1;

    setfirepflag((fire_hold != 0 || fire_edge != 0) ? 1 : 0);
    fire_edge = 0;

    if (mouse_active) {
        int sx = 0, sy = 0;
        for (i = 0; i < 4; i++) {
            sample_mouse();
            sx += mouse_x;
            sy += mouse_y;
        }
        mouse_x = (s16)(sx >> 2);
        mouse_y = (s16)(sy >> 2);
        setfirepflag(mouse_button != 0 ? 1 : 0);
    }
}

/* CS:0x5C5E (file 0x5B5E) - nonzero starts the game from the title. */
int readkey(void)
{
    int start = 0;
    int i;

    if (mouse_active) {
        sample_mouse();
        if (mouse_button)
            start = 1;
    }

    if (keypressed != 0 && (keypressed & 0x80) == 0 && keypressed != 1) {
        start = 1;
        mouse_active = 0;
        keypressed = 0;
    }

    if (!start)
        return 0;
    if (!mouse_active)
        return 1;

    /* 50-sample average and the cursor box used by getdir(). */
    mouse_x = 0;
    mouse_y = 0;
    for (i = 0; i < 50; i++)
        sample_mouse();
    mouse_x /= 50;
    mouse_y /= 50;

    mouse_xmin = mouse_x - 0x23;
    if (mouse_xmin < 0)
        mouse_xmin = 0;
    mouse_xmin += 0xA;
    mouse_ymin = mouse_y - 0x23;
    if (mouse_ymin < 0)
        mouse_ymin = 0;
    mouse_ymin += 0xA;
    mouse_xmax = mouse_x + 0x23;
    if (mouse_xmax > 0xFF)
        mouse_xmax = 0xFF;
    mouse_xmax -= 0xA;
    mouse_ymax = mouse_y + 0x23;
    if (mouse_ymax > 0xFF)
        mouse_ymax = 0xFF;
    mouse_ymax -= 0xA;

    if (mouse_button)
        return 1;
    return 0;
}

/* CS:0x5D71 (file 0x5C71) */
int getdir(void)
{
    int d = cur_dir;

    if (mouse_active) {
        d = -1;
        if (mouse_x < mouse_xmin)
            d = DIR_LEFT;
        if (mouse_x > mouse_xmax)
            d = DIR_RIGHT;
        if (mouse_x >= mouse_xmin && mouse_x <= mouse_xmax) {
            if (mouse_y < mouse_ymin)
                d = DIR_UP;
            if (mouse_y > mouse_ymax)
                d = DIR_DOWN;
        }
    }
    return d;
}
