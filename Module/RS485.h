#ifndef RS485_H
#define RS485_H

#include <uart.h>
#include <stdint.h>

//! maximale Länge eines Datenblocks
#define BLK_MAXLENGTH			32
//!
#define BLK_MASTERDAT_LENGTH	20
//! maximale Anzahl Teilnehmer im Netzwerk
#define RS485_NET_MAXANZAHL		16
//! Adresse des Microterminals/PC, falls angeschlossen
#define MT_ADR					RS485_NET_MAXANZAHL-1
//!
#define GLB_ADR					0


//! Bits im Statusregister für FAT
#define st_Taste        0x2000  //!< Tastenbit
#define st_Error        0x0010  //!< Errorbit, Ausrufezeichen ein


//! Bits im Statusregister für Multisignal
const unsigned int 	NET_LINE	= 0x8000;	//!< Alle Teilnehmer verbunden
const unsigned int 	NET_RQST	= 0x4000;	//!< Anforderung Datentransfer
const unsigned int 	NET_MT		= 0x1000;	//!< MT ist angesteckt

//#define NET_LINE	0x8000		//!<
//#define NET_RQST	0x4000		//!< Anforderung Datentransfer
#define NET_PRG		0x2000		//!< Gültiges Programm vorhanden
//#define NET_MT	0x1000		//!<
#define NET_OK		0x0800		//!<
#define NET_FREI	0x0400		//!< Teilnehmer zeigt Freigabesignal
#define NET_NEW		0x0200		//!< Neues Programm ist vorhanden
#define NET_QRZ		0x0100		//!< Quarzersatzmode ist erforderlich
#define NET_SENS	0x0080		//!< Sensor
#define NET_TAST	0x0040		//!< Taster
#define NET_SENS2	0x0020		//!< Sensor 2
#define NET_INTERR	0x0010		//!< Interner Fehler liegt vor
#define NET_AUX		0x0008		//!< frei
#define NET_ROT		0x0004		//!<
#define NET_GELB	0x0002		//!<
#define NET_GRUEN	0x0001		//!<

//! Bits im Adressfeld des Netzblocks
#define BLK_REPEAT 	0x40



//! Konfigurationswerte in den Netzparametern
typedef struct {
 	unsigned short 	TRM_TIMEOUT;		//!< Nach dieser Zeit muss das Senden abgeschlossen sein
 	unsigned short 	BLKPAUSE;			//!< Pause zwischen zwei Blöcken
 	unsigned short 	NEWBLK;				//!< Nach dieser Zeit ohne Zeichen wird ein neuer Block im Empfang gestartet
 	unsigned long 	RCV_TIMEOUT_KABEL;	//!< Nach dieser Zeit ohne Zeichen startet der Master ein Ping
 	unsigned long 	RCV_TERMINAL_KABEL;	//!< Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
 	unsigned long 	SLV_TIMEOUT_KABEL;	//!< Nach dieser Zeit ohne gültigen Block macht der Slave Timeout bzw. wird isNetOn=false, wenn nach dieser Zeit kein Master empfangen wurde */
 	unsigned long 	RCV_TIMEOUT_FUNK;	//!< Nach dieser Zeit ohne Zeichen startet der Master ein Ping
 	unsigned long 	RCV_TERMINAL_FUNK;	//!< Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
 	unsigned long 	SLV_TIMEOUT_FUNK;	//!< Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout bzw. wird isNetOn=false wenn nach dieser Zeit kein Master empfangen wurde */
 	bool 			DataCycle;			//!< wenn true darf jeder Teilnehmer sofort Daten senden
 	bool 			DataFlag;			//!< wenn true verriegelt "Flag" den Masterblock nicht, d.h. jeder neue Masterblock wird übergeben, auch wenn der alte noch nicht ausgewertet wurde */
 	bool 			RepeatData;			//!< wenn true werden erst alle fertigen Datenblöcke gesendet, bevor der Statusblock gesendet wird */
	//! \todo Parameter "Repeatdata" muß noch getestet werden
  uint8_t   SubAdr;   //!< Erweiterungsadresse auf MT-Platz '0..7' 0=Alle
} TNetParam;

//! Zeiger auf eine TNetParam-Struktur
typedef const TNetParam* PNetParam;


#define TMSStat uint64_t

/* Bits in MSstate  */
/*
( Status1 )         ( Status2 )                        ( Adr MasterBlock )     ( PrgNr )
80  CONSTANT ^line       80 EQU ^sensor    8000 EQU ^^sensor  80 EQU ^NBlk           80 EQU ^Pr.man
40  EQU      ^rqst       40 EQU ^taster    4000 EQU ^^taster  40 EQU ^repeat   ( bits 10,20,40 = quelng )
20  EQU      ^S1.Prg     20 EQU ^sensor2   2000 EQU ^^sensor2 30 EQU ^Verbx
10  EQU      ^MT         10 EQU ^IntErr    1000 EQU ^^IntErr
 8  EQU      ^OK          8 EQU ^SI         700 EQU ^^Signal  0F EQU ^Teilnx         0F EQU ^Pr.Nr
 4  EQU      ^frei        7 EQU ^S2.Signal  400 EQU ^^rot
 2  EQU      ^S1.New
 1  EQU      ^Q.flg
*/


#pragma pack(1)
//! \brief Multisignal Statusblock
typedef union {
 	//!
  	struct {
  		unsigned short signal:  3;    	//!< 4=rot  2=gelb  1=grün
  		unsigned short SI:      1;    	//!< Signal kommt setzen
  		unsigned short interr:  1;    	//!< Fehler
  		unsigned short sensor2: 1;    	//!<
  		unsigned short taster:  1;    	//!<
  		unsigned short sensor:  1;    	//!<

  		unsigned short quarz:   1;    	//!<
  		unsigned short bitnew:  1;    	//!< neues Programm wartet auf Start
  		unsigned short frei:    1;    	//!< Kopf zeigt Freigabesignal
  		unsigned short OK:      1;    	//!<
  		unsigned short MT:      1;    	//!< MT ist angesteckt
  		unsigned short Prg:     1;    	//!< Steuerung hat ein Programm
  		unsigned short rqst:    1;    	//!< Busanforderung für Datentransfer
  		unsigned short line:    1;    	//!< Teilnehmer hat alle anderen empfangen

  		unsigned char  Abs;				//!<
  		unsigned char  Adr;				//!<
 	};

	unsigned int All;					//!<

} TMS_STATE;
#pragma pack()



//! \brief Multisignal Statusbits
typedef enum {
 				msbit_sensor		= 0x8000, 	//!< Ein Sensorsignal liegt an
 				msbit_taster		= 0x4000,  	//!< Der Taster ist betätigt
 				msbit_sensor2		= 0x2000,  	//!< Am Sensoreingnag 2 liegt ein Signal an
 				msbit_interr		= 0x1000,  	//!< Der Absender hat einen internen Fehler gefunden
 				msbit_rot			= 0x0400,	//!< Rot ist an
				msbit_gelb			= 0x0200,	//!< Gelb ist an
				msbit_gruen			= 0x0100,	//!< Grün ist an
				msbit_line			= 0x0080,	//!< Der Teilnehmer hat alle Netzteilnehmer empfangen
				msbit_rqst			= 0x0040,	//!< Busanforderung für Datentransfer
				msbit_Prg			= 0x0020,	//!< Die Steuerung hat ein gültiges Programm
				msbit_MT			= 0x0010,	//!< Das Mikroterminal ist angesteckt
				msbit_OK			= 0x0008,	//!< Die Steuerung hat keine Fehler gefunden
				msbit_frei			= 0x0004,	//!< Das Freigabesignal ist eingeschaltet
				msbit_new			= 0x0002,	//!< ein neues Programm wurde geladen
				msbit_quarz			= 0x0001	//!< Die Steuerung arbeitet im Quarzbetrieb
} TMS_STATUS_BITS;



//! Betriebszustände der Multisignal
typedef enum {
 				msmode_fehler		= 0xFF,		//!< CONSTANT ~fehler
 				msmode_aus			= 0xFE,		//!< CONSTANT ~aus
 				msmode_ein			= 0xFD,		//!< CONSTANT ~ein
 				msmode_blink		= 0xFC,		//!< DEUTSCH (( FC  CONSTANT ~gbbl  (  )
                        						//!< USA     (( FC  CONSTANT ~blink (  )
 				msmode_rot			= 0xFB,		//!< CONSTANT ~rot
 				msmode_rtbl			= 0xFA,		//!< CONSTANT ~rtbl
 				msmode_gelb			= 0xF9,		//!< CONSTANT ~gelb
 				msmode_rtgb			= 0xF8,		//!< CONSTANT ~rtgb
 				msmode_off			= 0xF7,		//!< CONSTANT ~off    (  nur intern  )
 				msmode_qrz			= 0xF6,		//!< CONSTANT ~qrz
    			        	            		//!< DEUTSCH (( F6 EQU ~sgnkmt   F6 EQU ~MSgrbl   F5 EQU ~grbl (  )
            				            		//!< USA .IF
 				msmode_yebl			= 0xF5,		//!< CONSTANT ~yebl
 				msmode_sgnzwzt		= 0xF4,		//!< CONSTANT ~sgnzwzt  ( Fußgänger )
 				msmode_sgnkmt		= 0xF3,		//!< CONSTANT ~sgnkmt    GRÜNBL ((  F2 EQU ~grbl   F4 EQU ~MSgrbl  (   )
 				msmode_gelb2rot		= 0xF2,		//!< CONSTANT ~gelb->rot
 				msmode_blink2rot	= 0xF1,		//!< CONSTANT ~blink->rot
                		        				//!< F1 EQU ~lastsgn
												//!< .THEN
 				msmode_over			= 0x80,		//!< CONSTANT ~over

 				msmode_OFF			= 0,		//!< CONSTANT  [OFF]
 				msmode_BLINK		= 1,		//!< CONSTANT  [BLINK]
 				msmode_ROTBL		= 2,		//!< CONSTANT  [ROTBL]
 				msmode_ROT			= 3,		//!< CONSTANT  [ROT]
 				msmode_GELB			= 4,		//!< CONSTANT  [GELB]
 				msmode_QRZ			= 5,		//!< CONSTANT  [QRZ]         ( Quarzersatzmode )
 				msmode_AUTO			= 6,		//!< CONSTANT  [AUTO]        ( Betriebszustände )
 				msmode_MAN			= 7,		//!< CONSTANT  [MAN]
 				msmode_GELBBL		= 8,		//!< USA (( 8  CONSTANT  [GELBBL] (  )

 				msmode_sync			= 0xE,		//!< CONSTANT  [sync]        ( kann losgehen )
 				msmode_error		= 0xF		//!< CONSTANT  [error]

} TMS_MODE;

#pragma pack(1)
//typedef enum { QUARZ=0, KABEL, FUNK } TVerbindung;

//! Verbindungstyp
enum TVerbindung {
  				QUARZ				= 0, 		//!< Verbindung über Quarz
				KABEL,   						//!< Verbindung über Kabel
				FUNK     						//!< Verbindung über Funk
};

//! Bits im Adressfeld des Netzblockes
typedef union {
  	//!
  	struct {
   		uint8_t 		abs:    	4;  		//!< Absender
   		TVerbindung verb:    	2;  		//!< Verbindungstyp ::TMSNetz ::TVerbindung
   		uint8_t 		repeat: 	1;  		//!< 1=Der Teilnehmer mit der nächsten Adresse antwortet
   		uint8_t 		net:    	1;  		//!< 1=Statusblock
  	} bit;

  	uint8_t 		byte;					//!<
} TMSNetAdr;



//! Bits im Absenderfeld des Netzblocks
typedef union {
 	//!
  	struct {
  		uint8_t 	glbnr:  	4;  		//!< Bestätigung des letzten globalen Datenblocks
  		uint8_t 	anz:    	4;  		//!< Anzahl der Netzteilnehmer
 	} bit;

	uint8_t 		byte;					//!<
} TMSNetAbs;



/*
//! Daten im Masterblock
typedef union {
 	//!
  	struct {
  		uint8_t 	Cnt;         				//!< Synchronisationszähler im 16ms-Takt bis 2s
  		uint8_t 	PrgNr:  	4;   			//!< Aktuelle Signalprogrammnummer
  		uint8_t 	quelng: 	3;   			//!< Länge der Kommandokette in byte
  		uint8_t 	man:    	1;   			//!< 1 = Manuellmode
  		uint8_t 	mode;        				//!< Aktuelle Betriebsart s. TMS_MODE
 	};

 	uint16_t   		SyncCnt;     				//!< Synchronzähler für FAT
 	uint8_t 				All[BLK_MASTERDAT_LENGTH];	//!<
} TMSNet_MasterDaten;
*/
typedef union {
 struct {
  uint8_t Cnt;         ///< Synchronisationszähler im 16ms-Takt bis 2s
  uint8_t PrgNr:  4;   ///< Aktuelle Signalprogrammnummer
  uint8_t quelng: 3;   ///< Länge der Kommandokette in byte
  uint8_t man:    1;   ///< 1 = Manuellmode
	union {
		struct {
  		uint8_t mode;        ///< Aktuelle Betriebsart s. TMS_MODE
  		uint8_t amp1:   4;   ///< Signale der Quarzampeln
  		uint8_t Nr:     4;   ///< Signale der Quarzampeln
  		uint8_t amp3:   4;   ///< Signale der Quarzampeln
  		uint8_t amp2:   4;   ///< Signale der Quarzampeln
  		uint8_t amp5:   4;   ///< Signale der Quarzampeln
  		uint8_t amp4:   4;   ///< Signale der Quarzampeln
  		uint8_t amp7:   4;   ///< Signale der Quarzampeln
  		uint8_t amp6:   4;   ///< Signale der Quarzampeln
  		uint8_t xxxx:   4;   ///< Signale der Quarzampeln
  		uint8_t amp8:   4;   ///< Signale der Quarzampeln
		};
		struct {
			uint16_t PhZtNeu;
			uint16_t ModeNeu;
		};
		uint16_t  img[3];      ///< Freibits für bis zu 15 Gruppen
	};
 };
 uint16_t SyncCnt;     ///< Synchronzähler für FAT
 uint8_t All[BLK_MASTERDAT_LENGTH];
} TMSNet_MasterDaten;

typedef uint8_t MasterDaten_t[BLK_MASTERDAT_LENGTH];
//! Status des Netzwerkes
typedef struct {
  	TBlockKopf 			Kopf;               		//!< Kopfdaten des Masterblockes
//		uint8_t MasterDaten[BLK_MASTERDAT_LENGTH];  ///< Daten im Masterblock
  	TMSNet_MasterDaten 	MasterDaten;  				//!< Daten im Masterblock
  	TMSStat	  		AND;                     	//!< und Verknüpfung der Statusmeldungen der Teilnehmer über einen Zyklus
  	TMSStat 			OR;                      	//!< oder Verknüpfung der Statusmeldungen der Teilnehmer über einen Zyklus
  	TMSStat 			State[RS485_NET_MAXANZAHL]; //!< Liste der Statusmeldungen der einzelnen Teilnehmer

} TMSNet_State;
#pragma pack()



//!  Implementiert den Zugriff auf das RS485-Netzwerk
class TMSNetz {

	protected:
  	void (TMSNetz::*Mode)();								//!< Zeiger auf den Zustand der Statemachine (Klassenmethode)
  	void (TMSNetz::*Next)();								//!< Zeiger auf den nächsten einzunehmenden Zustand



  	unsigned long 			TimeOutChar;						//!< Timeout für max. Zeit zwischen zwei Zeichen
  	unsigned long 			TimeOutBlk; 						//!< Timeout für max. Zeit zwischen zwei Blöcken, danach Timeout im Slave
  	unsigned char 			Teiln;								//!< Nummer des letzten empfangenen Teilnehmers
  	unsigned long 			RcvTimeout;     					//!< Timeout für Blockanfang im Empfänger
  	unsigned int 			TaktCnt;         					//!< Zähler zum Abzählen bis zum nächsten Start von Execute()
  	PChannels 				RxCh;                      			//!< Zeiger auf die Empfangskanäle
  	PChannels 				TxCh;								//!< Zeiger auf die Sendekanäle
  	unsigned long 			Tx;									//!<
	unsigned long			dt;									//!<
  	bool 					masterblkflg;						//!< true wenn der Masterblock gesendet werden soll
	TBlock 					caDummy;							//!<
  	char 					DummyBuff[BLK_MAXLENGTH];			//!<
  	unsigned long 			Timeout[RS485_NET_MAXANZAHL + 1];	//!<
  	char 					LastChar;							//!<
		TBlock TempRcvBuf;
  	MasterDaten_t TempMasterDaten;  ///< Daten im Masterblock
  	TBlock 					RcvBuf;              				//!< BlockPuffer des Empfängers für Netzblöcke
  	PBlock 					Daten;								//!< Zeiger auf den zu sendenden Datenblock
  	PBlock 					RcvPnt;								//!< Zeiger auf den Block, der gerade Empfangen wird
  	TMSNet_State 			tmp;								//!<

	unsigned char 			DatPnt;    							//!< Nr des Teilnehmers, der Daten senden darf
  	unsigned char 			DataIdx;							//!< Idx des Datenkanals der als letztes Daten gesendet hat
  	unsigned int 			Takt;								//!< Reloadwert zum Abzaehlen bis zum naechsten Start
  	PBlock 					TrmPnt;								//!< Zeiger auf den Block der gesendet wird
  	PNetParam 				Param;								//!<
  	int 					TasteCnt;							//!<

	volatile unsigned char 	_Abs;								//!<
	volatile unsigned char 	_Glb;								//!<
  	volatile bool 			_Rep;								//!<



	unsigned long 	GetRcvTimeOut	(void);					//!< Holt die Empfängertimeoutkonstante
  	unsigned long 	GetSlvTimeOut	(void);					//!< Holt die Slavetimeoutkonstante
  	unsigned long 	GetTrmTimeOut	(void);        			//!< Holt die Sendertimeoutkonstante
  	void 			finddatenblock	(void);
  	bool 			finishstate		(void);
  	void 			storeSelfStatus	(void);
  	void 			doTrmTimeout	(void);
  	bool 			datatosend		(void);
  	unsigned char 	getAdr			(void);
  	unsigned char 	getAbs			(unsigned char Pnt);
  	bool 			TrmRdy			(void);
  	void 			seterrnumber	(char Nr, char Last);
  	TMSStat 		makestatus		(TBlockKopf& Kopf);

  	void 			wait			(void);
  	void 			waittrm			(void);
  	void 			waitdaten		(void);
  	void 			pingantw		(void);
  	virtual void 	dopingantw		(void);
  	void 			dstart			(void);
  	void 			bstart			(void);
  	void 			slvst			(void);
  	void 			slvtrm			(PBlock channel);
  	void 			slvb			(void);
  	void 			msttrm			(void);
  	void 			MTdelay			(void);
  	void 			mstst			(void);
  	void 			ping			(unsigned char PingAdr);
  	void 			MTping			(void);
  	void 			repeat			(unsigned char PingAdr);
// protected:

  	virtual bool 			MTTest		(void) 			{ return false; }; 	//!< Testet ob ein weiters MT angesteckt ist
  	virtual void 			RS485on		(void)			= 0;				//!< RS485 Sender einschalten
  	virtual void 			RS485off	(void)			= 0;				//!< RS485 Sender ausschalten
  	virtual bool 			TxEmpty		(void)			= 0;				//!< Frage ob der Sendepuffer leer ist
  	virtual void 			TxStart		(char Cx)		= 0;				//!< Die Übertragung starten und den Interrupt bei leerem Sendepuffer erlauben

		virtual void 			trmstart	(void);								//!< macht den Netzblock fertig und schickt ihn los
  	void 					trmsofort	(PBlock Pnt);
  	int 					getch		(void);                   			//!< holt ein Zeichen von der Netzlogik
  	int 					putch		(unsigned short cx);  				//!< übergibt das Zeichen an die Netzlogik
  	virtual void 			TxStop		(void)			= 0;				//!< den Interrupt bei leerem Puffer abschalten
  	virtual void 			TxD			(char Cx)		= 0;				//!< Das Zeichen Cx in den Sendepuffer schreiben
  	virtual bool 			RxEmpty		(void)			= 0;				//!< Frage ob der Empfangspuffer leer ist;
  	virtual unsigned short 	RxD			(void)			= 0;				//!< Das nächste Zeichen auslesen;
  	virtual unsigned short 	geterr		(void)			= 0;				//!< wird aufgerufen wenn die Serielle Schnitstelle einen Fehler entdeckt





public:
  /*! @param[in] AParam Zeiger auf eine Parameterstruktur für das Netzwerk
      @param[in] ARxCh  Zeiger auf die Liste der Empfangskanäle
      @param[in] ATxCh  Zeiger auf die Liste der Sendekanäle
      @param[in] ATakt  Anzahl der Interruptaufrufe bis Execute() ausgeführt wird
      @param[in] Abs    Absender (Baugruppennummer) Hardware (1..14)
      @param[in] Verb   Verbindungstyp
      @param[in] Anz    Anzahl der Hardware-Netzteilnehmer
  */
  TMSNetz(PNetParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,
          TVerbindung Verb,
          unsigned char Anz);

  	TBlock 			TrmBuf;         //!< BlockPuffer des Senders
  	PBlock 			sofort;			//!< Block wird sofort gesendet;
  	TVerbindung 	VerbTyp;   		//!< Verbindungstyp
  	unsigned char 	Anzahl;    		//!< Anzahl der Netzteilnehmer
  	TMSNet_State 	State;			//!< Eigener Status
  	char 			ErrNumber;		//!< Fehlernummer
  	bool 			Flag;       	//!< Verriegelt das Überschreiben, Gibt an, dass ein neuer Status vorliegt
  	unsigned char 	Self;      		//!< eigene Netzadresse
  	unsigned char 	GlobalNr;  		//!< Nr des letzten gesendeten bzw. empfangenen globalen Datenblocks
  	unsigned int 	GlobalQuit;    	//!< Quittung für den zuletzt gesendeten globalen Datenblock, 1 Bit pro Teilnehmer

	bool 			MTconflict;		//!<
  	int 			MasterBlkCnt;	//!< zählt wie oft der Masterblock empfangen wurde. Max. 1000

  	void Execute(void);                       //!< Schaltet die State-Machine weiter
  	virtual void trmnext(void);                       /*!< \brief Nächstes Zeichen senden.

                                             	Ermittelt, ob noch ein Zeichen zum Senden da ist und gibt
                                             	es aus. Ist ein Block vollständig gesendet wird dieser auf
                                             	BLK_TRM gesetzt und nach dem nächsten Block gesucht. \n
                                             	Muss im Sendeinterrupt aufgerufen werden. */
 	virtual void 			rcvnext			(void);  /*!< \brief Nächstes Zeichen abholen.

                                             	Holt ein Zeichen von der SIO und gibt es an die Blocklogik weiter \n
                                             	Muss im Empfangsinterrupt aufgerufen werden. */
  	unsigned short 	clrerr			(void);
    void trmchar(char ch);
		bool 			ismaster		(void);
  	bool 			hasMT			(void);
  	bool 			isTeilnOn		(int Nr);
  	bool 			isNetOn			(void);
  	bool 			isTaste			(void);
  	virtual void 	initports		(void)		= 0;	//!< initialisiert die Hardware

	void (*OnTxStart)(TMSNetz* NET);					//!< wird aufgerufen, nachdem das erste Zeichen (cx) eines Blockes gesendet wurde
  	void (*OnGetMasterDat)(TMSNetz* NET);				//!< wird aufgerufen, bevor der Masterblock gesendet wird
//  	TMSStat (*OnGetState)(TMSNetz* NET);				//!< wird aufgerufen bevor der Statusblock gesendet wird
	  TMSStat (*OnGetState)(TMSNetz *NET, int & Lng);	///< wird aufgerufen bevor der Statusblock gesendet wird
  	void (*OnNetRcv)(TMSNetz* NET,TMSNet_State& State);	//!< wird aufgerufen nachdem ein Netzblock empfangen wurde
	  void (*OnMasterRcv)(TMSNetz *NET,TMSNet_State &State);	///< wird aufgerufen nachdem ein Netzblock empfangen wurde
		bool ExtFlag;
};







/*! Die Klasse TMSNetzMTTest erweitert TMSNetz um einen Test, ob ein weiteres Terminal angesteckt ist
 	Wird vom Ayantra-Interface benutzt, da das Interface auf der gleichen Adresse arbeitet
 	wie das Terminal                                                                                    */
class TMSNetzMTTest : public TMSNetz {

protected:
  	TMSNetzMTTest(PNetParam AParam, PChannels ARxCh, PChannels ATxCh, unsigned int ATakt, unsigned char Abs, TVerbindung Verb, unsigned char Anz)
   		: TMSNetz(AParam, ARxCh, ATxCh, ATakt, Abs, Verb, Anz) {
		// kein weiterer Inhalt
	};

   	int MTcntrl;
	unsigned int MTcnt;

  	virtual bool MTTest(); // Testet ob ein weiters MT angesteckt ist
};





/*! Die Klasse TMSNetzAyantra antwortet nicht auf ein MTPing, solange kein Masterblock empfangen wurde
	Damit wird verhindert, dass ein MT erkannt wird, wenn die MT-Brücke gesetzt ist
	wie das Terminal                                                                                      */
class TMSNetzAyantra : public TMSNetzMTTest {

protected:
  	TMSNetzAyantra(PNetParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,TVerbindung Verb, unsigned char Anz)
   		: TMSNetzMTTest(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) {
		// kein weiterer Inhalt
	};

	virtual void dopingantw();
};

#endif // RS485_H
