
#ifndef _SQ485
#define _SQ485

#include    <uart.h>

#define SQ_BLK_MAXLENGTH	32
#define SQ_BLK_MASTERDAT_LENGTH	14
#define SQ_NET_MAXANZAHL	16
#define MAX_STAT_COUNT		7
// #define MT_ADR			RS485_NET_MAXANZAHL-1
#define SQ_GLB_ADR			0

// Bits im Statusregister
#define SQ_UNGLT  0x80
#define SQ_LINEOK 0x40
#define SQ_VEKT   0x20
#define SQ_GLOBAL 0x10
#define SQ_RQST   0x08
#define SQ_OK     0x04
#define SQ_GREEN  0x01

typedef struct {
 unsigned short TRM_TIMEOUT;	// Nach dieser Zeit muss das Senden abgeschlossen sein
 unsigned short BLKPAUSE;	// Pause zwischen zwei Blöcken
 unsigned long  SLV_TIMEOUT;	// Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
 unsigned int   CHWCtakt;
} TSQParam;
typedef const TSQParam *PSQParam;

typedef union {
  struct {
   unsigned char green:		1;
   unsigned char schalter:	1;
   unsigned char OK:		1;
   unsigned char rqst:		1;
   unsigned char global:	1;
   unsigned char vekt:  	1;
   unsigned char line:  	1;
   unsigned char unglt: 	1;
  };
  unsigned char all;
} TSQStat;

typedef struct { TSQStat status; unsigned char count,OK,Line; } TSQNet_TeilnehmerStatus;

typedef enum { mdAuto,mdMan,mdBlink,mdOff,mdSensor,
               mdErot,mdErotbl,mdEgelb,mdEblink,
               mdSoff,mdSgelb,mdSblink,mdSrot,mdSrotbl,
               mdRot,mdRotblink,mdGelb,mdLernen,
               mdFgelb,
               mdError,mdQuarz,
               mdgelbblink } TLSAMode;


// SQ2 -Netz
#pragma pack(1)
typedef union {
 struct { unsigned char  ActSt,ERR0,ERR1,LONERR,LOFERR;
          unsigned char  s,min,std,tag,mon,year;
          unsigned short akku,ptime;
          unsigned char  Lamps,xx; }                 i;
 struct { unsigned char  ERR0,ERR1,Lamp,LONERR,ActSt,Iok;
          unsigned short PhZt;        }              E;
 struct { unsigned char  secL,secH,minL,minH,stdL,stdH,dayL,dayH,monL,monH,
                         yearL,yearH; }              T;
 struct { unsigned char  Anz,CHWC,SWSTi;
          unsigned short Sensor;
          unsigned char  VktFlg,Frei,PhZt,Dest,ActSt;
          unsigned int   Time;               }       N;
 struct { unsigned char  buffer[SQ_BLK_MAXLENGTH]; } P;
} TSQDaten;

typedef TSQDaten *PSQDaten;
#pragma pack()

typedef struct {
  TBlockKopf Kopf;
  TSQDaten Daten;
  TSQStat AND;
  TSQStat OR;
  TSQNet_TeilnehmerStatus State[SQ_NET_MAXANZAHL+2];
} TSQNet_State;

// Vereinbarung der benutzten Kanäle
extern TBlock caGlobal;

class TSQNetz {
 private:
  void (TSQNetz::*Mode)();	// Zeiger auf eine Klassenmethode
  bool AAtrm,AArcv;
  unsigned int TimeOutChar;	// Timeout für max. Zeit zwischen zwei Zeichen
  unsigned long TimeOutBlk; 	// Timeout für max. Zeit zwischen zwei Blöcken, danach Timeout im Slave
  unsigned long RcvTimeout;     // Timeout für Blockanfang im Empfänger
  unsigned int TaktCnt;         // Zähler zum Abzaehlen bis zum naechsten Start
  unsigned int CHWCcnt;
  PChannels RxCh,TxCh;
  void finddatenblock(void);
  void seterrnumber(char Nr, char Last);
  unsigned long Tx,dt;
  void wait();			// wartet auf 'N' oder auf Zeitpunkt um 'N' zu senden
  void trmD();			// Sendet einen Datenblock
  void rcvState();		// wartet auf den Status
 protected:
  PSQParam Param;
  TBlock caDummy;
  unsigned long Timeout[SQ_NET_MAXANZAHL+1];
  char LastChar;
  int StatIdx;   		// Idx des gerade empfangenen Statusbyte
  PBlock Daten;			// Zeiger auf den zu sendenden Datenblock
  PBlock RcvPnt;		// Zeiger auf den Block, der gerade Empfangen wird
  TBlock TrmBuf;              		// BlockPuffer des Senders
  PBlock TrmPnt;			// Zeiger auf den Block der gesendet wird
  unsigned int Takt;			// Reloadwert zum Abzaehlen bis zum naechsten Start
  void trmstart();			// macht den Netzblock fertig und schickt ihn los
  unsigned char DatPnt;    		// Nr des Teilnehmers, der Daten senden darf
  unsigned char DataIdx;		// Idx des Datenkanals der als letztes Daten gesendet hat
  TSQNet_State Temp;
  virtual void RS485on()=0;		// RS485 Sender einschalten
  virtual void RS485off()=0;		// RS485 Sender ausschalten
  virtual bool TxEmpty()=0;		// Frage ob der Sendepuffer leer ist
  virtual void TxStart(char Cx)=0;	// Die Übertragung starten und den Interrupt bei leerem Sendepuffer erlauben 	
  int putch(unsigned short cx);  	// übergibt das Zeichen an die Netzlogik
  int getch();                   	// holt ein Zeichen von der Netzlogik
  int RFlag;				// R-Block mit Nummer=self wurde empfangen, es kann gesendet werden
 public:
  TSQNetz(PSQParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,unsigned char Verb, unsigned char Anz);
  void Execute();
  void trmnext();			// Sendeinterrupt  Sendet das nächste Zeichen
  void rcvnext();			// Empfangsinterrupt
  unsigned short clrerr();	// wird aufgerufen wenn die Serielle Schnitstelle einen Fehler entdeckt
  bool ismaster();
  char ErrNumber;
  bool Flag; 				// Masterblock wurde empfangen
  unsigned char Self;      		// eigene Netzadresse
  unsigned char GlobalNr;  		// Nr des letzten gesendeten bzw. empfangenen globalen Datenblocks
  unsigned int GlobalQuit;      	// Quittung für den zuletzt gesendeten globalen Datenblock

  void (*OnTxStart)(TSQNetz *NET);	// wird aufgerufen, nachdem das erste Zeichen (cx) eines Blockes gesendet wurde
  void (*OnGetMasterDat)(TSQNetz *NET);	// wird aufgerufen, bevor der Masterblock gesendet wird
  void (*OnGetState)(TSQNetz *NET);	// wird aufgerufen bevor der Statusblock gesendet wird

  int wrstatus();			// Schreibt den Status des Teilnehmers StatIdx
  int (*OnSyncNetz)(TSQNetz *NET,int Flag);    // wird aufgerufen wenn auf Statusempfang bzw. zurückgeschaltet wird

  TSQStat SelfState;			// eigender Status
  unsigned char CHWC;
  TSQNet_State State;			// letzter N Block AND,OR und Teilnehmerstatus
  bool isNetOn();
  int MasterBlkCnt;
  PBlock sofort;			// Block wird sofort gesendet;
  virtual void TxStop()=0;		// den Interrupt bei leerem Puffer abschalten	
  virtual void TxD(char Cx)=0;		// Das Zeichen Cx in den Sendepuffer schreiben
  virtual bool RxEmpty()=0;		// Frage ob der Empfangspuffer leer ist;
  virtual unsigned short RxD()=0;	// Das nächste Zeichen auslesen;
  virtual unsigned short geterr()=0;	// Bereinigt die serielle Schnittstelle bei Fehlern
  virtual void initports()=0;
};

#endif
