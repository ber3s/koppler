/* Hardwarevereinbarungen Abschaltrechner */

 #define Test          /*  */
/* #define Alt           /* wenn alte Platine */
 #define Ohne_Netzteil /*  */
/* #define M1          /*  */
/* #define M20        /*  */
 #define TC35        /*  */
 #define Quelle        /* protokolliert SMS und ANLAGE Ñnderung */

sbit  P1_0    = 0x90;
sbit  P1_1    = 0x91;
sbit  P1_2    = 0x92;
sbit  P1_3    = 0x93;
sbit  P1_4    = 0x94;
sbit  P1_5    = 0x95;
sbit  P1_6    = 0x96;
sbit  P1_7    = 0x97;

sbit  P3_0    = 0xB0;
sbit  P3_1    = 0xB1;
sbit  PFIint  = 0xB2; /* P3_2 ext Int. 0 */
sbit  P3_3    = 0xB3;
sbit  P3_4    = 0xB4;
sbit  P3_5    = 0xB5;
sbit  P3_6    = 0xB6;
sbit  P3_7    = 0xB7;

sbit  V220    = 0xE8; /* P4_0 */       /* 230V liegen an     */
sbit  TASTE   = 0xE9; /* P4_1 */       /* 0=Taste gedrÅckt   */
sbit  TIEF    = 0xEA; /* P4_2 */       /* 0=Ein Schaltet Relais fÅr Rechner */
sbit  RING    = 0xEB; /* P4_3 */       /* 0=Ein RING-Indikator    */
sbit  CD      = 0xEC; /* P4_4 */       /* 0=Ein Carrier Detect    */
sbit  OSZI    = 0xED; /* P4_5 */

#ifdef Alt
 sbit  P4_6    = 0xEE; /* P4_6 */       /* 0=Ein                   */
 sbit  RTS     = 0xEF; /* P4_7 */       /* 0=Ein                   */
#else
 sbit  RTS     = 0xEE; /* P4_6 */       /* 0=Ein                   */
 sbit  DTR     = 0xEF; /* P4_7 */       /* 0=Ein                   */
#endif

sbit  WDclk   = 0xF8; /* P5_0 */       /* Watchdog clock          */
sbit  LEDerr  = 0xF9; /* P5_1 */       /* Fehler   0=gelb         */
sbit  LEDpow  = 0xFA; /* P5_2 */       /* Betriebs 0=grÅn         */
sbit  POWER   = 0xFB; /* P5_3 */       /* Schaltet Relais 0=Ein   */
sbit  LEDfunk = 0xFC; /* P5_4 */       /* Funkmodem OK 0=rot      */
sbit  IGNITION = 0xFD; /* P5_5 */      /* Modemausgang 0=Ein      */
sbit  RS485   = 0xFE; /* P5_6 */       /* RS485 senden 0=Ein      */
sbit  CTS     = 0xFF; /* P5_7 */       /* CTS fÅr V24  0=Ein      */


#define ES1      1                    /* Bits im 80517 */
#define SMOD     0x80
#define Ein      0
#define Aus      1
#define RI1      1
#define TI1      2
#define ECT      8
#define CTF      8

#define XTAL     (long) 11059000      /* Quarzfrequenz MHz */

