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
#define CS_SOUND_SHUTDOWN       0x7CC9u  /* probable */
#define CS_PIT_TIMER2_SETUP     0x7D10u  /* probable */
#define CS_SETT0                0x7DD9u  /* probable */
#define CS_SETSOUNDT2           0x7E7Cu  /* probable */
#define CS_SETSOUNDMODE         0x7EB8u  /* probable */
#define CS_SETT2VAL             0x7FA2u  /* probable */
#define CS_SETTIMER2_WRITE      0x7FD5u  /* probable */
#define CS_INPORTB              0x8DDDu  /* probable runtime helper */
#define CS_OUTPORTB             0x8DE8u  /* probable runtime helper */
#define CS_LONG_DIVIDE          0x8C7Du  /* probable runtime helper */
#define CS_LONG_COMPARE         0x8E23u  /* probable runtime helper */
#define CS_VIDEO_RLE_BLIT       0x17EBu  /* probable: DS:0x0DAC -> B800 */

/* Sound/timer state (DS offsets). */
#define DS_SPKRMODE             0x0248u
#define DS_TIMERRATE            0x024Au
#define DS_TIMERCOUNT           0x024Cu
#define DS_TIMERCLOCK           0x024Eu  /* byte */
#define DS_PULSEWIDTH           0x024Fu
#define DS_OLD_INT8_OFFSET      0x0251u
#define DS_OLD_INT8_SEGMENT     0x0253u

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

/* Data tables (DS offsets). */
#define DS_CGA_MSG1             0x0195u
#define DS_CGA_MSG2             0x01D2u
#define DS_CGA_MSG3             0x0254u
#define DS_RLE_BITMAP           0x0DACu
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
