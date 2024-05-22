/** \file States.hpp
    \brief Main Datei
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <LPC2138_sys_cnfg.h>
#include <LPC_SysControl.h>
#include <LPC_Vic.h>
#include <tools.h>
#include <task.h>
#include <timer.h>
#include <uart.h>
#include <LPC_fifo.h>
#include <RS232.h>
#include <RS485.h>
#include <SQ485.h>
#include <LPC_IAP.h>
#include <msprgscript.h>

#define MAXSGN 15  // eins größer als die maximale Kopfnummer
typedef enum { SQ2=1, MULTI, USA, EURO, FATTYP, USAneu, STORE=0xFF } NetzTyp_t;   // 1 2 3 4 5
extern NetzTyp_t NetzTyp;

extern TMSNetz *LSA;
extern const char Version[];
extern TBlock caGlobal;
extern unsigned char InfoBuff[50];
extern TBlock caInfo;		// Typ MultiSignalBlock
extern unsigned char InfoBuff1[50];
extern TBlock caInfo1;		// Typ MultiSignalBlock
extern unsigned char TxInfoBuff[50];
extern TBlock TxcaInfo;
extern unsigned char TxSofortBuff[50];
extern TBlock TxSofort;

extern const int QRYREPEATTIME;  //!< Nach dieser Zeit wird die Abfrage wiederholt, wenn keine Antwort kommt
extern const int QRYINTERVALL;  //!< In diesem Abstand erfolgt die Abfrage der Informationen
extern const int TRMTIMEOUT;      //!< Timeout für das Senden eines Blockes
extern const int STATINTERVALL; //!< In diesem Abstand werden Statusmeldungen ausgegeben 5min
extern const int QUITTIMEOUT;    //!< Nach dieser Zeit ohne Quittung wird der Block erneut gesendet
extern const int PAUSE;
extern const int MAXVERSUCHE;      //!< Anzahl der Versuche Informationen von der Multisignal zu lesen
extern const int SPANNUNGLOW;    //!< Ausgabe Spannungsfehler
extern const int SPANNUNGOK;    //!< Rücknahme Spannungsfehler
extern const int TIMEOUTK; //!< Timeout fuer die Abfrage eines K-Blockes
extern const int TIMEOUTR; //!< Timeout fuer die Abfrage eines R-Blockes
extern const int SGNSPUREN;          //!< Anzahl der zusätzlichen Aufzeichnungsspuren, PhZt (2), PrgNr

#pragma pack(1)
/*
    Statusbits sind auf die alten Bits im Netz abgestimmt
*/
typedef struct {
 union {
  struct {
	 uint8_t gruen:   1;
	 uint8_t gelb:    1;
	 uint8_t rot:     1;
   uint8_t SI:      1;    ///< Signal kommt setzen
   uint8_t interr:  1;    ///< interner Fehler oder Warnung
   uint8_t sensor2: 1;    ///< Sensor2 ausgelöst
   uint8_t taster:  1;    ///< Taster gedrückt
   uint8_t sensor:  1;    ///< Sensor ausgelöst

   uint8_t quarz:   1;    ///< Steuerung arbeitet im Quarzbetrieb
   uint8_t neu:     1;    ///< neues Programm wartet auf Start
   uint8_t frei:    1;    ///< Kopf zeigt Freigabesignal
   uint8_t OK:      1;    ///< keine internen Fehler
   uint8_t MT:      1;    ///< MT ist angesteckt
   uint8_t Prg:     1;    ///< Steuerung hat ein Programm
	 uint8_t request: 1;
	 uint8_t line:    1;

	 uint8_t gruen2:  1;
	 uint8_t gelb2:   1;
	 uint8_t rot2:    1;
	 uint8_t arrow:   1;
	 uint8_t arrow2:  1;
	 uint8_t res1:    1;
	 uint8_t res2:    1;
	 uint8_t digin1:  1;

	 uint8_t digin2:  1;
   uint8_t drehsch: 3;
	 uint8_t warnung: 1;

	 uint8_t Abs;
	 uint8_t Adr;

	};
   uint64_t All;
 };
} msstate_t;
#pragma pack()

#pragma pack(1)
typedef union {
 struct {
  unsigned short MT: 1;
  unsigned short sensor: 1;
  unsigned short taster: 1;
  unsigned short sensor2: 1;
 };
 unsigned short All;
} TMSInputs;

typedef union {
 struct {
  unsigned short gruen: 1;
  unsigned short gelb: 1;
  unsigned short rot: 1;
  unsigned short :5;
  unsigned short radio: 1;
  unsigned short lights: 1;
  unsigned short indicator: 1;
 };
 unsigned int All;
} TMSOutputs;

typedef struct {
 unsigned char power;
 unsigned char dimmer;
 unsigned short fehler;
 unsigned char versionHI;
 unsigned char versionLO;
 unsigned short zusatz;
 unsigned short maxeurover;
 unsigned char xxx;
 unsigned char quarz;
 unsigned char frequenz;
 unsigned short funkid;
 unsigned char subversion;
} TKmdK2;
typedef TKmdK2 * PKmdK2;

typedef struct {
	uint8_t  	power;         	///< Spannung in 0.1V Schritten
  uint8_t  	dimmer;   			///< Dimmer in %
  uint8_t  	versionHI; 			///< Version High
  uint8_t  	versionLO; 			///< Version Low
  uint8_t  	subversion; 		///< Unterversion
  errorbits_t	fehler;     		///< 4 Byte Fehlerbits
  uint16_t	Lon;						///< Lampenbits Lampe falsch an
  uint16_t	Loff;						///< Lampenbits Lampe kuputt
} TKmdK2new;
typedef TKmdK2new * PKmdK2new;

typedef struct {
 unsigned char Lng;
 unsigned short Adr;
 union {
  unsigned char mem[1];
  unsigned short PrgLng;
 };
} TKmdR;
typedef TKmdR * PKmdR;

typedef struct {
  unsigned short Kenn;
  unsigned short Lng;
  unsigned short Rqst;
  union {
   unsigned char Flags;       // Rqst=2211 und 2221
   struct {                   // Rqst=2232
    unsigned short Offset;
    unsigned short DataLng;
   };
   struct {                   // Rqst=2233
    unsigned char Nr;
    unsigned char Line;
    unsigned char Amount;
   };
  };
} T0xAEE1;
typedef T0xAEE1 * P0xAEE1;

typedef struct {
  unsigned short Kenn;
  unsigned short Lng;
  unsigned short Typ;
  unsigned char BlkCnt;
} T0xAEE2;
typedef T0xAEE2 * P0xAEE2;

typedef struct {
 unsigned char power;
 unsigned char dimmer;
 unsigned short fehler;
 unsigned char lng;          // Länge des Namens
 unsigned char name[1];       // nach dem Namen folgt noch der gewählte Funkkanal
} TAntwort;
typedef TAntwort * PAntwort;

typedef union {
 struct {
  unsigned char gruen:  1;
  unsigned char gelb:   1;
  unsigned char rot:    1;
  unsigned char Uerr:   1;
  unsigned char ogruen: 1;
  unsigned char ogelb:  1;
  unsigned char orot:   1;
  unsigned char uhr:    1;
  unsigned char qmode:  1;
  unsigned char feind:  1;
  unsigned char netz:   1;
  unsigned char OK:     1;
  unsigned char sgnerr: 1;
  unsigned char noprg:  1;
  unsigned char Cerr:   1;
  unsigned char sample: 1;
  unsigned char xxx:    8;
  unsigned char xxxx:   7;
  unsigned char unavail:1;
 };
 unsigned long all;
} TMSError;
#pragma pack()

class TMTSKopf {
 public:
  unsigned char Nr;
  unsigned char Lamps;
  unsigned short Version;   // Softwareversion
  unsigned short Spannung;
  unsigned char Signal;     // aktuelles signal
  unsigned short Green;     // letzte Grünzeit
  TMSInputs Inputs;         // aktueller Eingangspegel
  TMSOutputs Outputs;       // aktueller Ausgangspegel
  TMSError Error;           // Fehlerbits
  bool Ulow;                // Spannung war low
  bool Frei;
};

class TMultisignal : public TLock {
 public:
  unsigned char Time[6];      // Systemzeit
  unsigned char Anzahl;       // Anzahl Signalgeber
  unsigned char Nr;           // Nr an dem der Koppler steckt
  unsigned char VerbTyp;      // Verbindungstyp
  unsigned char Mode;         // Aktueller Mode
	unsigned char Warnung;      // ein Fehler der nicht zum Ausfall führt
  unsigned char PrgNr;        // Aktuelle Programmnummer
  unsigned short PhZt;        // Aktuelle Umlaufsekunde
  unsigned short Sekunden;    // Sekundenzähler
  unsigned char PrgName[10];  // Programmname
  TMTSKopf Kopf[MAXSGN];
  unsigned char TerminalNr;
  TMTSKopf Terminal;
  unsigned short TerminalTout;
};
typedef TMultisignal * PMultisignal;

class TMsgFlags : public TLock {
 public:
  bool BusyMsg;
  bool ChgMsg;    // Ausgabe einer StatusMsg
  bool ErrMsg;    // Ausgabe einer Fehlermeldung
  bool StatMsg;   // Ausgabe einer StatusMsg
  bool PrtkMsg;
  bool PrgMsg;
  bool Read;      // Einlesen des Status und anschließend StatusMsg
  bool ReadPrg;   // Programm einlesen und anschließend PrgMsg mit Länge 2
  struct {
   bool Flag;	  // Protokoll auslesen starten von Kopf Nr
   unsigned char Nr,Line,Amount;
   signed char Cnt;
  } Prtk;         // Protokoll auslesen
  TMsgFlags() { memset((void *)this,0,sizeof(TMsgFlags)); };
};

class TFrage {
 public:
  unsigned int Adr,Abs,Nr,Typ,Lng,MemAdr;
  TFrage operator=(TBlock op2);
  bool operator==(TBlock op2);
};

class TQuittung: public TLock {  // Quittung für einen Block an Ayantra
 public:
  unsigned short Typ;
  unsigned char BlkCnt;
  int Timeout;
  unsigned short Cnt;         // Anzahl der bereits gesendeten Quittungen
  TQuittung();
};

class TState {
 protected:
  void (TState::*Mode)();   //!< Zeiger auf eine Klassenmethode
  void wait();              //!< Warten auf Kommandos von der RS232 oder Ausgabe des naechsten Kommandos an die RS485
  void getInfo();           //!< Holt den Infoblock von allen Koepfen
  void getLamps();          //!< Holt die Lampen von allen Koepfen
  void getPrg();            //!< Holt das Programm von Kopf 1
  void getPrtkStart();      //!< Startet das Auslesen des Fehlerspeichers
  void getPrtk();           //!< setzt das Auslesen des Fehlerspeichers fort


//  volatile int AckType,AckCnt;
  volatile char ErrMsg,ChgMsg,PrgMsg;
  volatile int QryAdr,QryCnt,QryVer;         // Frage die gesendet wird
//  volatile int TrmTimeout;
  volatile bool Quit;
  volatile int PrgCnt;
  volatile int PrgAdr;      //!< Hält die Adresse von der das Programm gelesen wird
  volatile int PrtkAdr;     //!< Hält die Speicheradresse beim Protokollauslesen
  volatile int PrtkLng;     //!< Hält die Anzahl der zu lesenden Bytes
  volatile int PrtkNr;      //!< Hält die Kopfnummer von dem das Protokoll gelesen wird
  TQuittung Quittung;
  unsigned int Vorteiler,MasterSync;
/*! \brief Status ueber die RS232 ausgeben
   \param[in] Prtk Benutzte Protokollschnittstelle
   \param[in] Msg  Typ der Meldung (0x2211 oder 0x2221)
*/
  void trmstatus0(TMTSprotocol &Prtk, unsigned short Msg);
/*! \brief Speicherabfrage mit R-Block
   \param[in] channel  Blockpuffer der RS485
   \param[in] Adr      Adresse an die der R-Block gesendet wird
   \param[in] Abs      Absender des R-Blocks
   \param[in] Daten    Zeiger auf den Datenpuffer fuer Laenge und Adresse
   \param[in] BLng     Anzahl der zu lesenden Bytes
   \param[in] BAdr     Speicheradresse von der gelesen wird
*/
  void makeRblock(PBlock channel, unsigned char Adr,unsigned char Abs, void *Daten,
		                  unsigned char BLng, unsigned short BAdr);
  unsigned short TmpGreen[MAXSGN];
  virtual void SetUlow(int Nr)=0;
  short TimeOutK[MAXSGN];
  short TimeOutR[MAXSGN];
  bool RS485OK;
  bool PRGFLAG;            // ein Program wurde von Kopf 1 geladen
  virtual bool Conflict()=0;
  bool gestartet;
 public:
  TFrage Frage;
  TMsgFlags Flags;
  bool prgavail();
  unsigned char Prg[0x1000];
  short Protokoll[620];        // Aufzeichnung des Signalverlaufs
  char Log[10][20];            // Speicher für Logeintrag
  TMultisignal Daten;
  volatile unsigned long dt;
  volatile int T,StatCnt,dT;
  TMTSprotocol *Prtk;
  volatile unsigned char BlkCnt;

  TState(TMTSprotocol &P);
  volatile bool StatusFlg;  //! true = shortstatus false=status
  void trmstatus(TMTSprotocol &P); //!< 0x2211 ueber die RS232 ausgeben
  void trmshortstatus(TMTSprotocol &P);    //!< 0x2212-Block ueber die RS232 ausgeben
  void trmerror(TMTSprotocol &P);          //!< 0x2221-Block bis Quittung oder max. 3 mal ueber die RS232 ausgeben
  void trmprogram(TMTSprotocol &P, int Offset, int n);
  void trmsignal(TMTSprotocol &P, int n);
  void trmlog(TMTSprotocol &P);
  void trmbusy(TMTSprotocol &P, short Type);
  void trmversion(TMTSprotocol &P);
  virtual void reset();
  virtual void resetK(int n);
  virtual void resetR(int n);
  virtual void execute()=0;
  virtual void processblk(TBlock &Blk)=0;      // für normale Blöcke
  virtual void processblk(TMSNetz &MS)=0;   // für Statusblöcke
  virtual void processRS232()=0;
  virtual int convert(char *P, int Offset, int n)=0;        // Konvertiert das Program auf Prg in ASCII-Text
  virtual void errlog(char *P)=0;         // Konvertiert den Log-Eintrag auf Log in ASCII-Text
  virtual void qrylamps(TBlock &Blk, void *B)=0;
  virtual void qryinfo(TBlock &Blk)=0;
  virtual void qryprg(TBlock &Blk)=0;
  virtual void qryprtk(TBlock &Blk, void *B)=0;
  bool system_running();
};

class TMSState : public TState {
 protected:
  int StartCnt;
  int Pause;
  unsigned short ErrDelay;
	unsigned int Udelay;
	unsigned int Ulimit;
  virtual void SetUlow(int Nr);
  virtual bool Conflict();
	virtual void processblk_k2(TBlock &Blk);
 public:
  TMSState(TMTSprotocol &P);
  virtual void execute();                     //!< State machine fuer die Anfragen an die Ampel (RS485)
  virtual void processblk(TBlock &Blk);       //!< Auswertung von normalen Datenbloecken
  virtual void processblk(TMSNetz &MS);    //!< Auswertung von Statusbloecken
  virtual void processRS232();                //!< Datenverkehr auf der RS232
  virtual int convert(char *P, int Offset, int n);              //!< Konvertiert das Programm in ASCII-Text
  virtual void errlog(char *P);               //!< Konvertiert das Fehlerprotokoll in ASCII-Text
  void qrylamps(TBlock &Blk, void *B);        //!< Fragt mit einem R-Block die Lampen ab
  void qryinfo(TBlock &Blk);                  //!< Fragt mit einem K-block Spannung und Fehler ab
  void qryprg(TBlock &Blk);                   //!< Fragt mit einem R-Block das Programm ab
  void qryprtk(TBlock &Blk, void *B);         //!< Fragt mit R-Bloecken das Protokoll ab
};

class TMSStateNeu : public TMSState {
	protected:
	void processblk_k2(TBlock &Blk);
	public:
	void processblk(TMSNetz &MS);

	TMSStateNeu(TMTSprotocol &P);
};


