/* Buendelfunksteuerung fuer SQ2 mit 80537 */

/* <tools.asm>            */
/* <getput.c>             */
/* <startup.asm>          */
/* <! NOOVERLAY PDATA(0)> */
/* <end>                  */


//#include <reg517.h>     /* 80515 Registersatz                */
//#include <80c517.h>
#include <stdio.h>      /* I/O Funktionen einbinden          */
#include <string.h>
//#include <hardware.h>   /* hier werden die Varianten definiert */
#include <task.h>
#include <cmath>
#include <intrinsics.h>

typedef union { char C[12];
                int  I[6];
                long L[3]; } Taskvar;
typedef struct Taskmem
  { unsigned int next;
    unsigned char mode,prio;
    unsigned int wait;
    unsigned int timer;
    unsigned char rld;
    Taskvar  var;
  } TTmem;

typedef struct Taskmem *PTmem;

#define Aus false
#define Ein true

char getkey() { return 0; }
void LEDfunk(bool On) { }
void LEDpow(bool On) { }
void LEDerr(bool On) { }


#define TIMER(NAME, STACKSIZE, dT)     \
  typedef struct {                \
  unsigned long mem[STACKSIZE];   \
  TTask task;                     \
 } T__ ## NAME;                   \
__no_init T__ ## NAME NAME;       \
inline void NAME ## _body();      \
 __arm void NAME ## _start() { NAME.task.mem=&NAME; NAME ## _body(); } \
inline void NAME ## _body() {  \
 taskinit(&NAME ## .task,0,NAME ## _start,0);
NAME ##
}

extern unsigned int  crc(unsigned int CRC, unsigned char X);
extern unsigned int  CCITT(unsigned int CCITT, unsigned char X);
extern unsigned char odd(unsigned char X);
extern unsigned char rdbit(unsigned char X);
extern unsigned char testbit(unsigned char X);
extern void          setbit(unsigned char X);
extern unsigned char getbit(unsigned char X);
extern void          freebit(unsigned char X);

extern bool          keypressed();
extern void          iniIO(char baud);
extern void          putinit();

#define dADU     0.11719       /* ADU_Auflîsung in V 0V..30V */
#define ADUAkku  0             /* Spannungswerte */
#define minAkku  99            /* 11,6V          */
#define AkkuOK  102            /* 12V            */

#define DelErrorTime  43200    /* Zeit/s bis ein Fehler gelîscht wird   */
#define maxcnt   7                    /* max Statuscnt */
#define ModSt    0                    /* Anfangszaehler fÅr Netzmode */
#define NetzReload  -5940             /* 10560 / 2 * XTAL / 9630400 */
#define StartReload 4*NetzReload-600  /* +722 */
#define NetzTimeOut 30                /* 300ms */

#define maxErrorZeit   50             /* maximale Zeit bis zum Abschalten */
#define maxFehlerCnt    4

#define Hauptlampen 0x47
#define Wiederholer 0xD8
#define maxHold     1800              /* 30 min Laufzeit ohne Spannung */

/* Bits in SEMAPHORE 8-bit mîglich */
#define DecErr   0x01                  /* 1 Tag um -> --FehlerCnt */
#define ErrMem   0x02                  /* Steuert den Zugriff auf Fehlermeldungen */
#define Synchr   0x04                  /* wird gesetzt bei Progr. Start */
#define bit1s    0x08                  /* gesetzt wenn neue sekunde im Netz */
#define incgreen 0x10                  /* gesetzt wenn gruen zaehlen */
#define newblk   0x20                  /* gesetzt wenn neuer Block */
#define secflg   0x40                  /* Sekundenflag fÅr SampleErr */

enum Betriebsarten
 { AUTO, MANUAL, BLINK, OFF,   SENSOR, Erot,  Erotbl, Egelb,  Eblink, off,
   gelb, blink,  rot,   rotbl, ROT,    ROTBL, GELB,   LERNEN, Fgelb,
   NoCnct, PowerOff, StromAus };

/* enum Lampenbits { x_R1, x_G1, x_E1, x_R2, x_E2, x_G2, x_Y1, x_Y2 }; */

enum ERR1bits { e1_F=0x01, e1_C=0x02, e1_k=0x04, e1_n=0x08,
                e1_S=0x10, e1_I=0x20, e1_x=0x40, e1_u=0x80 };
enum ERR0bits { e_u=0x01, e_C=0x02, e_L=0x04, e_O=0x08,
                e_P=0x10, e_N=0x20, e_U=0x40, e_A=0x80 };
enum Modebits { x_reset=0x80, x_rewrite=0x40, x_data=0x20, x_send=0x10,
                x_AA=0x02,    x_ass=0x01,
                x_listening =x_reset | x_ass,
                x_BlkOK     =x_reset | x_data | x_ass,
                x_sleeping  =x_ass,
                x_transmit  =x_rewrite | x_send | x_ass,
                x_binaer    =x_listening | 8 };

enum Errorbits  { x_unglt=0x80, x_LineOK=0x40, x_Vekt  =0x20, x_global=0x10,
                  x_Rqst =0x08, x_OK    =0x04, x_Switch=0x02, x_gruen =0x01,
                  x_220V   =0x8000,
                  x_24VE   =0x4000,
                  x_24VA   =0x2000,
                  x_Akku   =0x1000,
                  x_timeout=0x0800,
                  x_On     =0x0400,
                  x_Mode   =0x0200,
                  x_NoPrg  =0x0100  };

enum Frames   { f_listen,
                f_first=0x0F,
                f_SDM, f_stop, f_ack, f_status, f_reset, f_info,
                f_RIC, f_UIC, f_bin, f_ruf, f_AHY, f_xxx, f_send,
                f_last=0x1D };

#define x_fatal  (x_unglt+x_LineOK+x_gruen+x_220V+x_24VA)
/* #define x_fatal  (x_unglt+x_LineOK+x_gruen+x_timeout+x_On)  */
#define x_nosync (x_unglt+x_LineOK+x_gruen+x_timeout+x_On+x_OK+x_Mode \
                  +x_220V+x_24VA+x_24VE)
#define x_Power  (x_220V+x_24VA)
#define x_quarz  (x_Akku+x_NoPrg+x_Rqst+x_On+x_timeout+x_Mode)
#define x_quarzerr (x_NoPrg+x_On+x_timeout)
#define x_err1   (e1_S+e1_I)

typedef char *Pchr;

typedef struct { unsigned char status,count,OK;  } Status;

typedef struct { unsigned char X,cnt; } THysterese;

typedef THysterese *PHystRAM;
typedef struct { PHystRAM RAM; int Limit; unsigned char max; } CHysterese;
typedef const CHysterese *PHyst;

typedef char TimeStr[13];
typedef char TSMSStr[180];
typedef char TelefonNr[80];

typedef union {
 unsigned char R[100];
 struct { unsigned char ST,kmd,STe; } KMD;
 struct { unsigned char ST;
          unsigned char kmd[3];
          unsigned char PFH,PFL,ID1,ID2,ID3,ID4;
          unsigned char STe;                     } ACKT;
 struct { unsigned char ST,Len,PFH,PFL,ID1,ID2,ID3,ID4;
          unsigned char buffer[100];
          unsigned char STe;                     } SDM; } TTele;
typedef TTele *PTele;


enum Telefonbits
        { t_isdn=0x1, t_ruf=0x2, t_cnct=0x4, t_sms=0x8, t_email=0x10 };


typedef struct { TelefonNr     Nr;
                 unsigned char Typ;
                 unsigned char Event;
                 unsigned char Trial,Cnt,D; } TTelefon;

typedef struct { unsigned char Mode;
                 unsigned char Anz;
                 char          pnt;
                 unsigned int  CRC;
                 TTele         Blk; } TeleKanal;

typedef union {
 struct { unsigned char Adr,Abs,Lng,Nr,Typ;
          unsigned char ActSt,ERR0,ERR1,LONERR,LOFERR;
          unsigned char sec,min,std,tag,mon,year;
          unsigned int  akku,ptime;
          unsigned char Lamps,xx; }           i;
 struct { unsigned char Adr,Abs,Lng,Nr,Typ;
          unsigned char ERR0,ERR1,Lamp,LONERR,ActSt,Iok;
          unsigned int  PhZt;        }        E;
 struct { unsigned char Adr,Abs,Lng,Nr,Typ;
          unsigned char secL,secH,minL,minH,stdL,stdH,dayL,dayH,monL,monH,
                        yearL,yearH; }        T;
 struct { unsigned char Adr,Abs,Lng,Nr,Typ;
          unsigned char Anz,CHWC,SWSTi;
          unsigned int  Sensor;
          unsigned char VktFlg,Frei,PhZt,Dest,ActSt;
          unsigned long Time;               } N;
 struct { unsigned char Adr,Abs,Lng,Nr,Typ;
          unsigned char buffer[32];         } P; } Block;

typedef Block *PBlock;

typedef struct { unsigned char Mode;
                 char          pnt;
                 unsigned int  CRC;
                 Block         Blk; } Kanal;

typedef struct { unsigned char day,std,min,sec,dsec; } Ttime;
typedef struct { unsigned char X; unsigned int CRC;  } TvarCRC;

typedef struct { Block  Blk;
                 unsigned char St[18]; } NtzBlk;


/****************************************************************/
/** BIT-Bereich                                            ******/
/****************************************************************/

bool Modem;       /* Modem=1 BÅndelfunk=0  */
bool Quarz=1;

/****************************************************************/
/** DATA-Bereich                                           ******/
/****************************************************************/

unsigned char CHWC;
unsigned int  Umlauf;
unsigned int  SMSDelay=0;
unsigned int  RufDelay=0;
unsigned int  CnctDelay=0;
unsigned char Fang=0;
unsigned int  Green;        /* GrÅnflag                    */
unsigned int  xon;          /* Gruen ohne OK               */

/****************************************************************/
/** PDATA-Bereich                                          ******/
/****************************************************************/

Kanal          rdwr;        /* Empfangskanal Netz */
Status         SQstat[18];

struct S_Netz {
          char Nr;
 unsigned char Mode;
 unsigned int  Kabel;
 unsigned int  Err;
 unsigned int  TimeOut;
 Block    *Dest;
 unsigned char Or,And,Anzahl;
                       } SQ2;

unsigned long Uhr;          /* interne Uhr */
unsigned long LastErrZeit;  /* Zeitpunkt des letzten Fehlers */
TimeStr       UhrZeit;      /* Uhrzeit vom T-Block "25.10. 13:25" */

THysterese MessCnt[7]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};

unsigned int  ErrorZeit;    /* Zeit in welcher ein Fehler anliegt */
unsigned int  NewErr;       /* beinhaltet Neue Fehler */

unsigned char LonERR[17];
unsigned char LoffERR[17];
unsigned char ERR0[17];
unsigned char ERR1[17];
unsigned char Line[17];     /* enthaelt die Bits von LineOK */
unsigned char Frei[17];     /* GrÅnzeitzÑhler               */

/****************************************************************/
/** XDATA-Bereich                                          ******/
/****************************************************************/

Block  Netz,Fehler,Kommando,Daten,Lernen,Global,Micro,Info;


#define MaxIBlk  180
TvarCRC        Reset;       /* gesetzt, wenn Anlage Reset */
TvarCRC        FehlerCnt;   /* ZÑhlt die Fehler, wenn mehr als maxErrCnt
                                     dann Anlage aus.
                                     Ruecksetzen, wenn 1 Tag vorrueber oder
                                     mit Taste */
unsigned char ie_gap;
unsigned char  EblockCnt[17]={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
                                  /* Timeout fÅr E-Blîcke  */
unsigned char  iblockCnt[17]={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
                                  /* Timeout fÅr i-Blîcke  */
unsigned char  iErrorCnt[17]={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
                                  /* Timeout fÅr i-Blîcke mit Fehler */
unsigned int   Akku[17];     /* Akkuspannung          */
unsigned int   Ptime[17];    /* Restbetriebsdauer     */
unsigned char  Lamps[17];    /* LampenbestÅckung      */
unsigned char  ActSt[17];    /* Aktuelle Betriebsart  */

char          Receiver[241];   /* ="01711456763"; */
unsigned int  CRCReceiver;

char          AmpelID[16];    /* ="TESTANLAGE"; */
unsigned int  CRCAmpelID;

// char          PIN[9];         /* ="9307"; */
// unsigned int  CRCPIN;         /*   */

unsigned int  LEDcnt,TELEcnt,MAINcnt;
unsigned int  UAkku;
TSMSStr       History[16];
char          HPnt;        /* Zeiger auf History */

TTelefon      Tel[4];      /* Drei TelefonNumern */
char               *OA;         /* SMS-Absender       */
char               *SCTS;       /* SMS-Zeit           */
char               *UD;         /* SMS-Nachricht      */
unsigned int  Qfunk,Efunk; /* Qualitaet Funkverbindung */
unsigned int  HoldTime;    /* Betriebszeit am Akku */

unsigned char  DNr;        /* DNetz-Nummer */
char  GSM[4000];
unsigned int   SMSlng;     /* LÑnge einer SMS Nachricht */

NtzBlk    Notizen[2][32];   /* Netzprotokoll */
unsigned char  NtzI=1;
unsigned char  NtzJ[2]={0,0};

/****************************************************************/
/** CODE-Bereich                                           ******/
/****************************************************************/

const unsigned int bittab[]=
 {0x0000,
  0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
  0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000};



const char   Adressen[]=
 {0xFF,  0xF3,    0xF0,      0xF1,   0xD0,    0x00,    0x01,   0x55 };


const PBlock  Bloecke[]=
 {&Netz, &Fehler, &Kommando, &Daten, &Lernen, &Global, &Micro, &Info, NULL};

const CHysterese Taste ={&MessCnt[0],1,10};
const CHysterese U_AkkuE={&MessCnt[2],AkkuOK,100};
const CHysterese U_Akku={&MessCnt[3],minAkku,100};
const CHysterese C_OKerr={&MessCnt[6],1,150};

const char *Lampenfehler[]={ " Ro1", " Gr1", " Re1", " Ro2",
                            " Re2", " Gr2", " Ge1", " Ge2" };
                     /* 01   02   04   08   10   20   40   80 */
const char *ERR0str[]={ "u", "C", "L", "O", "P",   0, "U", "A" };
const char *ERR1str[]={ "F", "#", "!", "%", "S", "+",   0, "=" };
const char *SQstrEx[]= { "*",    /* Gruen an         */
                        "S",    /* Schalterkonflikt */
                        "!",    /* nicht OK         */
                        "r",    /* Request          */
                        "g",    /* globaler Block   */
                        "v",    /* Vektor           */
                        "N",    /* Line OK          */
                        "-",    /* ung¸ltig         */

                        "P",    /* kein Programm    */
                        "M",    /* Mode Fehler      */
                        "#",    /* Lampe kˆnnte An sein A C O */
                        "T",    /* Timeout          */
                        "A",    /* Akku             */
                        "1",    /* 24V Ausgang      */
                        "2",    /* 24V Eingang      */
                        "3" };  /* 220V             */

const char *SQstr[]={ " Signal", " Schalter", "",       "",
                     "",        "",          " Kabel", " Kabel",
                     "",        "",          " On",    " Timeout",
                     "",        "",          "",       "" };
const char *Modestr[]={ "AUTOMATIK", "MANUELL",   "BLINKEN", "AUS",
                       "KNOTEN",    "rot",       "rotbl",   "gelb",
                       "blinken",   "aus",       "gelb",    "blinken",
                       "rot",       "rotbl",     "ROT",     "ROTBL",
                       "GELB",      "LERNEN",    "gelb",    "fehler",
                       "Abgesch.",  "Abgesch.",  "???",     "???",
                       "???",       "???",       "???",     "???",
                       "???",       "???",       "???",     "???"  };
const char ModeTyp[]=  { 0,           0,           0,         0,
                        0,           1,           1,         1,
                        1,           2,           2,         2,
                        2,           2,           0,         0,
                        0,           0,           3,         1,
                        1,           1,           1,         1,
                        1,           1,           1,         1,
                        1,           1,           1,         1 };

unsigned char const *CLREOL="\x1B[0K";

#ifdef M20
unsigned char const *Modemname="M20";
#endif
#ifdef TC35
unsigned char const *Modemname="TC35";
#endif
#ifdef M1
unsigned char const *Modemname=": M1";
#endif


void deciblk(int i)
{ __disable_interrupt(); --iblockCnt[i]; __enable_interrupt();  }


void incUhr()
/*********************************************************************/
{ __disable_interrupt(); ++Uhr; __enable_interrupt(); }

void decCHWC()
/*********************************************************************/
{ __disable_interrupt(); --CHWC; __enable_interrupt(); }

void incx(unsigned int *X)
/*********************************************************************/
{ __disable_interrupt(); if (*X<0xFFFE) *X+=1; __enable_interrupt(); }

unsigned int rdx(unsigned int *X)
/*********************************************************************/
{ volatile unsigned int x;
  __disable_interrupt(); x=*X; __enable_interrupt(); return (*X); }

void wrx(unsigned int *X,unsigned int D)
/*********************************************************************/
{ __disable_interrupt(); *X=D; __enable_interrupt(); }

void IncNtz()
/*********************************************************************/
{ __disable_interrupt(); Fang=10; __enable_interrupt(); }

char *rdstrCRC(char *Buff, unsigned int CRC)
/*********************************************************************/
{ unsigned int i,X;
  if (strlen(Buff)==0) return NULL;
  else
   { X=0;
     for (i=0; i<strlen(Buff); ++i)
     X=crc(X,Buff[i]);
     if (CRC==X) return Buff; else return NULL;
   };
}

void cpystrCRC(char *Dest,char *Buff, unsigned int *CRC, unsigned int n)
/*********************************************************************/
{ unsigned int i;
  *CRC=0;
  for (i=0; ( (i<strlen(Buff)) && (i<n) ); ++i) *CRC=crc(*CRC,Buff[i]);
  strncpy(Dest,Buff,n); Dest[n]=0;
}

unsigned char rdCRC(TvarCRC  *X)
/*********************************************************************/
{ if (X->CRC==crc(0,X->X)) return (X->X); else return (0x00); }

void wrCRC(TvarCRC  *X,unsigned char C)
/*********************************************************************/
{ X->CRC=crc(0,C); X->X=C; }

void Messung(PHyst P,unsigned int Value)
/*********************************************************************/
{ if (Value>=P->Limit)
   { if (P->RAM->cnt<P->max) ++P->RAM->cnt; else P->RAM->X=1; }
  else
   { if (P->RAM->cnt) --P->RAM->cnt; else P->RAM->X=0; }
}

void Halten(PHyst P,unsigned int Value)
/*********************************************************************/
{ if (Value>=P->Limit)
   { if (P->RAM->cnt<P->max) ++P->RAM->cnt; else P->RAM->X=1; }
  else
   { P->RAM->cnt=0; P->RAM->X=0; }
}

void wrM(PHyst P,unsigned char X) /* nutzt nur Register -> reentrant */
/*********************************************************************/
{ if (X) { P->RAM->cnt=P->max; P->RAM->X=1; }
  else { P->RAM->cnt=0; P->RAM->X=0; }
}

unsigned char rdM(PHyst P)        /* nutzt nur Register -> reentrant */
/*********************************************************************/
{ return (P->RAM->X); }

unsigned int swap(unsigned int X)
/*********************************************************************/
{ return ( ((X&0xFF)<<8) | ((X&0xFF00)>>8) );  }

unsigned int BCDtoI(unsigned int X)
/*********************************************************************/
{ return (((X&0xF000)/0x1000)*1000+((X&0xF00)/0x100)*100+((X&0xF0)/0x10)*10+(X&0xF)); }

/* IntNetz und IntRS485 treten nie gemeinsam auf */

void syncNetz(unsigned int Rel)
/*********************************************************************/
/* Netztakt synchronisieren
   Netz SQ2 = 1,07421875ms = 9830400/8/24/55
   Reload=10560/2*XTAL/9830400=5940
   Der erste Rolaod muss 5940-2890 sein, weil die N-Block Erkennung 479mics
   dauert.
   fq/2 5529500Hz=180,85ns (minimale Auflîsung)
*/

{ /*
  CTCON=0x0;
  CTREL=Rel;
  IEN2 &= ~ES1;  // Empfangsint. aus
  CTCON&=~CTF;   // Interruptflag loeschen
  IEN2 |= ECT;   // Interrupt fÅr CT freigeben
  */
  SQ2.Nr=0;
}

void wrstatus(unsigned char nr)
/*********************************************************************/
{ /*
  unsigned char key;
  Status *Pnt;

  Pnt=&SQstat[nr];

  if (S1CON & RI1)
   { key=S1BUF;
     if (odd(key)) key&=0x7F; else key=Pnt->status|x_unglt;
     S1CON &= ~RI1;
   }
  else key=x_unglt;   // Pnt->status|x_unglt;

  Notizen[NtzI][NtzJ[NtzI]].St[nr]=key;

  if (Pnt->status==key)
   { if (Pnt->count < maxcnt) ++Pnt->count; }
  else Pnt->count=0;

  if (key&x_OK)
   { if (Pnt->OK < 3) ++Pnt->OK; }
  else
   { if (Pnt->OK) --Pnt->OK; };

  if (key&x_unglt)   // 80 wird gesetzt, wenn 2x unglt
   { if (Line[nr]<2) ++Line[nr]; else Line[nr]|=0x80;
   }
  else Line[nr]=0;

  Pnt->status=key; */
}

void IntNetz() // interrupt 19 using 1
/*********************************************************************/
{ /*
  char Nr;
  OSZI=Ein;
  if (SQ2.Nr==0) syncNetz(NetzReload);
  CTCON&=~CTF;

  Nr=SQ2.Nr; if (Nr >= 0) wrstatus(Nr);

  if (Nr <= SQ2.Anzahl) ++SQ2.Nr;
  else {
   IEN2 |= ES1; IEN2 &= ~ECT;   // Empfangsinterrupt wieder ein
   SQ2.TimeOut=0;
   if (Fang) { if (! --Fang) ++NtzI; };
  };
  OSZI=Aus;
 */
}

void nextchar(unsigned char C) /* Zeichen im Block abspeichern */
/*********************************************************************/
  {  /*
     char Nr; unsigned char i;
     Nr=rdwr.pnt;
     NtzI&=1; NtzJ[NtzI]=(NtzJ[NtzI])&0x1F;
     if (rdwr.Mode==x_listening) {
     switch (Nr) {
      case -5: { i=0;
                 while ( (i<sizeof(Adressen)) && (Adressen[i]!=C)) ++i;
                 if (Adressen[i]==C)
                  { rdwr.Blk.P.Adr=C; SQ2.Dest=Bloecke[i]; }
                 else
                  { rdwr.Mode=x_sleeping; SQ2.Dest=NULL; }
               }; break;
      case -4: rdwr.Blk.P.Abs=C; Notizen[NtzI][NtzJ[NtzI]].Blk.P.Abs=C; break;
      case -3: if (C<sizeof(rdwr.Blk.P.buffer)) {
                rdwr.Blk.P.Lng=C; Notizen[NtzI][NtzJ[NtzI]].Blk.P.Lng=C;
               } else { rdwr.Mode=x_sleeping; SQ2.Dest=NULL; };
               break;
      case -2: rdwr.Blk.P.Nr =C; Notizen[NtzI][NtzJ[NtzI]].Blk.P.Nr=C; break;
      case -1: rdwr.Blk.P.Typ=C; Notizen[NtzI][NtzJ[NtzI]].Blk.P.Typ=C; break;
      default: { if (Nr<rdwr.Blk.P.Lng)
                  { rdwr.Blk.P.buffer[Nr]=C;
                    Notizen[NtzI][NtzJ[NtzI]].Blk.P.buffer[Nr]=C;
                  }
                else if (Nr==rdwr.Blk.P.Lng) rdwr.CRC^=(((unsigned int)C)<<8);
                else if (Nr==rdwr.Blk.P.Lng+1)
                 { rdwr.CRC^=C;
                   if   (rdwr.CRC) rdwr.Mode=x_sleeping;
                   else
                    { if ((rdwr.Blk.P.Adr==0xFF)&&(rdwr.Blk.N.Typ=='N'))
                       { syncNetz(StartReload);
                         if (rdwr.Blk.N.Anz<17) SQ2.Anzahl=rdwr.Blk.N.Anz;
                          else SQ2.Anzahl=16;
                         if ((CHWC>85)||(CHWC<55)) CHWC=rdwr.Blk.N.CHWC;
                       };
                      rdwr.Mode=x_BlkOK;
                      if (SQ2.Dest->P.Typ==0) *SQ2.Dest=rdwr.Blk;
                      NtzI&=1; NtzJ[NtzI]=(NtzJ[NtzI]+1)&0x1F;
                    };
                 };
              };
     };
     if (Nr<rdwr.Blk.P.Lng) rdwr.CRC=crc(rdwr.CRC,C);
     ++rdwr.pnt;
     };
     */
  }

/*
void IntRS485() interrupt 16 using 1
// *********************************************************************
{ unsigned char C,M;

  S1CON &= ~RI1;

  C=S1BUF; M=rdwr.Mode;
  if (C==0xAA) {
    if ((M & x_AA)==0) rdwr.Mode|=x_AA;
    else { rdwr.Mode&=~x_AA; nextchar(C); };
   }
  else {
    if (M & x_AA) {  // C ist Adresse
     rdwr.pnt=-5; rdwr.Mode=x_listening; rdwr.CRC=0; nextchar(C);
     }
    else nextchar(C);
  };
}
*/

// #pragma REGISTERBANK (0)

void clrBloecke()
/*********************************************************************/
{ char i;
  for (i=0; Bloecke[i]!=NULL; ++i) Bloecke[i]->E.Typ=0;
}

void clrlinecnt()
/*********************************************************************/
{ char i;
  for (i=0; i<17; ++i) Line[i]=0; }

void iniRS485()
/*********************************************************************/
/* Initialisierung serielle Schnittstelle fÅr XTAL=11059000Hz        */
/* Schnittstelle 1 RS485                                             */
{
//  RS485=Aus;
// S1CON=0x90;
//  S1REL=238;             /* 19,2 kbaud       */
  clrlinecnt();
//  IEN2|=ES1;             /* Empfangsint. ein */

  SQ2.Anzahl=1; SQ2.Mode=0xFF; SQ2.Err=x_Mode|x_OK; Umlauf=0;
  rdwr.Mode=x_ass;
  SQstat[1].status=x_unglt;
}

unsigned char adc(unsigned char channel)
/*********************************************************************/
 { /*
   ADCON1 = (ADCON1 & 0xF0) | (channel & 0x0F);  // Channel Åbergeben
   DAPR=0x00;                                    // Wandlung starten
   while(BSY);                                   // solange Wandlung lÑuft..
   return ADDAT;                                 // Ergebnis zurÅckliefern
   */ return 0;
 }                                               // Ende adconverter


/*
  TASK(UHR,70)
  wrM(&U_Akku,0); wrM(&U_AkkuE,0);

___ ENDTASK
*/

char runUhr(PTmem mem)
/* laeuft als Warteroutine im Uhrentask */
/*********************************************************************/
/* Zeitbasis  1,0000181ms  Entprellen von Eingaengen */
{ static unsigned int C1s,C10;
  putinit();
  if (mem->timer==0) { ++mem->timer; C1s=0; C10=0; };
//  C1s+=EBScnt; C10+=EBScnt;

  while (C1s>=1000)
    { incUhr(); C1s-=1000;
      if (SMSDelay) --SMSDelay;
      if (RufDelay) --RufDelay;
      if (CnctDelay) --CnctDelay;
      if (!(SQ2.Err&x_Mode))
       { ++LastErrZeit;
         if (LastErrZeit>DelErrorTime)
          { LastErrZeit=0; setbit(DecErr); }
       }
      else LastErrZeit=0;
      setbit(secflg);
    };

   while (C10>=10)
    { C10-=10;
      UAkku=adc(ADUAkku);

      Messung(&C_OKerr,(SQ2.And&x_OK) && (SQstat[SQ2.Anzahl+1].count==maxcnt)
                         && (SQstat[SQ2.Anzahl+1].status==0x80));


//      Messung(&Taste,!TASTE);

      Messung(&U_Akku,UAkku);
      Messung(&U_AkkuE,UAkku);
      incx(&ErrorZeit);
      incx(&SQ2.TimeOut);
      if (CHWC) decCHWC(); else CHWC=99;
      if (CHWC==70)
       { setbit(incgreen); Umlauf=(int)(Netz.N.PhZt+1); }

    };


   return 0; //M_wait;
}


/*
TASK(LEDP,70)
// *********************************************************************
LEDpow=Ein;
___
 sys(K_rld,200);
 while (1) {
  sys(K_time,0);
  if (!ModeTyp[SQ2.Mode]) LEDpow=Ein; else LEDpow=!LEDpow;
 };
ENDTASK

TASK(LED,120)
// *********************************************************************
char i;
unsigned int E,gap;
unsigned int Toggle;
LEDerr=Aus;
___
 sys(K_rld,500);
 while (1) {
  E=0; sys(K_time,0);  if (Toggle) Toggle=0; else Toggle=1;

  if (Toggle) {
   gap=1; E=0;
   for (i=1; i<=SQ2.Anzahl; ++i) {
    if (LoffERR[i] || (ERR0[i]&e_U)) E=1;

    if (iErrorCnt[i]) --iErrorCnt[i];

    if (iblockCnt[i]) {
     deciblk(i); gap=0;
    } else {
     Akku[i]=0xFFFF; Ptime[i]=0xFFFF;
    };
   };
   ie_gap=gap;
  };

  if (SQ2.Err&x_Mode) LEDerr=Ein;
  else if (SQ2.Err || E) LEDerr=!LEDerr;
  else LEDerr=Aus;

 };

ENDTASK
*/

void StopSystem()
/*********************************************************************/
{ //wrCRC(&FehlerCnt,0); Reset.X=0; LEDpow=Aus;
}


unsigned char AllOK()
/*********************************************************************/
{ if ((SQ2.And&x_OK) && (SQstat[SQ2.Anzahl+1].count==maxcnt)
      && (SQstat[SQ2.Anzahl+1].status==0x80))
   return 1; else return 0;
}

char waitmem()
/*********************************************************************/
/* wartet auf die Freigabe des Fehlerbereichs                        */
{ if (getbit(ErrMem)) return 1; else return 2; }// return(M_rdy); else return(M_wait); }


unsigned int orcmp(unsigned char *Dest,unsigned char X)
/*********************************************************************/
{ unsigned int result;
  result=((*Dest^X)&X); *Dest|=X; return result;
}

unsigned int setcmp(unsigned char *Dest,unsigned char X)
/*********************************************************************/
{ unsigned int result;
  result=((*Dest^X)&X); *Dest=X; return result;
}


void clrerr()
{ char i;
  for (i=0; i<17; ++i) {
   Frei[i]=0; Line[i]=0; ERR0[i]=0; LoffERR[i]=0; ERR1[i]=0;
  };
}

void SampleErrors()
/*********************************************************************/
/* Setzt SQ2.Err und macht die Anlage scharf                         */
/* NewErr ist gesetzt, wenn die Anlage von Normal in Fehler geht     */
{ unsigned int  E,Kabel,b;
  unsigned int  X1;
  unsigned char i,OKx,X,CntX,Res,Mode;
  unsigned char Flag,isOK;

  E=0; Kabel=0; xon=0;

  if ( SQ2.TimeOut>NetzTimeOut ) {
   for (i=0; i<17; ++i) {
    SQstat[i].status=x_unglt; SQstat[i].count=0; SQstat[i].OK=0; Line[i]=0;
   };
   Mode=NoCnct; Kabel=1; Line[1]=0x85;
   E|=x_unglt;
   OKx=0; isOK=0;
  } else {
   Mode=Netz.N.ActSt&0x1F;
   if (rdbit(incgreen)) Flag=1; else Flag=0;
   SQ2.And=0xFF; SQ2.Or=0;
   for (i=1,b=1; i<=SQ2.Anzahl; ++i,b<<=1) {
    X=SQstat[i].status;  SQ2.And&=X;  SQ2.Or|=X;
    if (Line[i]&0x80) Kabel|=b;
    if ( (X&(x_gruen | x_OK))==x_gruen ) xon|=b;
    if ( Flag ) {
     if (X&x_gruen) {
      if (Green&b) {
       if (Frei[i]<255) ++Frei[i];
      } else {
       Frei[i]=1; Green|=b;
      };
     } else Green&=~b;
     if (EblockCnt[i]>0) --EblockCnt[i];
     else {
      if (EblockCnt[i]==0) {
       LoffERR[i]=0; LonERR[i]=0; ERR0[i]=0; ERR1[i]=0; };
     };
    };
   };
   if (!(SQ2.And&x_LineOK)) E|=x_unglt;
   OKx=AllOK(); isOK=rdM(&C_OKerr);
  };

  if (Quarz) Mode=SQ2.Mode;

  switch (Netz.N.Typ) {
   case 0:   break;
   case 'N': Netz.N.Typ=0;
             Mode=Netz.N.ActSt&0x1F;
             if ( (!(E&(x_unglt))) && (SQ2.Anzahl>1) ) Quarz=0;
             iblockCnt[1]=MaxIBlk;
             break;
   default:  Netz.N.Typ=0;
  };

  switch (Global.P.Typ) {
   case 0: break;
   case 'P': if (Global.E.Abs<17) i=Global.E.Abs; else i=1;
             EblockCnt[i]=30;                      /* 30s GÅltigkeit */
             Global.P.Typ=0; ERR0[i]|=e_P; break;
   case 'T': strcpy(UhrZeit,"00.00. 00:00");
             UhrZeit[0]|=Global.T.dayH; UhrZeit[1]|=Global.T.dayL;
             UhrZeit[3]|=Global.T.monH; UhrZeit[4]|=Global.T.monL;
             UhrZeit[7]|=Global.T.stdH; UhrZeit[8]|=Global.T.stdL;
             UhrZeit[10]|=Global.T.minH; UhrZeit[11]|=Global.T.minL;
             Global.P.Typ=0; break;
   default:  Global.P.Typ=0;
  };


  Res=rdCRC(&Reset);

  if (SQ2.Err&x_Akku) { if (!rdM(&U_AkkuE)) E|=x_Akku; }
  else { if (!rdM(&U_Akku)) E|=x_Akku; };

  if (!OKx) { E|=x_OK; if (SQ2.Or&x_gruen) E|=x_gruen; };

  if (ModeTyp[Mode]) E|=x_Mode;

  if (!Quarz) {
   if ( !(E&x_fatal) || Res ) wrx(&ErrorZeit,0);
   else if ( rdx(&ErrorZeit)>maxErrorZeit ) E|=x_timeout;
  };

  CntX=rdCRC(&FehlerCnt);

  if (rdbit(DecErr)) { if (CntX>1) wrCRC(&FehlerCnt,CntX-1); };

  for (i=1; i<=SQ2.Anzahl; ++i)
   { if (ERR0[i] & (e_u | e_C | e_O | e_A | e_L)) E|=x_On;   /* A---O-C- */
   };

 // sys(1,waitmem);

   switch (Info.i.Typ) {
    case 0: break;
    case 'i':
     i=Info.i.Abs;
     if ((SQ2.Anzahl<i) && (i<10)) SQ2.Anzahl=i;
     if (Info.i.Nr==1) {
      if (Quarz) {
       Mode=Info.i.ActSt&0x1F;
       if (!ModeTyp[Mode]) {
        iErrorCnt[i]=0; E&=~x_Mode;
       } else E|=x_Mode;
      };
      EblockCnt[i]=-1;  /* unendliche G¸ltigkeit */
      iblockCnt[i]=MaxIBlk;
      Akku[i]=Info.i.akku; Ptime[i]=Info.i.ptime;
      if (!Res) {
       if (iErrorCnt[i]) {
        X1=orcmp(&LonERR[i],Info.i.LONERR)
           | orcmp(&LoffERR[i],Info.i.LOFERR)
           | (orcmp(&ERR0[i],Info.i.ERR0)&e_U);
       } else {
        X1=setcmp(&LonERR[i],Info.i.LONERR)
           | setcmp(&LoffERR[i],Info.i.LOFERR)
           | (setcmp(&ERR0[i],Info.i.ERR0)&e_U);
       };
       if (X1) { E|=x_Rqst; iErrorCnt[i]=MaxIBlk; };
       ERR1[i]=Info.i.ERR1;
      };
     };
     Info.i.Typ=0;
    break;
    default: Info.i.Typ=0;
   };

   switch (Fehler.E.Typ) {
    case 0: break;
    case 'E':
      i=Fehler.E.Abs;
      EblockCnt[i]=30;  /* 30s GÅltigkeit */
      iblockCnt[i]=MaxIBlk;
      if (!Res) {
       X1=orcmp(&LonERR[i],Fehler.E.LONERR)
          | orcmp(&LoffERR[i],Fehler.E.Lamp)
          | (orcmp(&ERR0[i],Fehler.E.ERR0)&e_U);
       if (X1) { E|=x_Rqst; };
       ERR1[i]|=Fehler.E.ERR1;
      };
      Fehler.E.Typ=0;
     break;
    default: Fehler.E.Typ=0;
   };

   if (Quarz) {
    E&=x_quarz;
    if (ie_gap) E|=x_timeout;
    if (E&x_quarzerr) { Mode=19; E|=x_Mode; };
   };

   if ( (SQ2.Err^E) & E & x_Akku ) NewErr|=E;

   if (!(E&x_Mode)) {
    if (SQ2.Err&x_Mode) clrerr();
    if (Res) {
     wrCRC(&Reset,0); wrCRC(&FehlerCnt,1); Res=0;
     setbit(Synchr);
    };
   };

   if (!Res) {
    if ( (SQ2.Err^E) & E & x_Mode ) { NewErr|=E; SQ2.Kabel|=Kabel; };
    if (E&x_Rqst) NewErr|=E;
   };

   wrx(&SQ2.Err,E&~x_Rqst);
   SQ2.Mode=Mode;

  freebit(ErrMem);

  if (Flag) setbit(bit1s);

}


unsigned char upcase(unsigned char c)
/*********************************************************************/
{ if ((c>='a') && (c<='z')) return(c & 0xDF); else return c; }


char *iskmd(char  *buff,char *muster)
/*********************************************************************/
/* Zeigt auf erstes Zeichen nach muster in buff oder NULL wenn nicht */
/* gefunden                                                          */
/* *buff muss ein Zeiger nach                                   */

{ char *MU;
  char  *BU;
  bool OK;
  char first;

  first=upcase(muster[0]); OK=0;
  while (*buff && !OK)
   { if (upcase(*buff)==first)
      { OK=1; MU=muster+1; BU=buff+1;
        while (OK && *MU)
         { if (upcase(*MU)!=upcase(*BU)) OK=0;
           ++MU; ++BU; };
      }
     if (!OK) ++buff;
   };
  if (OK) return (buff+strlen(muster)); else return NULL;
}


char *StatStr(unsigned char St)
/*********************************************************************/
{ static char buff[30];
  if (St & x_OK) strcat(buff," OK");    else strcat(buff,"   ");
  if (St & x_Switch) strcat(buff," M"); else strcat(buff,"  ");
  if (St & 8) strcat(buff,"+");         else strcat(buff," ");
  return buff;
}

void calcstr(char *buff,unsigned int X,const char *List[])
/*********************************************************************/
{ unsigned char i;
  unsigned int  b;
  for (i=0,b=1; i<16; ++i,b<<=1 ) {
   if ((b & X) && (List[i]!=0)) strcat(buff,List[i]);
  };
}

void hex(char *buff,unsigned int X)
/*********************************************************************/
{ char Ch;
  Ch=(X&0xF0)>>4; if (Ch>9) Ch=Ch+7; buff[0]=Ch+0x30;
  Ch=X&0xF; if (Ch>9) Ch=Ch+7; buff[1]=Ch+0x30;
  buff[2]=0;
}

char *calcSMS(char *buff,char *SMS,char *Receiver)
/*********************************************************************/
{ char temp[20];
  unsigned char i,Chx,Ch;
  unsigned int X,n;

  strcpy(SMS,"1100");   /* zu sendende Nachricht, vier Tage GÅltigkeit */
  n=strlen(Receiver);
  hex(temp,n); strcat(SMS,temp);                /* LÑnge Nummer */
  strcat(SMS,"81");                             /* Inland       */
  for (i=0; i<n; i=i+2)                         /* Nummer       */
   { temp[i+1]=Receiver[i];
     if ((i+1)>=n) temp[i]='F'; else temp[i]=Receiver[i+1];
   };

  temp[i]=0; strcat(SMS,temp);
  strcat(SMS,"0000AA");                 /* PID DCS=7-Bit VP      */
  n=strlen(buff);
  hex(temp,n); strcat(SMS,temp);        /* UDL                   */

  for (i=0; i<n; ++i)
   { Ch=buff[i]&0x7F;
     if ((i+1)==n) Chx=0; else Chx=buff[i+1]&0x7F;
     if ((i&7)!=7)
      { X=( Ch>>(i&7) )|( Chx<<(7-(i&7)) );
        hex(temp,X); strcat(SMS,temp); };
   }
  n=strlen(SMS); SMS[n]=0x1A; SMS[n+1]=0;  /* ^Z */
  return SMS;
}


void wait_empty()
/*********************************************************************/
{ /*
  int time;
  time=0;
  sys(K_rld,100);
   while (time<30)
    { time+=sys(K_rdflg,1);
      while (keypressed()) { getkey(); time=0; };
      sys(K_time,0);
    };
  sys(K_rld,0);
  */
}

char *line(char kmd)
/*********************************************************************/
/* kmd=0 gibt letztes Kmd zur¸ck, wenn NULL Tastaturabfrage          */
/* kmd=1 lˆschen des Kmd                                             */
/* kmd=2 lˆschen wenn kmd vorhanden                                  */
/* Kmd=3 h‰ngt die n‰chste Zeile an                                  */
/* Kmd=4 gibt momentanen Inhalt von InStr                            */
/* Kmd=5 vorher Kmd=0 aufrufen, gibt Zeiger auf neue Zeichen oder    */
/*       Null wenn keine da                                          */
/*********************************************************************/
{ static char InStr[500];
  static char Ch;
  static char *Flag,*X;
  int i;
/*  char *chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890 =,:+-.@<>?\'\"/"; */

  switch (kmd) {
   case 1:  InStr[0]=0; Ch=0; break;
   case 2:  if (Ch=='\r') { InStr[0]=0; Ch=0; }; break;
   case 3:  i=strlen(InStr);
            if (i<(sizeof(InStr)-1))
             { InStr[i]='\r'; InStr[i+1]=0; Ch=0; }; break;
   case 4:  return InStr; //break;
   case 5:  if (Flag!=InStr+strlen(InStr))
             { X=Flag; Flag=InStr+strlen(InStr); }
            else X=NULL;
            return X; //break;
   default: { i=strlen(InStr);
              while (keypressed() && (Ch!='\r'))
               { Ch=getkey();
                 switch (Ch) {
                  case '\r': case '\n':
                   if (i==0) Ch=0; break;
                  case '\b': if (i) { --i; InStr[i]=0; }; break;
                  case ' ':
                   { if ( i<(sizeof(InStr)-1) )
                      { InStr[i]=Ch; ++i; InStr[i]=0; }
                     else { InStr[0]=0; i=0; };
                     if (strcmp(InStr,"> ")==0) Ch='\r';
                   }; break;
                  default:
                   { if ( (Ch>=' ') && (Ch<=127) )
                      { if ( i<(sizeof(InStr)-1) )
                         { InStr[i]=Ch; ++i; InStr[i]=0; }
                        else { InStr[0]=0; Ch=0; i=0; }
                      }
                     else Ch=0;
                   };
                 };
               };
            };
  };
  if (Ch=='\r') return InStr; else return NULL;
}


char *wait_kmd(unsigned int time)
/*********************************************************************/
{ /*sys(K_rld,1000);
   while ((line(0)==NULL) && (time>0))
    { time-=sys(K_rdflg,1); sys(K_rdy,0); };
  sys(K_rld,0);
  */
  return line(0);
}

char *wait_OK(int time)
/*********************************************************************/
/** Zeiger wenn gefunden  NULL wenn timeout oder ERROR        ********/
{ signed char OK=-1;
 // sys(K_rld,1000); OK=-1; line(2);
   while (OK==-1)
    { if (time<=0) OK=1;
 //     time-=sys(K_rdflg,1);
      if (line(0))
       { if (iskmd(line(0),"\rOK") || (strcmp(line(0),"OK")==0)) OK=0;
         else if (iskmd(line(0),"\rERROR") || (strcmp(line(0),"ERROR")==0)) OK=2;
         else if (iskmd(line(0),"+CMS ERROR:")) OK=2;
         else if (iskmd(line(0),"+CME ERROR:")) OK=2;
         else line(3);
       };
 //     sys(K_rdy,0);
    };
 // sys(K_rld,0);
  if (OK==0) return line(0); else return NULL;
}

char *strstr(char *cs,char *ct)
/*********************************************************************/
{ while (cs=strchr(cs,ct[0]),cs)
   { if (strncmp(cs,ct,strlen(ct))==0) return cs; else ++cs; };
  return NULL;
}

char DNetz(char *Receiver)
/*********************************************************************/
{  if (Receiver) return DNr; else return 0;
}

void rdReceiver()
/*********************************************************************/
{ unsigned char n,i,Flg;
  int l;
  char  *Temp,*Dest,*komma;

  for (n=0; n<=3; ++n)
   { Tel[n].Nr[0]='\0'; Tel[n].Typ=0; Tel[n].Trial=0; Tel[n].D=0; };

  if (Temp=rdstrCRC(Receiver,CRCReceiver))
   { n=1;
     while ((*Temp) && (n<=3))
      { Dest=(char *)Tel[n].Nr; i=0; Flg=0;
        if (komma=strchr(Temp,','),komma==0) komma=Temp+strlen(Temp);
        l=(int)(komma-Temp);
        if (*Temp=='(') {       /* emails und Festnetz in der Schweiz */
         if ((sizeof(TelefonNr)-2)>l)
          strncpy(Dest,Temp,l); Dest[l]=0;
         Temp=komma;
         Tel[n].Typ|=(t_email | t_sms);
        } else {
         while (*Temp && (*Temp!=',') && (i<sizeof(TelefonNr)-2))
          { switch (*Temp) {
             case 'C': case 'c': Tel[n].Typ|=t_cnct; Tel[n].Typ&=~t_sms; break;
             case 'R': case 'r': Tel[n].Typ|=t_ruf; Tel[n].Typ&=~t_sms; Flg=1; break;
             case 'S': case 's': Tel[n].Typ|=t_sms; break;
              default: if ( (*Temp>='0') && (*Temp<='9')
                             || (*Temp=='I')
                             || (*Temp=='i')
                          )
                       { if (Flg) Tel[n].Trial=*Temp&0xF; else *Dest=*Temp;
                         ++Dest; ++i;
                       };
             };
            ++Temp; *Dest='\0';
          };
        };
        Tel[n].D=DNetz(Tel[n].Nr);
        if (strlen(Tel[n].Nr) && !Tel[n].Typ) Tel[n].Typ=t_sms;
        ++n; if (*Temp) ++Temp;
      };
   };
}


void ResetModem()
/*********************************************************************/
{ line(1);
  // LEDfunk=Ein;
   iniIO(96);

#ifndef Alt
  // DTR=Aus; sys(K_time,1500); DTR=Ein;
#else
   sys(K_time,1500);
   printf("+++");
#endif
//   sys(K_time,1500);
   rdReceiver();
   iniIO(96);
#ifdef Alt
  #ifdef M20
   printf("AT H E0 &D0 \r\n");
  #endif
  #ifdef TC35
   printf("AT H E0 &D0 \r\n");
  #endif
  #ifdef M1
   printf("AT H &F E0 &D0 B99\r\n");
  #endif
#else
  #ifdef M20
   printf("AT H E0 &D2 \r\n");
  #endif
  #ifdef TC35
   printf("AT H E0 &D0 \r\n");
  #endif
  #ifdef M1
   printf("AT H &F E0 B99\r\n");
  #endif
#endif

//  LEDfunk=Aus;
}

void SetBaudrate()
/*********************************************************************/
{
#if (defined(M20) || defined(TC35))
  line(1); LEDfunk=Aus;
  iniIO(192);
 #ifndef Alt
   DTR=Aus; LEDfunk=Ein; sys(K_time,1500); DTR=Ein;
 #else
   sys(K_time,1500); LEDfunk=Ein;
   printf("+++");
 #endif
   sys(K_time,1500); LEDfunk=Aus;
   printf("AT+IPR=9600\r\n"); sys(K_time,500);
#endif
   ResetModem();
}


void wait_reset()
/*********************************************************************/
{ ResetModem(); wait_kmd(20); line(1); }



char Dial(char *TelNr)
/*********************************************************************/
{
  line(2); printf("atd %s\r\n",TelNr);
  if (iskmd(wait_kmd(60),"CONNECT")) return 2;
  else { printf("\r\n"); wait_reset(); return 0; };
}




char sendemail(char *SMS,char *Buff,char *DA)
/*********************************************************************/
{ char *pnt;
  char result;

 /* DA ist "(Nummer)email-adresse". email mu· auch evt. Trennzeichen */
 /* zur Abgrenzung zur SMS enthalten                                 */
 /* z.B D2  DA='(3400)hightech@horizont.com:'                        */
 /*     D1  DA='(8000)hightech@horizont.com '                        */

  result=0;

  strcpy(Buff,DA); pnt=strchr(Buff,')');
  if ( (*Buff!='(') || (pnt==NULL) ) return 1;
  ++Buff; *pnt=0; DA=pnt+1;

  line(2); printf("at+cmgs=\"%s\"\r\n",Buff);
  if (iskmd(wait_kmd(10),">"))
  { line(2); printf("%s %s%c",DA,SMS,0x1A);
    if (wait_OK(30)) result=1;
  };
  line(2); return result;
}

char sendTxtSMS(char *SMS,char *Buff,char *DA)
/*********************************************************************/
{ char result;
  result=0;
  strcpy(Buff,DA);
  line(2); printf("at+cmgs=\"%s\"\r\n",Buff);
  if (iskmd(wait_kmd(10),">")) {
   line(2); printf("%s%c",SMS,0x1A);
   if (wait_OK(30)) result=1;
  };
  line(2); return result;
}


char sendSMS(char *SMS)
/*********************************************************************/
{ char result;
  result=0;
  line(2);
  printf("at+cmgs=140\r\n");
  if (iskmd(wait_kmd(60),">")) {
   line(2); printf("%s\r\n",SMS);
   if (wait_OK(60)) result=1;
  };
  line(2); return result;
}

void makeASCII7bit(char *Buff,unsigned int UDL)
/*********************************************************************/
{ char *Pnt,n;
  int  Ch,X;

  Pnt=Buff; n=0;
  if (UDL<=160)
   { sscanf(Pnt,"%2x",&Ch);
     while (n<UDL) {
      Buff[n]=Ch&0x7F; Ch>>=7; ++n;
      if (n&7) { Pnt+=2; sscanf(Pnt,"%2x",&X); Ch|=X<<(n&7); };
     };
   };
  Buff[n]=0;
}

void makeASCII(char *Buff,unsigned int UDL)
/*********************************************************************/
{ char *Pnt,n;
  int  Ch;

  Pnt=Buff; n=0;
  if (UDL<=140)
   { while (n<UDL) {
      sscanf(Pnt,"%2x",&Ch); Buff[n]=Ch&0xFF; ++n; Pnt+=2;
     };
   };
  Buff[n]=0;
}

void makeDig(char *Buff,unsigned int Lng)
/*********************************************************************/
{ char n;
  int  Ch;

  n=0; Ch='0';
   { while ((n<Lng) && (Ch<='9') ) {
      Ch=Buff[n]; Buff[n]=Buff[n+1]; ++n;
      if (Ch<='9') { Buff[n]=Ch; ++n; };
     };
   };
  Buff[n]=0;
}


char *GetWord(char *Buff, char **Dest)
/*+********************************************************************/
/* Dest zeigt auf Text zwischen "..." und Rueckgabe zeigt auf Zeichen */
/* nach , oder \n wenn vorhanden, sonst NULL                          */
{ char *Tmp;
  Tmp=Buff; Tmp=Tmp+strcspn(Tmp,",\"\r\n");
  switch (*Tmp) {
    case '"':  *Dest=++Tmp; Tmp=Tmp+strcspn(Tmp,"\"");
               if (*Tmp) { *Tmp='\0'; ++Tmp; };
               Tmp=Tmp+strcspn(Tmp,",\r\n");
               if (*Tmp==',') ++Tmp;
               break;
    case ',':  *Tmp='\0'; *Dest=Buff; ++Tmp; break;
    case '\0': *Dest=Buff; break;
    case '\r': case '\n':
               Tmp=Tmp+strspn(Tmp,"\r\n");
               *Dest=Tmp; Tmp=Tmp+strcspn(Tmp,"\r\n");
               *Tmp='\0';
               break;
  };
  if (*Tmp) return Tmp; else return NULL;
}


char *ConvTxtSMS(char *Pnt,char *Buff)
/*********************************************************************/
/* Gibt einen Zeiger auf die Meldung zurueck oder NULL wenn Fehler   */
/* Setzt die Globalen Zeiger UD,OA,SCTS sowie SMSlng                 */
/* +CMGR: "REC READ","+491703449286",,"99/06/08,07:47:07+4"\r\n      */
/* Text                                                              */
{ strcpy(Buff,Pnt); UD=NULL; OA=NULL; SCTS=NULL;
  if (strncmp(Buff,"+CMGR:",6)) Buff=NULL;
  else
   { Pnt=GetWord(Buff,&UD); UD=NULL;
     if (Pnt) { Buff=Pnt; Pnt=GetWord(Buff,&OA); };
     if (Pnt) { Buff=Pnt; Pnt=GetWord(Buff,&UD); UD=NULL; };
     if (Pnt) { Buff=Pnt; Pnt=GetWord(Buff,&SCTS); };
     while (Pnt) { UD=NULL; Buff=Pnt; Pnt=GetWord(Buff,&UD); };
/*   if (strncmp(OA,"+49",3)==0)
         { OA+=6;
           if (strncmp(OA,"0000",4)) { OA-=4; *OA='0'; };
         }; */
   };
 /*  if (Buff) printf("%s %s\r\n%s\r\n",SCTS,OA,UD); */
  return Buff;
}

char *ConvSMS(char *Pnt,char *Buff)
/*********************************************************************/
/* Gibt einen Zeiger auf die Meldung zurueck oder NULL wenn Fehler   */
/* Setzt die Globalen Zeiger UD,OA,SCTS sowie SMSlng                 */

{ unsigned int  Typ,PDU,OAL,Nrtyp,PID,DCS,UDL;
  SMSlng=0;
#if (defined(M20) || defined(TC35))
  sscanf(Pnt,"+CMGR: %i ,, %i %2x %2x %s",&Typ,&SMSlng,&PDU,&OAL,Buff);
#else
  sscanf(Pnt,"+CMGR: %i , %i %2x %2x %s",&Typ,&SMSlng,&PDU,&OAL,Buff);
#endif
  if ((strlen(Buff)+4)==(2*SMSlng))
   { if (OAL<=20)
      { if (OAL&0x1) OAL+=1;
        sscanf(Buff,"%2x",&Nrtyp);         Buff+=2;
        OA=Buff;                           Buff+=OAL;
        sscanf(Buff,"%2x %2x",&PID,&DCS);  Buff+=4;
        SCTS=Buff;                         Buff+=14;
        sscanf(Buff,"%2x",&UDL);           Buff+=2;
        if (DCS&4) makeASCII(Buff,UDL); else makeASCII7bit(Buff,UDL);
        makeDig(OA,OAL);
        if (strncmp(OA,"49",2)==0)
         { OA+=5;
           if (strncmp(OA,"0000",4)) { OA-=4; *OA='0'; };
         };
        makeDig(SCTS,14); UD=Buff;
        /* printf("%i %s %s\r\n%s\r\n",UDL,SCTS,OA,UD); */
        return Buff;
      };
   };
  UD=NULL;
  return NULL;
}

char *isSMS(unsigned int SMSnr,char *Buff)
/*********************************************************************/
{ line(1); printf("at+cmgr=%i\r\n",SMSnr);
  if (wait_OK(10))
   {
#if (defined(M20) || defined(TC35))
     if (Buff=ConvTxtSMS(line(4),Buff))
#else
     if (Buff=ConvSMS(line(4),Buff))
#endif
      { line(1); printf("at+cmgd=%i\r\n",SMSnr); wait_OK(5); };
     return Buff;
   };
  return NULL;
}


char CheckID()
/*********************************************************************/
{ return (rdstrCRC(Receiver,CRCReceiver) && rdstrCRC(AmpelID,CRCAmpelID));
}


char CheckRegistration()
/*********************************************************************/
{ char i,OK;
  line(1); // sys(K_time,1000);
  OK=0;
  for (i=0; ((i<3) && (OK==0)); ++i) {
   printf("at+cops?\r\n");
   if ( wait_OK(20) ) {
    if ( iskmd(line(4),"+COPS:") ) {
     if (strstr(line(4),"D1")!=NULL) { OK=1; DNr=1; }
     else if (strstr(line(4),"D2")!=NULL) { OK=1; DNr=2; }
     else { OK=1; DNr=0; }
    }
   }
   line(2); // if (OK==0) sys(K_time,3000);
  }
  return OK;
}


void CheckQuality()
/*********************************************************************/
{ line(1); printf("at+csq\r\n");
  if (wait_OK(20))
   { if ( iskmd(line(4),"+CSQ:") )
      { sscanf(line(4),"+CSQ: %i , %i",&Qfunk,&Efunk); line(1);
      }
   }
}
unsigned char ATKmd(char *s,char OK)
{ if (OK==1)
   { line(1); printf("%s",s); if (wait_OK(5)) return 1; }
  else return OK;
}


char InitModem()
/*********************************************************************/
{ unsigned char OK;
  OK=0;
  while (OK==0) {
   do {
    SetBaudrate(); wait_kmd(20);
   } while ( !( iskmd(line(0),"OK") || iskmd(line(0),"PC") ) );
   if (iskmd(line(0),"PC")) {
    line(1); OK=2;
   } else {
    if (1) {
     line(1); printf("at+cpin?\r\n");
     if (iskmd(wait_OK(20),": READY")) OK=1;
     else {
      if ( iskmd(line(0),": SIM PIN") ) {  // && rdstrCRC(PIN,CRCPIN) ) {
       line(1); printf("at+cpin=\"0000\"\r\n");
       if (wait_OK(20)) OK=1;
      };
     };
    };
   };
  };
  line(2);
  ATKmd("at+cmgd=1\r\n",OK);
  ATKmd("at+cmgd=2\r\n",OK);
  if (OK==1) { if (!CheckRegistration()) OK=0; };
#ifdef M20
  OK=ATKmd("at+csms=128\r\n",OK);
  OK=ATKmd("at+cmgf=1\r\n",OK);
#endif
#ifdef TC35
  OK=ATKmd("at+cmgf=1\r\n",OK);
  OK=ATKmd("at+cnmi=2,1\r\n",OK);
  OK=ATKmd("at+cpms=me,me,mt\r\n",OK);
  OK=ATKmd("at+cpms=sm,sm,sm\r\n",OK);
#endif
  return OK;
}

void perror()  { printf("FEHLER\r\n"); }
void clrscr()  { printf("\x1B[2J\x1B[0;0H"); }
void gotoxy(unsigned char r, unsigned char c) { printf("\x1B[%i;%iH",(int)r,(int)c); }
void clreos()  { printf("\x1B[0J"); }
void savcrs()  { printf("\x1B%c",'7'); }
void rstcrs()  { printf("\x1B%c",'8'); }

// TIMER(Verbindung,70)

void storehistory(char *Time,char *Txt)
/*********************************************************************/
{ HPnt&=0xF;
  sprintf(History[HPnt],"%-15s",Time);
  strcat(History[HPnt],Txt); ++HPnt;
  HPnt&=0xF;
}

void clearhistory()
/*********************************************************************/
{ char i;
  HPnt=0; i=0;
  do { History[i][0]=0; i=(i+1)&0xF; }
  while (HPnt!=i);
}

void printhistory(char ModemTyp,char *Kopf)
/*********************************************************************/
{ char HPntX;
  HPntX=(HPnt-1)&0xF;
  clrscr(); printf("%s\r\n",Kopf);
  while (keypressed()) getkey();
  while (HPntX!=HPnt) {
   printf("%s\r\n",History[HPntX]); HPntX=(HPntX-1)&0xF; };
  printf(">");
  Verbindung(1,60000);
  while (!keypressed() && (Verbindung(K_avail,0) || (ModemTyp==2)) ) { };
}

/* typedef union {
 struct { unsigned char Adr,Abs,Lng,Nr,Typ;
          unsigned char ERR0,ERR1,Lamp,LONERR,ActSt,Iok;
          unsigned int  PhZt;        }        E;
 struct { unsigned char Adr,Abs,Lng,Nr,Typ;
          unsigned char secL,secH,minL,minH,stdL,stdH,dayL,dayH,monL,monH,
                        yearL,yearH; }        T;
 struct { unsigned char Adr,Abs,Lng,Nr,Typ;
          unsigned char Anz,CHWC,SWSTi;
          unsigned int  Sensor;
          unsigned char VktFlg,Frei,PhZt,Dest,ActSt;
          unsigned long Time;               } N;
 struct { unsigned char Adr,Abs,Lng,Nr,Typ;
          unsigned char buffer[32];         } P; } Block;
*/

void printntz(char ModemTyp,char Nr)
/*********************************************************************/
{ char X,I,K,n,N;
  char temp[20];
  NtzBlk *Pnt;

  switch (Nr) {
   case 0: X=0; break;
   case 1: X=1; break;
   default: X=(NtzI+1)&1;
  };

  I=(NtzJ[X]+1)&0x1F;
  clrscr(); printf("Notizen %i\r\n",(int)X);
//  sys(K_time,500);

  while (keypressed()) getkey();
  for (N=0; N<32; ++N) {
   Pnt=&Notizen[X][I];
   temp[0]=Pnt->Blk.N.Typ; temp[1]=0; printf("[%s] ",temp);

   switch (Pnt->Blk.N.Typ) {
    case 'N':
      printf("%3i ",(int)Pnt->Blk.N.CHWC);
      hex(temp,Pnt->Blk.N.ActSt); printf("%s ",temp);
      printf("%3i   ",(int)Pnt->Blk.N.PhZt+1);
      if (Pnt->Blk.N.Anz>16) n=0; else n=Pnt->Blk.N.Anz;
      for (K=1; K<=n; ++K) printf("%2X ",(int)Pnt->St[K]); break;
    case 'E':
      hex(temp,Pnt->Blk.E.Abs); printf("<%s> ",temp);
      hex(temp,Pnt->Blk.E.ERR0); printf("%s ",temp);
      hex(temp,Pnt->Blk.E.ERR1); printf("%s ",temp);
      hex(temp,Pnt->Blk.E.Lamp); printf("%s ",temp);
      hex(temp,Pnt->Blk.E.LONERR); printf("%s ",temp);
      hex(temp,Pnt->Blk.E.Iok); printf("%s ",temp); break;
   };
   printf("%s\r\n",CLREOL);
   I=(I+1)&0x1F;
  };
  printf(">");
 // Verbindung(1,60000);
 // while (!keypressed() && (Verbindung(K_avail,0) || (ModemTyp==2)) ) { };
}


void echo(unsigned char Row)
/*********************************************************************/
{ if (line(5))
  { savcrs(); gotoxy(Row,4); printf("%s",line(4)); rstcrs(); };
}

unsigned char VT100(char *Message,char *Buff)
/*********************************************************************/
{ unsigned int  i,b,k,Md,Ex;
  unsigned char Row,St;
  char *Pnt;

/*  printf("%s %s%s\r\n",SMStime,lastSMS,CLREOL); */

  k=SQ2.Anzahl; Row=k+6; Md=SQ2.Mode&0x1F;

  if (rdstrCRC(AmpelID,CRCAmpelID)) strcpy(Buff,AmpelID);
  else strcpy(Buff,"???");

  strcpy(Message,Modestr[Md]);

  if (FehlerCnt.X<2) i=0; else i=FehlerCnt.X-1;

  Pnt=GSM;
  sprintf(Pnt,"%s   %-15s  Fehler: %i    %s%s\r\n",
                  Buff,Message,i,UhrZeit,CLREOL);
  Pnt+=strlen(Pnt);

  sprintf(Pnt,"U=%4.1fV  Qfunk=%i%s\r\n",
                  (double)(UAkku*dADU),Qfunk,CLREOL);
  Pnt+=strlen(Pnt);


  if (rdstrCRC(Receiver,CRCReceiver)) strcpy(Buff,Receiver);
  else strcpy(Buff,"???");

  sprintf(Pnt,"SMS:         %s%s\r\n",Buff,CLREOL);
  Pnt+=strlen(Pnt);


  Buff[0]=0;
  calcstr(Buff,SQ2.Err,SQstr);

  i=Netz.N.Dest>>5;
  sprintf(Pnt,"%i/%3i  %s%s\r\n",i,Umlauf,Buff,CLREOL);
  Pnt+=strlen(Pnt);

   for (i=1,b=1; i<=k; ++i,b<<=1)
    { St=SQstat[i].status;

      if (Line[i]&0x80) {
       if (Quarz&&(Akku[i]!=0xFFFF)) sprintf(Pnt,"[%2i]",i);
       else strcpy(Pnt,"[  ]");
      } else sprintf(Pnt,"[%2i]",i);
      Pnt+=strlen(Pnt);

      if (Akku[i]!=0xFFFF) {
       sprintf(Pnt," %4.1fV",(double)(Akku[i]/100.0));
      } else strcpy(Pnt,"      ");
      Pnt+=strlen(Pnt);

      if (!Quarz) {
       if (Green&b) strcpy(Pnt," #"); else strcpy(Pnt," -");
       Pnt+=strlen(Pnt);

       sprintf(Pnt," %3i",(int)Frei[i]);
       Pnt+=strlen(Pnt);

       if (Netz.N.Sensor&b) strcpy(Pnt,"+"); else strcpy(Pnt," ");
       Pnt+=strlen(Pnt);

       if (St&x_Switch) strcpy(Pnt,"M"); else strcpy(Pnt," ");
       Pnt+=strlen(Pnt);

       if (SQstat[i].OK) strcpy(Pnt," OK "); else strcpy(Pnt,"    ");
       Pnt+=strlen(Pnt);
      } else { strcpy(Pnt," "); Pnt+=strlen(Pnt); };

      if (!Quarz || (Akku[i]!=0xFFFF)) {
       if (ModeTyp[Md]) Ex=ERR0[i]; else Ex=ERR0[i]&e_U;
       calcstr(Pnt,Ex,ERR0str); Pnt+=strlen(Pnt);
       if (ModeTyp[Md]) Ex=ERR1[i]; else Ex=ERR1[i]&x_err1;
       calcstr(Pnt,Ex,ERR1str); Pnt+=strlen(Pnt);

       if (LoffERR[i])
        { strcpy(Pnt," L/"); calcstr(Pnt,LoffERR[i],Lampenfehler);
          Pnt+=strlen(Pnt); };
       if (LonERR[i])
        { strcpy(Pnt," O/"); calcstr(Pnt,LonERR[i],Lampenfehler);
          Pnt+=strlen(Pnt); };
      };

      sprintf(Pnt,"%s\r\n",CLREOL);
      Pnt+=strlen(Pnt);
      echo(Row);
    };
    Pnt=0;

  gotoxy(1,1); printf("%s",GSM); clreos(); gotoxy(Row,1); return Row;
}


void SendBlock(char *mem,unsigned int N)
/*********************************************************************/
{ unsigned int CRC,i;

  putchar(0xAA); CRC=0;
  for (i=0; i<N; ++i)
   { putchar(mem[i]); CRC=crc(CRC,mem[i]);
     if (mem[i]==0xAA) putchar(0xAA);
   };
  i=CRC>>8;   putchar(i); if (i==0xAA) putchar(i);
  i=CRC&0xFF; putchar(i); if (i==0xAA) putchar(i);
  ;
}

void ShortMsg(char *Buff,char *Inp)
/*********************************************************************/
{ unsigned int k;
  unsigned char i,n,St;
  unsigned int b;
  unsigned int *P;

  strcpy(Buff,"I00");
  k=3;
  if (rdstrCRC(AmpelID,CRCAmpelID))
   { i=strlen(AmpelID); Buff[k++]=i; memcpy(&Buff[k],AmpelID,i); k+=i; }
  else { strcat(Buff,"\x03???"); k+=4; }

  if (rdstrCRC(Receiver,CRCReceiver))
   { i=strlen(Receiver); Buff[k++]=i; memcpy(&Buff[k],Receiver,i); k+=i; }
  else { strcat(Buff,"\x03???"); k+=4; }

  i=strlen(Inp); Buff[k++]=i; memcpy(&Buff[k],Inp,i); k+=i;

  i=6; Buff[k++]=i;

   P=(unsigned int *)&Buff[k];
   *P=SQ2.Err;         ++P;
   *P=UAkku*(dADU*10); ++P;
   *P=Qfunk;

  k+=i;

  i=14; Buff[k++]=i;
  n=Netz.N.Anz; Buff[k++]=n; --i;
  memcpy(&Buff[k],&Netz.N.CHWC,i); k+=i;

  for (i=1,b=1; i<=n; ++i,b<<=1)
   { Buff[k++]=8;
     St=SQstat[i].status&~(x_Vekt+x_global+x_Rqst+x_gruen);
     if (Green&b) St|=x_gruen;
     if (Netz.N.Sensor&b) St|=x_Rqst;
     Buff[k++]=St;
     Buff[k++]=LoffERR[i];
     Buff[k++]=LonERR[i];
     Buff[k++]=ERR0[i];
     Buff[k++]=ERR1[i];
     Buff[k++]=Frei[i];
     Buff[k++]=(Akku[i]&0xFF);
     Buff[k++]=((Akku[i]>>8)&0xFF);
   };
  P=(unsigned int *)&Buff[1]; *P=k;
  SendBlock(Buff,k);
}


char strcmpup(char *S1,char *S2)
/*********************************************************************/
/* Vergleicht zwei Strings ohne Beachtung der Gross- Kleinschreibung */
/* gibt 0 zurÅck, wenn S1=S2                                         */
{ while (*S1)
   { if (upcase(*S1)!=upcase(*S2)) return 1; ++S1; ++S2; };
  if (*S2) return 1; else return 0;
}


void cpytoup(char *Src,char *Dest)
/*********************************************************************/
/* Kopiert Src nach Dest und wandelt in Grossbuchstaben              */
{ unsigned int i;
  for (i=0; i<strlen(Src); ++i)
   { if ((Src[i]>='a') && (Src[i]<='z')) Dest[i]=(Src[i] & 0xDF);
     else Dest[i]=Src[i]; };
  Dest[i]=0;
}

unsigned char SMSAuswerten()
/*********************************************************************/
/* Gibt 1 zurueck wenn SMS angefordert                               */
{ unsigned char n,kmd;
  char *Sy;
  char Uhr[20];
  char Buff[150];

  kmd=0; n=1;
  while ( (n<=3) && !kmd )
   { if (strcmpup(Tel[n].Nr,OA)==0) kmd=1; ++n; };
  if (kmd)
   { for (n=1; n<=3; ++n) Tel[n].Event&=~t_ruf;
     wrCRC(&FehlerCnt,1);
   };

  kmd=0;
  if (!(Tel[0].Event&t_sms))
   { if (!strcmpup(UD,"INFO"))  /* Nachricht=Anlagenname */
      { strcpy(Tel[0].Nr,OA);
        Tel[0].Event=t_sms; Tel[0].Typ=t_sms;
        Tel[0].Cnt=1; Tel[0].D=DNetz(Tel[0].Nr); kmd|=1; }

     else if (Sy=iskmd(UD,"SMS="),Sy)
      { if (strlen(Sy)) cpystrCRC(Receiver,Sy,&CRCReceiver,sizeof(Receiver)-1);
        if (rdstrCRC(Receiver,CRCReceiver)) rdReceiver();
        strncpy(Uhr,UhrZeit,19);
        strcpy(Buff,"SMS(");
        strncat(Buff,OA,sizeof(Buff)-strlen(Buff)-3); strcat(Buff,") ");
        strncat(Buff,Receiver,sizeof(Buff)-strlen(Buff)-1);

#ifdef Quelle
        storehistory(Uhr,Buff);
#endif
        strcpy(Tel[0].Nr,OA);
        Tel[0].Event=t_sms; Tel[0].Typ=t_sms;
        Tel[0].Cnt=1; Tel[0].D=DNetz(Tel[0].Nr); kmd|=2;
      }
     else if (Sy=iskmd(UD,"ANLAGE="),Sy)
      { if (strlen(Sy)) cpystrCRC(AmpelID,Sy,&CRCAmpelID,sizeof(AmpelID)-1);
        strncpy(Uhr,UhrZeit,19);
        strcpy(Buff,"ANLAGE(");
        strncat(Buff,OA,sizeof(Buff)-strlen(Buff)-3); strcat(Buff,") ");
        strncat(Buff,AmpelID,sizeof(Buff)-strlen(Buff)-1);
#ifdef Quelle
        storehistory(Uhr,Buff);
#endif
        strcpy(Tel[0].Nr,OA);
        Tel[0].Event=t_sms; Tel[0].Typ=t_sms;
        Tel[0].Cnt=1; Tel[0].D=DNetz(Tel[0].Nr); kmd|=2;
      }
     else if (Sy=iskmd(UD,"AKKU"),Sy)
      { strcpy(Tel[0].Nr,OA);
        Tel[0].Event=t_sms; Tel[0].Typ=t_sms;
        Tel[0].Cnt=1; Tel[0].D=DNetz(Tel[0].Nr); kmd|=4;
      }
     else { Tel[0].Nr[0]=0; Tel[0].Event=0; };
   };

  line(2);
  return kmd;
}

void AnlagenSMS(char *lastSMS,char  *Buff)
/*********************************************************************/
{ Buff=rdstrCRC(AmpelID,CRCAmpelID);
  if (strlen(Buff)) strncpy(lastSMS,AmpelID,sizeof(AmpelID));
  else strcpy(lastSMS,"Fehler");
  strcat(lastSMS," ");
  Buff=rdstrCRC(Receiver,CRCReceiver);
  if (Buff) strcat(lastSMS,Buff); else strcat(lastSMS,"Fehler");
}


void USMS(char *lastSMS, char *Message, char  *Buff)
/*********************************************************************/
{ unsigned int i,Md;

  Md=SQ2.Mode&0x1F;
  strncpy(lastSMS,AmpelID,sizeof(AmpelID));
  if (Quarz) strcat(lastSMS," Quarz");
  strcat(lastSMS," ");
  if (Quarz) {
   if (ModeTyp[Md]) strcat(lastSMS," Fehler");
   else strcat(lastSMS,Modestr[Md]);
  } else strcat(lastSMS,Modestr[Md]);
  sprintf(Message," U=%4.1fV Q=%i ",(double)(UAkku*dADU),Qfunk);
  strcat(lastSMS,Message);
  Message[0]=0;
  for (i=1; i<=SQ2.Anzahl; ++i) {
   if (Akku[i]!=0xFFFF) {
    sprintf(Buff,"%i=%4.1fV ",i,(double)(Akku[i]/100.0));
    strcat(Message,Buff);
   } else {
    if (!Quarz) {
     sprintf(Buff,"%i=? ",i);
     strcat(Message,Buff);
    };
   };
  };
  if (strlen(Message)) strcat(lastSMS,Message); else strcat(lastSMS," ???");
}

void ErrSMS(char *lastSMS, char *Message, char  *Buff, char  *SMStime,
            unsigned int *aE, unsigned int *aKabel)
/*********************************************************************/
{ unsigned int i,b,n,E,Kabel,Md,iFlg,Ex;
  unsigned int err0,lofferr,kabel,x,akku;

  E=*aE; Kabel=*aKabel; Md=SQ2.Mode&0x1F;

  strncpy(lastSMS,AmpelID,sizeof(AmpelID));
  strcpy(SMStime,UhrZeit);
  if (Quarz) strcat(lastSMS," Quarz");
  if (rdCRC(&Reset)) strcat(lastSMS," Reset");
  else {
   strcat(lastSMS," ");
   if (Quarz) {
    if (ModeTyp[Md]) strcat(lastSMS," Fehler");
    else strcat(lastSMS,Modestr[Md]);
   } else strcat(lastSMS,Modestr[Md]);
  };
  if (SQ2.Err&x_Akku) sprintf(Message,"  U=%4.1fV Q=%i ",(double)(UAkku*dADU),Qfunk);
  else sprintf(Message,"  Q=%i ",Qfunk);
  strcat(lastSMS,Message);

  Ex=E;
  if (E&x_fatal) E&=~x_timeout;
  if (E&(x_unglt)) E&=~(x_OK|x_Switch|x_gruen);

  Message[0]=0;

  calcstr(Message,E,SQstr);
  for (i=1,b=1; i<=SQ2.Anzahl; ++i,b<<=1) {
   iFlg=Akku[i];
   err0=ERR0[i]; if (err0&e_U) akku=iFlg; else akku=0xFFFF;
   if (ModeTyp[Md]) err0&=~e_U; else err0=0;
   lofferr=LoffERR[i]; kabel=Kabel&b; x=xon&b;
   if ( err0 || lofferr || kabel || x || (akku!=0xFFFF) ) {
    if (Quarz) {
/*     iFlg=1;     */
     if (iFlg!=0xFFFF) {
      if (akku!=0xFFFF)
       sprintf(Buff," (%i %4.1fV",i,(double)(akku/100.0));
      else sprintf(Buff," (%i",i);
      strcat(Message,Buff);
      if (err0) { strcat(Message," "); calcstr(Message,err0,ERR0str); };
      if (lofferr) calcstr(Message,lofferr,Lampenfehler);
      strcat(Message,")");
     };
    } else {
     if (akku!=0xFFFF)
      sprintf(Buff," (%i %4.1fV",i,(double)(akku/100.0));
     else sprintf(Buff," (%i",i);
     strcat(Message,Buff);
     if (x || kabel || err0) strcat(Message," ");
     if (x) strcat(Message,"*");
     if (kabel) strcat(Message,"K");
     if (err0) calcstr(Message,err0,ERR0str);
     if (lofferr) { calcstr(Message,lofferr,Lampenfehler); };
     strcat(Message,")");
    };
   };

  };
  if (strlen(Message)) strcat(lastSMS,Message);
  else if (ModeTyp[Md]==0) strcat(lastSMS," Keine Fehler");

  if (E)
   { Buff[0]=0; strcat(Buff,"["); calcstr(Buff,Ex,SQstrEx); strcat(Buff,"] "); strcat(Buff,lastSMS);
     storehistory(SMStime,Buff);
     rdReceiver();
     for (n=1; n<=3; ++n)
       { Tel[n].Event|=Tel[n].Typ; Tel[n].Cnt=Tel[n].Trial; };
   };
  wrx(&RufDelay,1); wrx(&CnctDelay,1);
  *aE=0; *aKabel=0;
}

// TIMER(DialDelay,70)

TASK(TELE,120) {
/*********************************************************************/
/* wenn timeout keine AktivitÑt wird Verbindung unterbrochen         */

static unsigned int E,Chk,Send,Kabel;
static  char Buff[400];
static char Message[300];
static char SMS[400];
static char lastSMS[300];
static unsigned char Connect,Ring,ModemTyp,Row,kmdSMS,IOMode,n;
static  char *Sy;
static TimeStr  SMStime;
static PTmem Pnt;

E=0; SMS[0]=0; strcpy(lastSMS,"-"); SMStime[0]=0; kmdSMS=0;
for (n=0; n<=3; ++n) Tel[n].Event=0; Qfunk=0; Efunk=0;

Pnt=Verbindung(K_adr,0);

while (1)
 { LEDfunk(Aus); TELEcnt=0;
   ModemTyp=InitModem(); line(1);
   Connect=0; Ring=0; Chk=0; Send=0; IOMode=0;
   switch (ModemTyp) {
    case 1: LEDfunk(Ein); CheckQuality(); break;
    case 2: printf("PC On\r\n"); break;
   };
   line(1);

   while (ModemTyp)
    { ++TELEcnt;

      if (ModemTyp==1) LEDfunk(Aus);
       // sys(K_time,100);

      if (ModemTyp==1)
       { if (Qfunk<10)
          { if ( (TELEcnt/10)&1 ) LEDfunk(Ein); }
         else LEDfunk(Ein);
       };

      ++Send;

      sys(1,waitmem);
       E|=NewErr;
       if (NewErr) wrx(&SMSDelay,10);
       if (E) Kabel|=SQ2.Kabel; else Kabel=0;
       NewErr=0; SQ2.Kabel=0;
      freebit(ErrMem);

      if (Kabel&1) Kabel=1;
      if (rdbit(Synchr)) storehistory(UhrZeit,"Programmstart");

      if (Connect && (ModemTyp!=2))   /* timeout zum automatischen Auflegen */
       { if ( !Verbindung(K_avail,0) )
          { wait_reset(); Connect=0; Ring=0;
          };
       };

      while (line(0)!=NULL)
       { if (iskmd(line(0),"RING"))
          { Ring=1; printf("ATA\r\n"); }

         else if (iskmd(line(0),"CONNECT"))
          { Connect=1; Ring=0; IOMode=1;
            wait_empty(); clrscr(); line(2); Row=0;
            rdbit(bit1s); Send=0;
          }

         else if (   iskmd(line(0),"NO CARRIER")
                  || iskmd(line(0),"BUSY") )
          { wait_reset(); Verbindung(K_del,0); Connect=0; Ring=0; }

         else if (iskmd(line(0),"ENDE"))
          { ModemTyp=0; Verbindung(K_del,0); }

         else if (iskmd(line(0),"XA"))
          { IOMode=1; }

         else if (iskmd(line(0),"XB"))
          { IOMode=2; }

         else if (iskmd(line(0),"NULL"))
          { wrCRC(&FehlerCnt,1); }

         else if (iskmd(line(0),"PROTOKOLL"))
          { if (rdstrCRC(AmpelID,CRCAmpelID)) strcpy(Message,AmpelID);
            else strcpy(Message,"???"); strcat(Message,"  ");
            strcat(Message,Modestr[SQ2.Mode]); strcat(Message," ");
            strcat(Message,UhrZeit);
            printhistory(ModemTyp,Message);
          }
         else if (iskmd(line(0),"NTZ0"))
          { printntz(ModemTyp,0); }
         else if (iskmd(line(0),"NTZ1"))
          { printntz(ModemTyp,1); }
         else if (iskmd(line(0),"NTZ"))
          { printntz(ModemTyp,3); }

         else if (iskmd(line(0),"CLEAR"))
          { E=0; kmdSMS=0; strcpy(lastSMS,"-"); SMStime[0]=0;
            sys(1,waitmem);
             if (rdCRC(&FehlerCnt)) wrCRC(&FehlerCnt,1);
            freebit(ErrMem);
            clrlinecnt(); clearhistory();
            if (!Connect) printf("OK\r\n");
          }

         else if (Sy=iskmd(line(0),"SMS="),Sy /* && !Connect */ )
          { if (strlen(Sy))
             cpystrCRC(Receiver,Sy,&CRCReceiver,sizeof(Receiver)-1);
            rdReceiver();
            if (!Connect) {
             if (rdstrCRC(Receiver,CRCReceiver))
              { for (n=1; n<=3; ++n)
                 { printf("%i %s",(int)n,Tel[n].Nr);
                   if (Tel[n].Typ&t_cnct)  printf(" Daten");
                   if (Tel[n].Typ&t_email) printf(" e-mail");
                   if ( (Tel[n].Typ&t_ruf) && Tel[n].Trial)
                     printf(" %ixRufen",(int)Tel[n].Trial);
                   if (Tel[n].Typ&t_sms)   printf(" SMS");
                   printf("\r\n");
                 }
              }
             else perror();
            }
          }

         else if (Sy=iskmd(line(0),"ANLAGE="),Sy /* && !Connect */ )
          { if (strlen(Sy)) cpystrCRC(AmpelID,Sy,&CRCAmpelID,sizeof(AmpelID)-1);
            if (!Connect) {
             if (rdstrCRC(AmpelID,CRCAmpelID))
              printf("ANLAGE=%s\r\n",AmpelID); else perror();
            };
          }

/*         else if (Sy=iskmd(line(0),"PIN="),Sy && !Connect)
          { if (strlen(Sy)) cpystrCRC(PIN,Sy,&CRCPIN,4);
            if (rdstrCRC(PIN,CRCPIN))
               printf("PIN=%s\r\n",PIN); else perror();
          } */
         else if (!Connect && (ModemTyp==2)) perror();

         if (Connect || Ring) Verbindung(1,60000);
         line(2);
       };

      if (!Verbindung(K_avail,0))
       { if (ModemTyp==1)
          { line(1); printf("at\r\n");
            if (iskmd(wait_kmd(20),"OK"))
             { if (isSMS(1,SMS)) kmdSMS|=SMSAuswerten();
               if (isSMS(2,SMS)) kmdSMS|=SMSAuswerten();
               line(2);
               CheckQuality();
               Verbindung(1,10000);

         /*    printf("\r\nLoff ");
               for (i=1; i<=SQ2.Anzahl; ++i) printf("%4X",(int)LoffERR[i]);
               printf("\r\nLon  ");
               for (i=1; i<=SQ2.Anzahl; ++i) printf("%4X",(int)LonERR[i]);
               printf("\r\nERR0 ");
               for (i=1; i<=SQ2.Anzahl; ++i) printf("%4X",(int)ERR0[i]);

               printf("\r\n%4X %4i %4X \r\n",E,SMSDelay,SQ2.Err);
               printf("%s\r\n",lastSMS); */
             }
            else
            /* if ( !line(0) || iskmd(line(0),"ERROR") ) */  ModemTyp=0;
          };
       };

      if (!rdx(&SMSDelay))
       { if ( E || kmdSMS)
          { if (E || (kmdSMS & 1)) ErrSMS(lastSMS,Message,Buff,SMStime,&E,&Kabel);
            else if (kmdSMS & 2) AnlagenSMS(lastSMS,Buff);
            else if (kmdSMS & 4) USMS(lastSMS,Message,Buff);
            else ErrSMS(lastSMS,Message,Buff,SMStime,&E,&Kabel);
          };
       };
      if (kmdSMS) { kmdSMS=0; wrx(&RufDelay,1); };

      if (!Connect && (ModemTyp==1))
       { if (!rdx(&RufDelay))
          { for (n=0; n<=3; ++n)
             { if (Tel[n].Event&t_sms)
                { Sy=Tel[n].Nr;
#if (defined(M20) || defined(TC35))
                  if (Tel[n].Event&t_email)
                   { if ( sendemail(lastSMS,SMS,Sy) )
                      Tel[n].Event&=~(t_sms|t_email); else wrx(&RufDelay,30);
                   }
                  else
                   { if ( sendTxtSMS(lastSMS,SMS,Sy) )
                        Tel[n].Event&=~t_sms; else wrx(&RufDelay,30);
                   }
#else
                  if ( sendSMS(calcSMS(lastSMS,SMS,Sy)) )
                   Tel[n].Event&=~t_sms; else wrx(&RufDelay,30);
#endif
                }
               else
                { if (Tel[n].Event&t_ruf)
                   { if (Tel[n].Cnt)
                      { Dial(Tel[n].Nr); sys(K_time,20000);
                        if (--Tel[n].Cnt && !rdx(&RufDelay))
                         wrx(&RufDelay,300);
                      }
                   }
                };
             };
          };

         if (!rdx(&CnctDelay))
          { wrx(&CnctDelay,120); n=4;
            while (n)
             { --n;
               if (Tel[n].Event&t_cnct)
                { if (Dial(Tel[n].Nr)) n=0;
                  if (n) sys(K_time,20000);
                };
             };
          };
       };


      if (Connect)
       { if ((Send>15) || rdbit(bit1s))
          { switch (IOMode) {
             case 0: Sy=line(5);
                     if (Sy!=NULL) printf("%s",Sy);
                     Send=16; Row=0; break;
             case 1: Row=VT100(Message,Buff);
                     printf("%2i>%.75s",(int)(Pnt->var.L[0]/1000),line(4));
                     Send=0; break;
             case 2: ShortMsg(Message,line(4)); Row=0;
                     Send=0; break;
             default: Send=0; Row=0;
             };
            for (n=1; n<=3; ++n) Tel[n].Event&=~(t_cnct|t_ruf);
          }
         else
          { Sy=line(5);
            if (Row && Sy)
            { gotoxy(Row,4); printf("%.75s%s",line(4),CLREOL); };
          };
         Chk=0;
       };
    };
 };
}

char wait_taste(PTmem mem)
{ if ( ((mem->var.I[0]-=EBScnt)>0) && rdM(&Taste) )
  return(M_wait); else return(M_rdy);
}

char tastedown(PTmem mem)
{ if ( ((mem->var.I[0]-=EBScnt)>0) && !rdM(&Taste) )
  return(M_wait); else return(M_rdy);
}


void Run(char New)
/*********************************************************************/
{ if (New) {
   wrCRC(&FehlerCnt,0); wrCRC(&Reset,2);
  } else wrCRC(&Reset,1);
  clrBloecke();
  New=0;
  while (1) { ++MAINcnt; SampleErrors(); };
  LEDP(10,0);
}

TASK(start,120) {
/*********************************************************************/
/* char B1[200]; /* */
/* char B2[200]; /* */
unsigned int i;

/*  cpystrCRC(Receiver,"high.horizont@t-online.de,03343289086",&CRCReceiver,100);
  rdReceiver();
  strcpy(B1,"Hallo");

  sendemail(B1,B2,Tel[2].Nr,1);
/*/

  ie_gap=0;
  for (i=0;i<sizeof(ERR0);++i) {
   Akku[i]=0xFFFF; Ptime[i]=0xFFFF;
   LonERR[i]=0; LoffERR[i]=0; ERR0[i]=0;
  };

  UHR(1,&runUhr); sys(K_time,1000);

  TELE(1,0);
  LED(10,0);
  LEDP(10,0);

  while (1)
   { Reset.X=0;
     Run(0);
   };
}

void main() {
   WDTREL=0; WDT=1; SWDT=1;
   strcpy(UhrZeit,"00.00. 00:00");
   LEDpow(Aus); LEDerr(Aus); Quarz=1;
   P2=0; /* Page 0 fÅr pdata anwaehlen */
   /*                     Ser.1    Comp.   Ser.0    Timer2
   /* Interruptprioritaet RS485 -> Netz -> RS232 -> EBS */
   /*                     3        2       1        0   */
   IP0=0x11;  /* 00010001 */
   IP1=0x09;  /* 00001001 */
   iniRS485();
   iniIO(96);
   EAL=1;

   Netz.P.Typ=0;
   Fehler.P.Typ=0;
   Kommando.P.Typ=0;
   Daten.P.Typ=0;
   Lernen.P.Typ=0;
   Global.P.Typ=0;
   Micro.P.Typ=0;
   NewErr=0;
   freebit(0xFF);     /* SEMAPHORE=0 */
   initebs(-461,10); start(1,0);
}

/*
char buff1[100];
char buff2[400];
char *pnt;

  EAL=0;
  while (1) {
   strcpy(buff1,"+CMGR: 0 , 26\r040C9194710100001000008901620172730008D3A80C442DCFE9");
   pnt=GetSMS(buff1,buff2);
  };
*/
