/*
 * Original Digger sound engine - reconstructed from digger.com.
 *
 * Functions marked "verified" match the disassembly and the matching C port;
 * see SYMBOLS.md and ANALYSIS.md.
 *
 * The original's globals live in its data segment; the DS offsets are given in
 * comments.  The music tables are not duplicated here: they are at the DS
 * offsets in symbols.h, dumped in digger.dat, and byte-identical to the
 * port's arrays (except for the port-added bonus-jingle bridge).
 */

#include "symbols.h"

/* ---- globals ---------------------------------------------------------- */

/* Timer/speaker state (DS offsets from symbols.h). */
static u16 spkrmode;        /* DS:0x0248 */
static u16 timerrate;       /* DS:0x024A: sett0 stores 4*t0val (or 0xFFFF) */
static u16 timercount;      /* DS:0x024C */
static u8  timerclock;      /* DS:0x024E */
static u16 pulsewidth;      /* DS:0x024F */

/* Music state. */
static u16 t0val;           /* DS:0x4570 */
static u16 t2val;           /* DS:0x456E */
static u16 wavetype;        /* DS:0x456C */
static u16 musvol;          /* DS:0x4572 */

static u16 musicp;          /* DS:0x45C8 */
static u16 tuneno;          /* DS:0x45CA */
static u16 noteduration;    /* DS:0x45CC */
static u16 notevalue;       /* DS:0x45CE */
static u16 musicplaying;    /* DS:0x45D0 */
static u16 soundpaused;     /* DS:0x45D2 */
static u16 musicmaxvol;     /* DS:0x45D4 */
static u16 musicattackrate; /* DS:0x45D6 */
static u16 musicsustainlevel; /* DS:0x45D8 */
static u16 musicdecayrate;  /* DS:0x45DA */
static u16 musicnotewidth;  /* DS:0x45DC */
static u16 musicreleaserate;/* DS:0x45DE */
static u16 musicstage;      /* DS:0x45E0 */
static u16 musicn;          /* DS:0x45E2 */

/* Sound-effect state. */
static u16 emerfreq;        /* DS:0x45A2 */
static u16 soundemeraldduration; /* DS:0x459E */
static u16 soundemeraldn;   /* DS:0x45A4 */
static u16 soundemeraldflag;/* DS:0x45A0 */

static u16 soundddien;      /* DS:0x45B0 */
static u16 soundddievalue;  /* DS:0x45B2 */
static u16 soundddieflag;   /* DS:0x45AE */

/* Sound/music enable flags. */
static u16 soundflag;       /* DS:0x0667 */
static u16 musicflag;       /* DS:0x0669 */
static u16 sndflag;         /* DS:0x4566 */
static u16 int8flag;        /* DS:0x4568 */
static u16 soundt0flag;     /* DS:0x456A */

/* Music tables (see symbols.h for addresses). */
extern const u16 ds_bonus_t1[];   /* DS:0x45E4, 86 words */
extern const u16 ds_background[]; /* DS:0x4866, 291 words */
extern const u16 ds_dirge[];      /* DS:0x4AAC, 49 words */

/* Forward declarations of routines reconstructed elsewhere / not yet. */
void soundint(void);
void musicupdate(void);
void soundemeraldupdate(void);
void soundwobbleupdate(void);
void soundddieupdate(void);
void soundbreakupdate(void);
void soundgoldupdate(void);
void soundemupdate(void);
void soundexplodeupdate(void);
void soundfireupdate(void);
void soundeatmupdate(void);
void soundfallupdate(void);
void sound1upupdate(void);
void soundbonusupdate(void);
void setsoundt2(void);
void setsoundmode(void);
void sett0(void);
void sett2val(u16 v);
void timer2(u16 v);
void soundoff(void);
void soundddieoff(void);
void soundstop(void);
void initint8(void);
void restoreint8(void);
u8 inportb(u16 port);
void outportb(u16 port, u8 value);

/* ---- timer / PC speaker ISR ------------------------------------------ */

/*
 * CS:0x0397 (file 0x0297) - INT 8 handler.
 *
 * The original saves all registers, loads DS/ES from cs:[0x337], drives the
 * PC speaker on port 0x61, then accumulates timerrate into a 16-bit counter.
 * Every overflow is one soundint tick.  sett0() stores 4*t0val in
 * DS:0x024A; with a timer-0 IRQ rate of 1,193,181/t0val this gives
 * 4*1,193,181/65,536 = 72.8 Hz.  The port emulates the same rate by storing
 * t0val and subtracting 0x4000 after each carry in newsnd.c:getsample().
 */
void timer_isr(void)
{
    u16 i;
    u8 al;

    if (spkrmode == 0) {
        /* speaker off */
    } else if (spkrmode == 1) {
        /*
         * PWM volume: turn the speaker on for `pulsewidth` CPU loop
         * iterations, then off.  This is CPU-speed dependent in the original
         * (calibrate()/getkips() scales the volume); the port approximates it
         * with pwlut[] in timer ticks.
         */
        i = pulsewidth;
        al = inportb(0x61);
        outportb(0x61, al | 0x02);
        while (i--)
            ;
        outportb(0x61, al & ~0x02);
    } else {
        /* square wave: toggle bit 1 of port 0x61 */
        outportb(0x61, inportb(0x61) ^ 0x02);
    }

    timercount += timerrate;
    if (timercount < timerrate) {  /* 16-bit carry */
        timerclock++;
        soundint();
    }

    outportb(0x20, 0x20);          /* EOI */
}

/* ---- sound interrupt -------------------------------------------------- */

/* CS:0x7227 (file 0x7127) - matches port sound.c:soundint(). */
void soundint(void)
{
    /* timerclock++ is done by timer_isr() in the original; the port does it
     * at the top of soundint(). */
    if (soundflag && !sndflag)
        sndflag = musicflag = 1;
    if (!soundflag && sndflag) {
        sndflag = 0;
        timer2(40);
        setsoundt2();
        soundoff();
    }
    if (sndflag && !soundpaused) {
        t0val = 0x7D00;
        t2val = 40;
        if (musicflag)
            musicupdate();
        soundemeraldupdate();
        soundwobbleupdate();
        soundddieupdate();
        soundbreakupdate();
        soundgoldupdate();
        soundemupdate();
        soundexplodeupdate();
        soundfireupdate();
        soundeatmupdate();
        soundfallupdate();
        sound1upupdate();
        soundbonusupdate();
        if (t0val == 0x7D00 || t2val != 40)
            setsoundt2();
        else {
            setsoundmode();
            sett0();
        }
        sett2val(t2val);
    }
}

/* ---- sound hardware control ------------------------------------------ */

/* CS:0x7FD5 (file 0x7ED5) - verified: write a 16-bit divisor to PIT timer 2. */
void timer2(u16 t2v)
{
    outportb(0x42, (u8)(t2v & 0xFF));
    outportb(0x42, (u8)(t2v >> 8));
}

/* CS:0x7DD9 (file 0x7CD9) - verified against port sett0(). */
void sett0(void)
{
    if (!sndflag)
        return;

    timer2(t2val);                    /* timer2_write(t2val) */

    /* The port clamps for wavetype 1 or 2; the original tests both cases. */
    if (t0val < 1000 && (wavetype == 1 || wavetype == 2))
        t0val = 1000;

    /* PIT timer 0 divisor = t0val (mode 3 is set by initsound/stopint8). */
    outportb(0x40, (u8)(t0val & 0xFF));
    outportb(0x40, (u8)(t0val >> 8));

    /*
     * ISR accumulator.  The original uses 4*t0val for t0val < 0x3E80,
     * otherwise 0xFFFF, which keeps the soundint tick at 72.8 Hz.
     * The port stores t0val and subtracts 0x4000 after each carry instead.
     */
    if (t0val < 0x3E80)
        timerrate = (u16)(4 * t0val);
    else
        timerrate = 0xFFFF;

    if (musvol < 1)
        musvol = 1;
    if (musvol > 50)
        musvol = 50;
    pulsewidth = musvol;             /* original: musvol (volume folded into pwlut) */

    setsoundmode();
}

/* CS:0x7E7C (file 0x7D7C) - verified against port setsoundt2(). */
void setsoundt2(void)
{
    if (soundt0flag) {
        spkrmode = 0;
        soundt0flag = 0;
        /* Original setspkrt2(): enable the speaker/timer-2 gate. */
        outportb(0x61, (u8)((inportb(0x61) & ~0x03) | 0x03));
    }
}

/* CS:0x7EB8 (file 0x7DB8) - verified against port setsoundmode(). */
void setsoundmode(void)
{
    spkrmode = wavetype;
    if (!soundt0flag && sndflag) {
        soundt0flag = 1;
        outportb(0x61, (u8)((inportb(0x61) & ~0x03) | 0x03));
    }
}

/* CS:0x7FA2 (file 0x7EA2) - verified against port sett2val(). */
void sett2val(u16 t2v)
{
    if (sndflag)
        timer2(t2v);
}

/* CS:0x7CC9 (file 0x7BC9) - probable soundoff(). */
void soundoff(void)
{
    outportb(0x61, (u8)(inportb(0x61) & ~0x03));
    outportb(0x40, 0xFF);
    outportb(0x40, 0xFF);             /* timer 0 divisor 0xFFFF */
}

/* CS:0x7EFF (file 0x7DFF) - verified against port startint8(). */
void startint8(void)
{
    if (!int8flag) {
        initint8();                   /* INT 8 hook at CS:0x0339 */
        timerrate = 0xFFFF;
        outportb(0x40, 0x00);
        outportb(0x40, 0x40);         /* timer 0 divisor 0x4000 */
        int8flag = 1;
    }
}

/* CS:0x7F36 (file 0x7E36) - verified structurally; matches stopint8(). */
void stopint8(void)
{
    if (int8flag) {
        restoreint8();
        int8flag = 0;
    }
    sett2val(40);
    outportb(0x61, (u8)((inportb(0x61) & ~0x03) | 0x03));
    outportb(0x43, 0x36);             /* timer 0, mode 3 */
    outportb(0x40, 0x00);
    outportb(0x40, 0x80);             /* timer 0 divisor 0x8000 */
}

/* CS:0x7D10 (file 0x7C10) - probable initsound(); state setup verified. */
void initsound(void)
{
    outportb(0x43, 0xB6);             /* timer 2, mode 3, LSB/MSB */
    outportb(0x42, 0x20);
    outportb(0x42, 0x00);             /* timer 2 divisor 0x0020 */
    outportb(0x61, (u8)((inportb(0x61) & ~0x03) | 0x03));

    outportb(0x43, 0x36);             /* timer 0, mode 3 */
    outportb(0x40, 0x00);
    outportb(0x40, 0x80);             /* timer 0 divisor 0x8000 */

    wavetype = 2;
    t0val = 12000;
    musvol = 8;
    t2val = 40;
    soundt0flag = 1;
    sndflag = 1;
    spkrmode = 0;
    int8flag = 0;

    setsoundt2();
    soundstop();
    startint8();

    timerrate = 0xFFFF;
    outportb(0x40, 0x00);
    outportb(0x40, 0x40);             /* timer 0 divisor 0x4000 */
}

/* ---- music ------------------------------------------------------------ */

/* CS:0x7A70 (file 0x7970) - matches port music(). */
void music(int tune)
{
    tuneno = tune;
    musicp = 0;
    noteduration = 0;
    switch (tune) {
    case 0:
        musicmaxvol = 50;
        musicattackrate = 20;
        musicsustainlevel = 20;
        musicdecayrate = 10;
        musicreleaserate = 4;
        break;
    case 1:
        musicmaxvol = 50;
        musicattackrate = 50;
        musicsustainlevel = 8;
        musicdecayrate = 15;
        musicreleaserate = 1;
        break;
    case 2:
        musicmaxvol = 50;
        musicattackrate = 50;
        musicsustainlevel = 25;
        musicdecayrate = 5;
        musicreleaserate = 1;
        break;
    }
    musicplaying = 1;
    if (tune == 2)
        soundddieoff();
}

/* CS:0x7B02 (file 0x7A02) - matches port musicupdate(). */
void musicupdate(void)
{
    if (!musicplaying)
        return;

    if (noteduration != 0) {
        noteduration--;
    } else {
        musicstage = musicn = 0;
        switch (tuneno) {
        case 0:  /* bonus jingle T1, DS:0x45E4 */
            noteduration = ds_bonus_t1[musicp + 1] * 3;
            musicnotewidth = noteduration - 3;
            notevalue = ds_bonus_t1[musicp];
            musicp += 2;
            if (ds_bonus_t1[musicp] == MUSIC_SENTINEL)
                musicp = 0;
            break;
        case 1:  /* background jingle, DS:0x4866 */
            noteduration = ds_background[musicp + 1] * 6;
            musicnotewidth = 12;
            notevalue = ds_background[musicp];
            musicp += 2;
            if (ds_background[musicp] == MUSIC_SENTINEL)
                musicp = 0;
            break;
        case 2:  /* dirge, DS:0x4AAC */
            noteduration = ds_dirge[musicp + 1] * 10;
            musicnotewidth = noteduration - 10;
            notevalue = ds_dirge[musicp];
            musicp += 2;
            if (ds_dirge[musicp] == MUSIC_SENTINEL)
                musicp = 0;
            break;
        }
    }

    musicn++;
    wavetype = 1;
    t0val = notevalue;
    if (musicn >= musicnotewidth)
        musicstage = 2;

    switch (musicstage) {
    case 0:
        if (musvol + musicattackrate >= musicmaxvol) {
            musicstage = 1;
            musvol = musicmaxvol;
            break;
        }
        musvol += musicattackrate;
        break;
    case 1:
        if (musvol - musicdecayrate <= musicsustainlevel) {
            musvol = musicsustainlevel;
            break;
        }
        musvol -= musicdecayrate;
        break;
    case 2:
        if (musvol - musicreleaserate <= 1) {
            musvol = 1;
            break;
        }
        musvol -= musicreleaserate;
        break;
    }

    if (musvol == 1)
        t0val = 0x7D00;
}

/* ---- individual sounds ------------------------------------------------ */

/* CS:0x76F7 (file 0x75F7) - matches port soundemerald(). */
void soundemerald(int n)
{
    switch (n) {
    case 0: emerfreq = 0x8E8; break;
    case 1: emerfreq = 0x7F0; break;
    case 2: emerfreq = 0x712; break;
    case 3: emerfreq = 0x6AC; break;
    case 4: emerfreq = 0x5F2; break;
    case 5: emerfreq = 0x54C; break;
    case 6: emerfreq = 0x4B8; break;
    case 7: emerfreq = 0x474; break;
    default: emerfreq = 0x8E8; break;  /* n == 0 path in the original */
    }
    soundemeraldduration = 7;
    soundemeraldn = 0;
    soundemeraldflag = 1;
}

/* CS:0x7933 (file 0x7833) - matches port soundddie(). */
void soundddie(void)
{
    soundddien = 0;
    soundddievalue = 20000;
    soundddieflag = 1;
}

/* ---- sound event wrappers ------------------------------------------- */

static u16 fireflag;          /* DS:0x4582 */
static u16 firevalue;         /* DS:0x4584 */
static u16 firen;             /* DS:0x4586 */
static u16 breakflag;         /* DS:0x4594 */
static u16 breakduration;     /* DS:0x4596 */
static u16 breakfreq;         /* DS:0x4598 */
static u16 fallvalue;         /* DS:0x4576 */
static u16 fallflag;          /* DS:0x4578 */
static u16 falln;             /* DS:0x457C */
static u16 wobbleflag;        /* DS:0x457E */
static u16 wobblen;           /* DS:0x4580 */
static u16 goldflag;          /* DS:0x45BE */
static u16 goldvalue;         /* DS:0x45C0 */
static u16 goldduration;      /* DS:0x45C2 */
static u16 goldfreq;          /* DS:0x45C4 */
static u16 goldn;             /* DS:0x45C6 */
static u16 eatmflag;          /* DS:0x45A6 */
static u16 eatmvalue;         /* DS:0x45A8 */
static u16 eatmduration;      /* DS:0x45AA */
static u16 eatmn;             /* DS:0x45AC */
static u16 emflag;            /* DS:0x459C */
static u16 bonusflag;         /* DS:0x4588 */
static u16 bonusoff;          /* DS:0x458A */
static u16 levdoneflag;       /* DS:0x45B8 */
static u16 levdonecounter;    /* DS:0x45BA */
static u16 levdoneduration;   /* DS:0x45BC */

static u16 explodeflag;       /* DS:0x458E */
static u16 explodevalue;      /* DS:0x4590 */
static u16 explodeduration;   /* DS:0x4592 */
static u16 oneupflag;         /* DS:0x45B4 */

/* CS:0x7571 (file 0x7471) */
void soundfire(void)
{
    firevalue = 500;
    fireflag = 1;
}

/* CS:0x7582 (file 0x7482) */
void soundfireoff(void)
{
    firen = 0;
    fireflag = 0;
}

/* CS:0x75E6 (file 0x74E6) */
void soundexplode(void)
{
    /* DS:0x4590 / 0x4592 / 0x458E */
    explodevalue = 1500;
    explodeduration = 10;
    explodeflag = 1;
    soundfireoff();
}

/* CS:0x7807 (file 0x7707) */
void soundgold(void)
{
    goldduration = 500;
    goldfreq = 4000;
    goldn = 30;
    goldvalue = 0;
    goldflag = 1;
}

/* CS:0x7497 (file 0x7397) */
void soundbreak(void)
{
    breakduration = 3;
    if (breakfreq < 0x3A98)
        breakfreq = 0x3A98;
    breakflag = 1;
}

/* CS:0x74BC (file 0x73BC) */
void soundbreakoff(void)
{
    breakflag = 0;
}

/* CS:0x7430 (file 0x7330) */
void soundfall(void)
{
    fallvalue = 1000;
    fallflag = 1;
}

/* CS:0x7441 (file 0x7341) */
void soundfalloff(void)
{
    falln = 0;
    fallflag = 0;
}

/* CS:0x7559 (file 0x7459) */
void soundwobble(void)
{
    wobbleflag = 1;
}

/* CS:0x7564 (file 0x7464) */
void soundwobbleoff(void)
{
    wobblen = 0;
    wobbleflag = 0;
}

/* CS:0x7891 (file 0x7791) */
void soundeatm(void)
{
    eatmduration = 20;
    eatmn = 3;
    eatmvalue = 2000;
    eatmflag = 1;
}

/* CS:0x76CC (file 0x75CC) */
void soundem(void)
{
    emflag = 1;
}

/* CS:0x7646 (file 0x7546) */
void soundbonus(void)
{
    bonusflag = 1;
}

/* CS:0x7651 (file 0x7551) */
void soundbonusoff(void)
{
    bonusoff = 0;
    bonusflag = 0;
}

/* CS:0x7A63 (file 0x7963) */
void musicoff(void)
{
    musicp = 0;
    musicplaying = 0;
}

void soundexplodeoff(void);
void soundemoff(void);
void soundemeraldoff(void);
void soundgoldoff(void);
void soundeatmoff(void);
void soundddieoff(void);
void sound1upoff(void);
void soundlevdoneupdate(void);
void soundlevdoneoff(void);
void soundpause(void);
extern void delay(int n);

/* CS:0x72D7 (file 0x71D7) - stop every effect and the music. */
void soundstopall(void)
{
    soundfalloff();
    soundwobbleoff();
    soundfireoff();
    musicoff();
    soundbonusoff();
    soundexplodeoff();
    soundbreakoff();
    soundemoff();
    soundemeraldoff();
    soundddieoff();
    soundgoldoff();
    soundeatmoff();
    sound1upoff();
}

/* CS:0x7CB3 (file 0x7BB3) */
void soundpause(void)
{
    soundpaused = 1;
}

/* CS:0x7CBE (file 0x7BBE) */
void soundpauseoff(void)
{
    soundpaused = 0;
}

/* CS:0x7303 (file 0x7203) - stop the level-done tune. */
void soundlevdoneoff(void)
{
    soundpaused = 0;
    levdoneflag = 0;
}

/* CS:0x7310 (file 0x7210) - play the level-done tune, blocking until the
 * ISR clears DS:0x45B8. */
void soundlevdone(void)
{
    u8 last = 0;

    soundstopall();
    levdonecounter = 0;
    levdoneduration = 20;
    levdoneflag = 1;
    soundpaused = 1;
    while (levdoneflag) {
        if (timerclock != last) {
            soundlevdoneupdate();
            last = timerclock;
        }
    }
}

/* ---- remaining sound off routines and the level-done tune ------------ */

/* CS:0x7600 (file 0x7500) */
void soundexplodeoff(void)
{
    explodeflag = 0;
}

/* CS:0x76C1 (file 0x75C1) */
void soundemoff(void)
{
    emflag = 0;
}

/* CS:0x76EC (file 0x75EC) */
void soundemeraldoff(void)
{
    soundemeraldflag = 0;
}

/* CS:0x77FC (file 0x76FC) */
void soundgoldoff(void)
{
    goldflag = 0;
}

/* CS:0x7886 (file 0x7786) */
void soundeatmoff(void)
{
    eatmflag = 0;
}

/* CS:0x7928 (file 0x7828) */
void soundddieoff(void)
{
    soundddieflag = 0;
}

/* CS:0x7A00 (file 0x7900) */
void sound1upoff(void)
{
    oneupflag = 0;
}

/* CS:0x7351 (file 0x7251) - one note of the level-done tune, called from
 * the timer tick while soundlevdone() waits. */
void soundlevdoneupdate(void)
{
    static const u16 freq[11] = {
        0x8E8, 0x712, 0x5F2, 0x7F0, 0x6AC, 0x54C,
        0x712, 0x5F2, 0x4B8, 0x474, 0x474
    };

    if (!sndflag) {
        delay(100);
        levdoneflag = 0;
        return;
    }
    if (levdonecounter > 10)
        return;

    t2val = freq[levdonecounter];
    musvol = 50;
    t0val = (u16)(t2val + 0x23);
    setsoundmode();
    sett0();
    sett2val(t2val);

    if (levdoneduration > 0)
        levdoneduration--;
    else {
        levdoneduration = 20;
        if (++levdonecounter > 10)
            soundlevdoneoff();
    }
}
