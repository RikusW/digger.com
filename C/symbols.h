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

/* Code addresses (CS:IP). */
#define CS_TIMER_ISR            0x0397u  /* INT 8 handler: PC speaker + soundint gate */
#define CS_INT8_HOOK            0x0339u  /* saves old vector, points INT 8 at CS_TIMER_ISR */
#define CS_SCORE_INIT           0x5EEAu  /* probable: scores/next-bonus initialisation */
#define CS_SCORE_ADD            0x5FA1u  /* probable: score/extra-life update */
#define CS_SOUNDINT             0x7227u
#define CS_SOUNDEMERALD         0x76F7u
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
#define CS_MAIN_TITLE           0x82A6u  /* probable */
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
#define CS_DODIGGER             0x4939u  /* verified by bonus block at CS:0x4912..0x4A11 */
#define CS_DOMONSTERS           0x2C90u  /* verified structure */
#define CS_MONAI                0x2E75u  /* verified via randno(levof10()+5) */
#define CS_DOBAGS               0x3E2Du  /* probable */
#define CS_CHECKLEVDONE         0x89F2u  /* probable */
#define CS_INITLEVEL            0x89A4u  /* probable */
#define CS_DRAWSCREEN           0x89CDu  /* probable */
#define CS_INITCHARS            0x89E4u  /* probable */
#define CS_INCMONT              0x3AEFu  /* probable */
#define CS_GETNMOVINGBAGS       0x47F8u  /* probable */
#define CS_COUNTEM              0x5A94u  /* probable */
#define CS_READDIR              0x5BB9u  /* probable */
#define CS_INITSCORES           0x5DE0u  /* verified: calls addscore(0) */
#define CS_CREATEBONUS          0x5510u  /* verified */
#define CS_INITBONUSMODE        0x5528u  /* verified */
#define CS_ENDBONUSMODE         0x5562u  /* verified */
#define CS_ERASEBONUS           0x5575u  /* verified */
#define CS_SOUNDLEVDONE         0x7310u  /* probable */
#define CS_MUSICOFF             0x7A63u  /* probable */
#define CS_SOUNDSTOP            0x71D7u  /* probable */

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
#define DS_NPLAYERS             0x500Eu  /* probable */
#define DS_PENALTY              0x5014u
#define DS_LEVEL_BASE           0x4FE2u  /* + 8*curplayer */
#define DS_LIVES_A              0x4FE0u
#define DS_LIVES_B              0x4FE8u
#define DS_KEYPRESSED           0x0658u
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
