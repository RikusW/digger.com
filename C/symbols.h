#ifndef DIGGER_ORIGINAL_SYMBOLS_H
#define DIGGER_ORIGINAL_SYMBOLS_H

/*
 * Addresses found in digger.com.
 *
 * CS addresses are CS:IP values (the .COM loads at CS:0x100).  File offset =
 * CS - 0x100.  DS offsets are relative to DS = CS+0x08E7; file offset =
 * DS + 0x8D70.  See SYMBOLS.md for confidence levels and evidence.
 */

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint32_t u32;
typedef int32_t  s32;

/* Direction values (same numbering as the port). */
#define DIR_NONE  (-1)
#define DIR_RIGHT 0
#define DIR_UP    2
#define DIR_LEFT  4
#define DIR_DOWN  6

#define MONSTERS 6
#define BAGS     7

/* Sprite-presence bits.  The original numbers the digger as sprite 0, the
 * bags as 1..7 and the monsters as 8..13; drawmon() returns the set of
 * sprites it overlapped. */
#define COLL_DIGGER   0x0001
#define COLL_BAG_BIT  0x00FE
#define COLL_MON_BIT  0x3F00

/* Code addresses (CS:IP). */
#define CS_TIMER_ISR            0x0397u  /* INT 8 handler: PC speaker + soundint gate */
#define CS_INT8_HOOK            0x0339u  /* saves old vector, points INT 8 at CS_TIMER_ISR */
#define CS_SCORE_INIT           0x5EEAu  /* probable: scores/next-bonus initialisation */
#define CS_SCORE_ADD            0x5FA1u  /* probable: score/extra-life update */
#define CS_SOUNDINT             0x7227u
#define CS_SOUNDEMERALD         0x76F7u  /* arg is a start flag; steps DS:0x45A2 through a frequency table */
#define CS_SOUNDEMERALD_UPDATE  0x779Bu  /* mapped from soundint call order */
#define CS_SOUNDWOBBLE_UPDATE   0x74ECu
#define CS_SOUNDDDIE_UPDATE     0x794Au
#define CS_SOUNDBREAK_UPDATE    0x74C7u
#define CS_SOUNDGOLD_UPDATE     0x782Au
#define CS_SOUNDEM_UPDATE       0x76D7u
#define CS_SOUNDEXPLODE_UPDATE  0x760Bu
#define CS_SOUNDFIRE_UPDATE     0x758Fu
#define CS_SOUNDEATM_UPDATE     0x78AEu
#define CS_SOUNDFALL_UPDATE     0x744Eu
#define CS_SOUND1UP_UPDATE      0x7A1Cu
#define CS_SOUNDBONUS_UPDATE    0x765Eu
#define CS_SOUNDDDIE            0x7933u
#define CS_MUSIC                0x7A70u
#define CS_MUSICUPDATE          0x7B02u
#define CS_GETLEVCH             0x8001u
#define CS_MAIN_TITLE           0x82A6u  /* verified: title/menu + life loop */
#define CS_DRAWGLYPH            0x08E5u  /* glyph draw: [w][h][2bpp rows] in DS */
#define CS_DRAWCHAR             0x11E4u  /* character -> glyph pointer table at 0x1523 */
#define CS_WRITESTRING          0x11A4u  /* (DS string, x, y, colour); 12 px cells */
#define CS_DISKCHECK            0x026Cu  /* returns 0x45 on success, 0 on failure */
#define CS_HANG                 0x08BE0u  /* infinite loop on a failed check */
#define CS_INPUT_INIT           0x5B3Fu  /* keyboard-controller reset */
#define CS_READKEY              0x5C5Eu  /* nonzero starts the game from the title */
#define CS_INSTALL_KEYBOARD     0x0E06u  /* hooks the keyboard interrupt */
#define CS_SOUND_OFF            0x7CC9u  /* probable: speaker off + timer 0 idle */
#define CS_INITSOUND            0x7D10u  /* probable */
#define CS_SETT0                0x7DD9u  /* verified */
#define CS_SETSOUNDT2           0x7E7Cu  /* verified */
#define CS_SETSOUNDMODE         0x7EB8u  /* verified */
#define CS_STARTINT8            0x7EFFu  /* verified */
#define CS_STOPINT8             0x7F36u  /* verified */
#define CS_SETT2VAL             0x7FA2u  /* verified */
#define CS_TIMER2_WRITE         0x7FD5u  /* verified: writes port 0x42 */
#define CS_SCOREWRAP_250A       0x68A2u  /* probable: scorekill/scoreoctave */
#define CS_SCOREWRAP_25         0x68B0u  /* probable: scoreemerald */
#define CS_SCOREWRAP_250B       0x68BEu  /* probable: scoreoctave/scorekill */
#define CS_SCOREWRAP_500        0x68CCu  /* probable: scoregold */
#define CS_SCOREWRAP_1000       0x68DAu  /* probable: scorebonus */
#define CS_INCPENALTY           0x8A32u  /* verified */
#define CS_LEVPLAN              0x8A65u  /* verified */
#define CS_LEVOF10              0x8A8Du  /* verified */
#define CS_LEVNO                0x8ABDu  /* verified */
#define CS_GETLIVES             0x8AF0u  /* verified */
#define CS_ADDLIFE              0x8B0Cu  /* verified */
#define CS_CURPLAYER            0x8B45u  /* verified getter */
#define CS_NPLAYERS             0x8B4Du  /* probable getter */
#define CS_TESTPAUSE            0x8B55u  /* probable */
#define CS_SOUNDPAUSE           0x7CB3u  /* verified: sets soundpaused */
#define CS_SOUNDPAUSEOFF        0x7CBEu  /* verified: clears soundpaused */
#define CS_INPORTB              0x8DDDu  /* probable runtime helper */
#define CS_OUTPORTB             0x8DE8u  /* probable runtime helper */
#define CS_LONG_DIVIDE          0x8C7Du  /* probable runtime helper */
#define CS_LONG_COMPARE         0x8E23u  /* probable runtime helper */
#define CS_VIDEO_RLE_BLIT       0x17EBu  /* DS:0x0DAC -> B800, 0x4000 bytes */
#define CS_CGA_ENTRY            0x03F4u
#define CS_CGA_VIDEO_INIT       0x03FCu
#define CS_CGA_PROBE            0x0418u
#define CS_CGA_INIT             0x0429u
#define CS_CGA_PRINT_TEXT       0x0456u
#define CS_CGA_MODE_OFF         0x048Au
#define CS_CGA_MODE_ON          0x04A0u
#define CS_CGA_CLEAR            0x04AEu
#define CS_CGA_CLEAR_FILL       0x04B9u
#define CS_CGA_PALETTE          0x04E7u
#define CS_CGA_WAIT_RETRACE     0x050Bu
#define CS_CGA_SET_BACKGROUND   0x0520u
#define CS_CGA_SET_INTENSITY    0x054Bu
#define CS_CGA_PUTPIXEL         0x057Au
#define CS_CGA_XORPIXEL         0x0595u
#define CS_CGA_PIXEL_WORKER     0x05B0u
#define CS_CGA_BLOCK_PUT        0x0628u
#define CS_CGA_BLOCK_XOR        0x0655u
#define CS_CGA_BLOCK_WORKER     0x0682u
#define CS_CGA_SHIFT_INC        0x1128u
#define CS_CGA_SHIFT_DEC        0x1155u

/* Random number generator: table DS:0x0CA7 + counter DS:0x0DA7 + BIOS tick. */
#define CS_RAND                 0x177Fu
#define CS_BIOS_TICK_BYTE       0x17C1u

/* Game loop and core updates (mapped from the port's game()/domonsters()). */
#define CS_GAME_LOOP            0x86F1u
#define CS_CLEARTOPLINE         0x8A3Bu  /* clears the top status line */
#define CS_DRAWSCORE            0x5F2Eu  /* current player score at (0,6), colour arg */
#define CS_DRAWHIGHSCORE        0x62D0u
#define CS_GAMEOVER             0x610Bu  /* called when the last life is lost */
#define CS_ERASEDIGGERS         0x4926u
#define CS_CLEANUPBAGS          0x3D2Fu
#define CS_STRCPY_DS            0x8D2Du  /* C runtime: strcpy into the data segment */
#define CS_STRCAT_DS            0x8DF4u  /* C runtime: strcat */
#define CS_DODIGGER             0x4939u  /* verified by bonus block at CS:0x4912..0x4A11 */
#define CS_DOMONSTERS           0x2C90u  /* verified structure */
#define CS_DOBAGS               0x3E2Du  /* verified */
#define CS_CHECKLEVDONE         0x89F2u  /* probable */
#define CS_INITLEVEL            0x89A4u  /* probable */
#define CS_DRAWSCREEN           0x89CDu  /* probable */
#define CS_INITCHARS            0x89E4u  /* probable */
#define CS_READDIR              0x5BB9u  /* probable */
#define CS_INITSCORES           0x5DE0u  /* verified: calls addscore(0) */
#define CS_CREATEBONUS          0x5510u  /* verified */
#define CS_INITBONUSMODE        0x5528u  /* verified */
#define CS_ENDBONUSMODE         0x5562u  /* verified */
#define CS_ERASEBONUS           0x5575u  /* verified */
#define CS_SOUNDLEVDONE         0x7310u  /* probable */
#define CS_MUSICOFF             0x7A63u  /* probable */
#define CS_SOUNDSTOP            0x71D7u  /* probable */
#define CS_UPDATEFIRE           0x4A3Eu  /* updatefire */
#define CS_DRAWEXPLOSION        0x4E66u  /* drawexplosion */
#define CS_KILLFIRE             0x4EE3u  /* verified */
#define CS_MOVEDRAWSPR          0x07B1u
#define CS_ERASESPR             0x07D3u
#define CS_SETSPR               0x0804u  /* (spr, data, mask, w, h, bw, bh) */
#define CS_SETSPR2              0x0754u  /* also fills the live arrays and clears sprenf */
#define CS_DRAWSPR              0x07E4u  /* sets 0x59F/0x5A1 then falls into 0x0997 */
#define CS_DRAWSPR_CORE         0x0997u
#define CS_SETORDER             0x083Cu  /* draw-order pointer, default 0x0627 */
#define CS_INITMISCSPR          0x084Fu  /* -> 0x0A32 */
#define CS_INITMISCSPR2         0x0881u  /* -> 0x0A3C, returns the redraw mask */
#define CS_DRAWMISCSPR          0x08ADu  /* -> 0x0CF3 */
#define CS_GETIS                0x0876u  /* -> 0x0CBA + 0x0D13 */
#define CS_CLEARRDRWF           0x0A65u
#define CS_CLEARRECF            0x0A7Cu
#define CS_SETRDRWFLGS          0x0A90u
#define CS_COLLIDE              0x0B05u
#define CS_BCOLLIDE             0x0B6Eu
#define CS_BCOLLIDES            0x0D48u  /* fills first[]/coll[] */
#define CS_PUTIS                0x0D30u  /* restore saved backgrounds */
#define CS_PUTIMS               0x0D13u  /* draw sprites via the order table */
#define CS_GGETI                0x0CD2u  /* screen -> buffer */
#define CS_GPUTI                0x0BFFu  /* buffer -> screen */
#define CS_GPUTIM               0x0CF3u  /* masked data/mask draw */
#define CS_GPUTICH              0x0C1Eu  /* glyph draw with DS:0x0623 */
#define CS_DRAWFIRE             0x24C6u  /* verified: 2x7; returns the collision mask */
#define CS_GGETPIX              0x56A6u  /* ggetpix(x, y) */
#define CS_GETFIREPFLAG         0x5DD8u  /* returns DS:0x40BC */
#define CS_SOUNDFIRE            0x7571u  /* verified: value 500, flag 1 */
#define CS_SOUNDFIREOFF         0x7582u  /* verified: clears the fire sound */
#define CS_SOUNDEXPLODE         0x75E6u  /* verified: value 1500, duration 10 */
#define CS_UPDATEDIGGER         0x4F07u  /* updatedigger */
#define CS_DIGGERDIE            0x5218u  /* diggerdie */
#define CS_SOUNDBONUS           0x7646u
#define CS_SOUNDBONUSOFF        0x7651u
#define CS_UPDATEBAG            0x3FEDu
#define CS_REMOVEBAG            0x4703u
#define CS_GETFIELD             0x7207u
#define CS_DRAWGOLD             0x1E9Du  /* verified: frames 0..6, 4x15 */
#define CS_SOUNDBREAK           0x7497u
#define CS_DIGGERX              0x5650u
#define CS_DIGGERY              0x5658u
#define CS_REVERSEDIR           0x55BDu
#define CS_BONUSMODE_GET        0x55B5u
#define CS_CREATEMONSTER        0x2DD3u  /* verified: fills the 0x28 record, stime=5 */
#define CS_INITBAGS             0x3B31u  /* verified: scans level for 'B' */
#define CS_MAKEFIELD            0x6A48u  /* probable */
#define CS_MAKEEMFIELD          0x56E8u  /* probable */
#define CS_CREATEMBSPR          0x1826u  /* verified: 3-frame buffers for sprites 1..13 */
#define CS_DRAWSTATICS          0x6B31u  /* probable */
#define CS_DRAWBAGS             0x3C93u  /* probable */
#define CS_DRAWEMERALDS         0x5777u  /* probable */
#define CS_INITDIGGER           0x48A0u  /* probable */
#define CS_INITMONSTERS         0x2BC1u  /* probable */
#define CS_INITMBSPR            0x1A03u  /* verified: default frames for sprites 1..13 */
#define CS_ERASEMONSTERS        0x2C58u
#define CS_MONAI                0x2E75u  /* verified */
#define CS_MONDIE               0x3682u
#define CS_NMONONSCR            0x3ABBu  /* verified */
#define CS_INCMONT              0x3AEFu  /* verified: increments t of monsters 1..n-1 */
#define CS_MONGOLD              0x3B26u  /* verified: sets DS:0x3C0C */
#define CS_MONSTER_COLLISIONS   0x3795u  /* verified */
#define CS_COLLISION_PENALTIES  0x3767u  /* verified */
#define CS_FIELDCLEAR           0x3815u  /* verified */
#define CS_DRAWMON              0x1B84u  /* verified: nobbin/hobbin + leg animation */
#define CS_DRAWMONDIE           0x1DDBu  /* verified: 4x15 / 4x14 death frames */
#define CS_DRAWRIGHTBLOB        0x21CCu  /* verified: x+16, y-1, 2x18 */
#define CS_DRAWLEFTBLOB         0x2214u  /* verified: x-8, y-1, 2x18 */
#define CS_DRAWTOPBLOB          0x225Cu  /* verified: x-4, y-6, 6x6 */
#define CS_DRAWBOTTOMBLOB       0x22A2u  /* verified: x-4, y+15, 6x6 */
#define CS_EATFIELD             0x6DFEu
#define CS_HITMERALD            0x57F1u
#define CS_BAGMASK              0x4774u  /* OR of 1<<bag for existing bags */
#define CS_PUSHBAGS             0x466Cu
#define CS_PUSHUDBAGS           0x46B2u
#define CS_REMOVEBAGS           0x47B5u
#define CS_BAGY                 0x4730u
#define CS_GETBAGDIR            0x4749u
#define CS_KILLMON              0x39A9u  /* verified */
#define CS_KILLDIGGER0          0x5668u  /* verified: killdigger(death, bag) for digger 0 */
#define CS_SCEATM               0x5598u  /* verified: addscore(msc*200), msc <<= 1 */
#define CS_SOUNDEATM            0x7891u  /* verified */
#define CS_SCOREKILL            0x689Fu  /* verified: addscore(250) */
#define CS_KILLEMERALD          0x5AE0u  /* clears the emerald below (h, v+1) */
#define CS_COUNTEM              0x5A94u  /* counts emeralds for emmask */
#define CS_DRAWEMERALD          0x208Au  /* verified: 4x10, sources 0x16E4/0x170D */
#define CS_ERASEEMERALD         0x20C0u  /* verified: 4x10, sources 0x175E/0x1787 */
#define CS_DRAWBONUS            0x260Eu  /* verified: sprite 0x0E */
#define CS_DRAWDIG              0x263Fu  /* verified: frames/legs, 4x15 */
#define CS_CREATEDBFSPR         0x20F6u  /* verified: 3-frame digger/bonus/fire buffers */
#define CS_INITDBFSPR           0x2167u  /* verified: resets the animation state */
#define CS_DRAWFURRYBLOB        0x22E8u  /* verified: x-4,y+15, 6x8 */
#define CS_DRAWSQUAREBLOB       0x2334u  /* verified: x-4,y+17, 6x6 */
#define CS_DRAWBACKG            0x237Au  /* verified: 8 tile sets, 5x4 */
#define CS_DRAWFIELD            0x6C3Eu  /* verified: walls from the field bits */
#define CS_SAVEFIELD            0x6BCAu  /* verified: field -> saved copy */
#define CS_CHECKMONSCARED       0x3962u  /* monsters on row h facing UP turn DOWN */
#define CS_SQUASHMONSTER        0x3A7Cu  /* alive=0; death/bag fields */
#define CS_SQUASHMONSTERS       0x39E7u  /* squash monsters hit by a bag */
#define CS_MONLEFT              0x3AA5u  /* nmononscr + totalmonsters - nextmonster */
#define CS_CHECKUNDERBAG        0x5602u
#define CS_BAGHITGROUND         0x4296u
#define CS_PUSHBAG              0x433Bu
#define CS_GETGOLD              0x4853u
#define CS_GETNMOVINGBAGS       0x47F8u
#define CS_SCOREEMERALD         0x68ADu  /* verified: addscore(25) */
#define CS_SCOREOCTAVE          0x68BBu  /* verified: addscore(250) */
#define CS_SCOREGOLD            0x68C9u  /* verified: addscore(500) */
#define CS_SCOREBONUS           0x68D7u  /* verified: addscore(1000) */
#define CS_GETDIR               0x5D71u  /* returns DS:0x40B0 or -1 */
#define CS_KILLMONSTERS         0x3A40u  /* killmon() over the mask, returns count */
#define CS_SOUNDEM              0x76CCu  /* sets DS:0x459C */

/* Score storage: INT 13h sector 7 (cylinder 39, head 0). */
#define CS_READ_SECTOR7         0x028Fu
#define CS_WRITE_SECTOR7        0x02B8u
#define CS_READ_SECTOR2         0x0321u
#define CS_DISK_CHECK           0x02F3u
#define CS_LOADSCORES           0x5DEDu
#define CS_SAVESCORES           0x6521u

/* Sound/timer state (DS offsets). */
#define DS_SPKRMODE             0x0248u
#define DS_TIMERRATE            0x024Au  /* ISR accumulator: sett0 stores 4*t0val (or 0xFFFF) */
#define DS_TIMERCOUNT           0x024Cu
#define DS_TIMERCLOCK           0x024Eu  /* byte */
#define DS_PULSEWIDTH           0x024Fu
#define DS_OLD_INT8_OFFSET      0x0251u
#define DS_OLD_INT8_SEGMENT     0x0253u
#define DS_SOUNDFLAG            0x0667u
#define DS_MUSICFLAG            0x0669u

#define DS_SNDFLAG              0x4566u
#define DS_INT8FLAG             0x4568u
#define DS_SOUNDT0FLAG          0x456Au
#define DS_WAVETYPE             0x456Cu
#define DS_T2VAL                0x456Eu
#define DS_T0VAL                0x4570u
#define DS_MUSVOL               0x4572u
#define DS_EXPLODE_FLAG0        0x458Eu
#define DS_EXPLODE_VALUE0       0x4590u
#define DS_EXPLODE_DUR0         0x4592u
#define DS_EMERALD_FLAG         0x45A0u
#define DS_EMERFREQ             0x45A2u
#define DS_EMERALD_N            0x45A4u
#define DS_EMERALD_DURATION     0x459Eu
#define DS_DDIE_FLAG            0x45AEu
#define DS_DDIE_N               0x45B0u
#define DS_DDIE_VALUE           0x45B2u
#define DS_MUSICP               0x45C8u
#define DS_TUNENO               0x45CAu
#define DS_NOTEDURATION         0x45CCu
#define DS_NOTEVALUE            0x45CEu
#define DS_MUSICPLAYING         0x45D0u
#define DS_SOUNDPAUSED          0x45D2u
#define DS_MUSICMAXVOL          0x45D4u
#define DS_MUSICATTACKRATE      0x45D6u
#define DS_MUSICSUSTAINLEVEL    0x45D8u
#define DS_MUSICDECAYRATE       0x45DAu
#define DS_MUSICNOTEWIDTH       0x45DCu
#define DS_MUSICRELEASERATE     0x45DEu
#define DS_MUSICSTAGE           0x45E0u
#define DS_MUSICN               0x45E2u
#define DS_CURPLAYER            0x500Cu
#define DS_PENALTY              0x5014u
#define DS_LEVEL_BASE           0x4FE2u  /* + 8*curplayer */
#define DS_LIVES_A              0x4FE0u
#define DS_LIVES_B              0x4FE8u
#define DS_SCOREBUF             0x0046u  /* 512-byte disk sector / score buffer */
#define DS_DISK_RETRY           0x0247u
#define DS_RNG_TABLE            0x0CA7u  /* 256-byte permutation */
#define DS_RNG_COUNTER          0x0DA7u
#define DS_BONUSVISIBLE         0x4010u
#define DS_BONUSMODE            0x4012u
#define DS_STARTBONUSTIMELEFT   0x400Cu
#define DS_BONUSTIMELEFT        0x400Eu
#define DS_BONUSMSC             0x4014u  /* probable */
#define DS_HIGHBUF              0x40D6u
#define DS_SCOREHIGH            0x4108u  /* 12 x 32-bit */
#define DS_SCOREINIT            0x4134u  /* 11 x 4 chars */
#define DS_LEVNOTDRAWN          0x5010u
#define DS_LEVNOTDRAWN1         0x5012u
#define DS_PLAYER_DEAD1         0x4FE6u
#define DS_PLAYER_LIVES         0x4FE0u  /* plus 8*curplayer */
#define DS_PLAYER_LEVEL         0x4FE2u  /* plus 8*curplayer */
#define DS_PLAYER_LEVDONE       0x4FE4u  /* plus 8*curplayer */
#define DS_PLAYER_LIVES1        0x4FE8u
#define DS_PLAYER_LEVEL1        0x4FEAu
#define DS_PLAYER_LEVDONE1      0x4FECu
#define DS_PLAYER_BANNER        0x4FEEu  /* "PLAYER n" scratch string */
#define DS_CHECKBYTE            0x5016u  /* 0x45 when the disk signature check passed */
#define DS_NPLAYERS             0x500Eu
#define DS_MENUFLAG             0x0665u
#define DS_KEYPRESSED           0x0658u
#define DS_TITLE_ORDER          0x4FBEu  /* 16-word alternate sprite order */
#define DS_STR_TITLE            0x5018u  /* "D I G G E R" */
#define DS_STR_BLANK12          0x5024u
#define DS_STR_NOBBIN           0x5031u
#define DS_STR_HOBBIN           0x5038u
#define DS_STR_DIGGER           0x503Fu
#define DS_STR_GOLD             0x5046u
#define DS_STR_EMERALD          0x504Bu
#define DS_STR_BONUS            0x5053u
#define DS_PLAYER_DEAD          0x4FDEu  /* per-player: +8*curplayer; setdead() */
#define DS_FIELD                0x4438u  /* 15x10 words, row stride 0x1E */
#define DS_FIELD0               0x41E0u  /* saved field, player 0 */
#define DS_FIELD1               0x430Cu  /* saved field, player 1 */
#define DS_EMFIELD              0x4018u  /* 15x10 bytes, row stride 0x0F */
#define DS_EMMASK               0x40AEu  /* per-player emerald mask */
#define DS_SPRRDRWF             0x03CFu  /* 17 words: needs redraw */
#define DS_SPRRECF              0x03F1u  /* 17 words: recursion guard */
#define DS_SPRENF               0x0413u  /* 16 words: on screen */
#define DS_SPRCH                0x0433u  /* live data source */
#define DS_SPRMASK              0x0455u  /* live mask source */
#define DS_SPRBUF               0x0477u  /* saved background buffer */
#define DS_SPRX                 0x0497u
#define DS_SPRY                 0x04B9u
#define DS_SPRWID               0x04DBu
#define DS_SPRHEI               0x04FDu
#define DS_SPRBWID              0x051Fu
#define DS_SPRBHEI              0x053Fu
#define DS_SPRNCH               0x055Fu
#define DS_SPRNMASK             0x057Fu
#define DS_SPRNWID              0x05A3u
#define DS_SPRNHEI              0x05C3u
#define DS_SPRNBWID             0x05E3u
#define DS_SPRNBHEI             0x0603u
#define DS_SPRNEWX              0x059Fu
#define DS_SPRNEWY              0x05A1u
#define DS_SPRMASKBYTE          0x0623u
#define DS_SPRBANK              0x0624u
#define DS_SPRORDERPTR          0x0625u
#define DS_SPRORDER             0x0627u  /* default order 0..15 */
#define DS_MONLEG               0x16CCu  /* 6 words: per-monster leg position */
#define DS_MONDELTA             0x16D8u  /* 6 words: per-monster leg delta */
#define DS_DIGLEG               0x2700u
#define DS_DIGDELTA             0x2702u
#define DS_FIRELEG              0x2704u

/* Current digger state (single-digger globals in the original). */
#define DS_DIG_X                0x3FDEu
#define DS_DIG_Y                0x3FE0u
#define DS_DIG_H                0x3FE2u
#define DS_DIG_V                0x3FE4u
#define DS_DIG_RX               0x3FE6u
#define DS_DIG_RY               0x3FE8u
#define DS_DIG_MDIR             0x3FEAu
#define DS_DIG_DDIR             0x3FECu  /* key direction; the port keeps it local */
#define DS_DIG_DIR              0x3FEEu  /* last valid direction */
#define DS_DIG_BAGTIME          0x3FF0u
#define DS_DIG_ALIVE            0x3FF2u
#define DS_DIG_VISIBLE          0x3FF4u
#define DS_DIG_NOTFIRING        0x3FF6u
#define DS_DIG_RECHARGETIME     0x3FF8u
#define DS_DIG_FX               0x3FFAu
#define DS_DIG_FY               0x3FFCu
#define DS_DIG_FDIR             0x3FFEu
#define DS_DIG_EXPSN            0x4000u
#define DS_DIG_DEATH            0x4002u  /* killdigger() writes the death state */
#define DS_DIG_BAG              0x4004u  /* ... and the bag index */
#define DS_DIG_DEATHANI         0x4006u
#define DS_DIG_DEATHTIME        0x4008u
#define DS_DIG_EMN              0x400Au
#define DS_DIG_EMOCTTIME        0x4016u
#define DS_FIREPRESSED          0x0663u  /* digger fire latch: set by input, consumed at CS:0x5BDF */

/* Monster scheduling globals. */
#define DS_MONGOTGOLD           0x3C0Cu
#define DS_NEXTMONSTER          0x3C0Eu
#define DS_TOTALMONSTERS        0x3C10u
#define DS_MAXMONONSCR          0x3C12u
#define DS_NEXTMONTIME          0x3C14u
#define DS_MONGAPTIME           0x3C16u
#define DS_UNBONUSFLAG          0x3C18u

/* Bag record, stride 0x28. */
#define DS_BAG_X                0x3E9Au
#define DS_BAG_Y                0x3E9Cu
#define DS_BAG_H                0x3E9Eu
#define DS_BAG_V                0x3EA0u
#define DS_BAG_XR               0x3EA2u
#define DS_BAG_YR               0x3EA4u
#define DS_BAG_DIR              0x3EA6u
#define DS_BAG_WOBBLING         0x3EA8u
#define DS_BAG_WT               0x3EAAu
#define DS_BAG_UNFALLEN         0x3EACu
#define DS_BAG_EXIST            0x3EAEu
#define DS_BAG_GT               0x3EB0u
#define DS_BAG_FALLH            0x3EB2u
#define DS_BAGSAVE0             0x3C1Au  /* player 0 saved bag array */
#define DS_BAGSAVE1             0x3D5Au  /* player 1 saved bag array */
#define DS_PUSHCOUNT            0x3FDAu  /* set by the push routine, per-frame decay */
#define DS_GOLDTIME             0x3FDCu  /* 150 - levof10()*10 (initbags) */

/* Monster record, stride 0x28. */
#define DS_MON_FLAG             0x3B1Cu
#define DS_MON_X                0x3B1Eu
#define DS_MON_Y                0x3B20u
#define DS_MON_H                0x3B22u
#define DS_MON_V                0x3B24u
#define DS_MON_XR               0x3B26u
#define DS_MON_YR               0x3B28u
#define DS_MON_DIR              0x3B2Au
#define DS_MON_HDIR             0x3B2Cu
#define DS_MON_T                0x3B2Eu
#define DS_MON_ALIVE            0x3B30u
#define DS_MON_NOB              0x3B32u
#define DS_MON_HNT              0x3B34u
#define DS_MON_DEATH            0x3B36u
#define DS_MON_BAG              0x3B38u
#define DS_MON_DTIME            0x3B3Au
#define DS_MON_STIME            0x3B3Cu

/* Data tables (DS offsets). */
#define DS_CGA_CRTC_TABLE       0x0255u  /* 14 bytes, R0..R13 */
#define DS_CGA_MSG_PLEASE       0x0263u  /* $ terminated */
#define DS_CGA_MSG_REQUIRES     0x0322u  /* $ terminated */
#define DS_CGA_RETRACE_FLAG     0x03B3u
#define DS_CGA_WRITE_MODE       0x03B4u  /* 0 = replace, 0xFF = XOR */
#define DS_CGA_COLOR_SELECT     0x03B5u  /* cached 3D9 value */
#define DS_CGA_PIXEL_COLOR      0x03B6u
#define DS_CGA_R2               0x0673u  /* current CRTC R2; starts at 0x2C */
#define DS_RLE_BITMAP           0x0DACu  /* title-screen RLE, 0x4000 bytes out */
#define DS_BONUS_T1             0x45E4u  /* 86 words */
#define DS_DEFAULT_SCORES       0x4690u  /* 112 bytes */
#define DS_BONUS_T2             0x4700u  /* 179 words, ends 0x7D64 */
#define DS_BACKGROUND           0x4866u  /* 291 words */
#define DS_DIRGE                0x4AACu  /* 49 words */
#define DS_LEVEL1               0x4B0Eu
#define DS_LEVEL2               0x4BA4u
#define DS_LEVEL3               0x4C3Au
#define DS_LEVEL4               0x4CD0u
#define DS_LEVEL5               0x4D66u
#define DS_LEVEL6               0x4DFCu
#define DS_LEVEL7               0x4E92u
#define DS_LEVEL8               0x4F28u
#define DS_STRINGS              0x4FBEu
#define DS_CTYPE                0x509Au

#define MUSIC_SENTINEL          0x7D64u

#endif /* DIGGER_ORIGINAL_SYMBOLS_H */
