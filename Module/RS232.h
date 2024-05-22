
#ifndef RS232_H
#define RS232_H

#include <uart.h>
#include <iolpc2138.h>

//! Konstanten zur Identifikation der Blöcke, die vom Rechner kommen
typedef enum {
 				NET_NORMAL	= 1,		//!< Block wird normal über die RS485 gesendet
 				NET_SOFORT,				//!< Block wird sofort gesendet
 				NET_KMD		= 0xFF		//!< Kmd für den Koppler, Block wird nicht gesendet
} TNetTypes;


typedef enum {
 				blAdr		= 0,		//!<
 				blAbs,					//!<
				blLng,					//!<
				blNr,					//!<
				blTyp					//!<
} TBlockbytes;





class TRS232Rcv {

private:
  short 			cnt;			//!<
  int   			Typ;			//!<
  unsigned short 	CRC;			//!<
  bool 				OK;				//!<

public:
  short*			Buf;			//!<

  int scann			(void);
  void reset		(void);
  void delblock		(void);
  void read			(PBlock Dst);	// liest einen Block aus Rcv und schreibt ihn auf Dst
};



class TRS232 {

private:
  	//! holt das nächste Zeichen zum Senden  -1 Trm ist leer
  	virtual short 			getch(void);
  	//! Schreibt das empfangene Zeichen nach Rcv
	virtual void 			putch(unsigned char cx);
  	//! Sendeinterrupt erlauben und erstes Zeichen in den Puffer schreiben
	virtual void 			TxStart(unsigned char cx)	= 0;
  	//! Fragt ob der Sendepuffer leer ist
	virtual bool 			TxEmpty(void)				= 0;
  	//! Schreibt cx in den Sendepuffer
	virtual void 			TxD(char Cx)				= 0;
  	//! Schaltet den Sendeinterrupt aus
	virtual void 			TxStop(void)				= 0;
  	//! Fragt nach ob der Empfangspuffer leer ist
	virtual bool 			RxEmpty(void)				= 0;
  	//! Liest den Empfangspuffer aus
	virtual unsigned char 	RxD(void)					= 0;
  	//! Fragt nach ob der Sendeinterrupt eingeschaltet ist
	virtual bool 			isTxOn(void)				= 0;


protected:
  	bool 			rcvkmd;			//!< gesetzt, wenn in den Kommandopuffer geschrieben wird
  	unsigned short 	CRCtrm;			//!<
  	short 			AArcv;			//!< Flags für den AA-Prefix
	short 			AAtrm;			//!< Flags für den AA-Prefix
  	short 			Rcv[200];		//!< Empfangspuffer für Blöcke für die Ampeln
  	short 			Kmd[150];		//!< Empfangspuffer für Kopplerkommandos


public:

  	short 			Trm[1000];		//!< Sendepuffer
  	TRS232Rcv 		RCV;			//!< Empfangspuffer fuer normale Bloecke
  	TRS232Rcv 		KMD;			//!< Empfangspuffer für Kopplerkommandos


//  TRS232(unsigned char AMask, unsigned char AAdresse);
//  unsigned char Mask,Adresse;

	// Konstruktor
  	TRS232();





	void 			rxd				(void);		 			  	// Empfangsinterrupt
  	void 			txd				(void);						// Sendeinterrupt
  	short 			clrerror		(void) 	{ return 0; }; 		//=0;	// Fehlerbehandlung
  	void 			reset			(void);
	//  schreibt einen Block mit [AA NET_KMD 0 0 0 Nr Typ]
	void 			writekmd		(unsigned char Typ, unsigned char Nr);

  	//! Statemaschine aufrufen
	virtual void 	Execute			(void);
	//! schreibt den Block Src in den Trm FIFO
	virtual void 	write			(PBlock Src, unsigned char cx);
  	virtual void 	wrblock			(unsigned char Netz, unsigned char Adr, unsigned char Abs, unsigned char Lng, unsigned char Nr, unsigned char Typ, void* Daten);
  	virtual void 	wrblockanfang	(unsigned char cx);
  	virtual void 	wrchar			(unsigned char cx);
  	virtual void 	wrblockende		(void);

};





class TMTSprotocol {

private:
  	virtual short 			getch		(void);		        			// holt das naechste Zeichen zum Senden  -1 Trm ist leer
  	virtual void 			putch		(unsigned char cx);	        	// Schreibt das empfangene Zeichen nach Rcv
  	virtual void 			TxStart		(unsigned char cx)		= 0;	// Sendeinterrupt erlauben und erstes Zeichen in den Puffer schreiben
  	virtual bool 			TxEmpty		(void)					= 0;    // Fragt ob der Sendepuffer leer ist
  	virtual void 			TxD			(char Cx)				= 0;    // Schreibt cx in den Sendepuffer
  	virtual void 			TxStop		(void)					= 0;    // Schaltet den Sendeinterrupt aus
  	virtual bool 			RxEmpty		(void)					= 0;    // Fragt nach ob der Empfangspuffer leer ist
  	virtual unsigned char 	RxD			(void)					= 0;    // Liest den Empfangspuffer aus
  	virtual bool 			isTxOn		(void)					= 0;    // Fragt nach ob der Sendeinterrupt eingeschaltet ist

protected:
  	unsigned short 		CRCtrm;					//!<
  	unsigned short 		CRCrcv;					//!<
  	int 				rcvcnt;					//!<
  	short 				EscTrm;					//!<
	short 				EscRcv;					//!<

public:
  	short 				Rcv[100];		        //!< Empfangspuffer für Bloecke fuer die Ampeln
  	short 				Trm[1000];		        //!< Sendepuffer

	// Konstruktor
	TMTSprotocol();


	void 					rxd				(void);		                //!< Empfangsinterrupt
  	void 					txd				(void);			        	//!< Sendeinterrupt
  	inline short 			clrerror		(void) 		{ return 0; }; 	//!< Fehlerbehandlung
  	void					reset			(void);			        	//!< Setzt die Esc-Werte zurück
  	int 					TrmRest			(void);            			//!< Gibt die verfuegbare Sendepuffergroesse zurueck
  	int 					scann			(void);             		//!< Liefert die Blockkennung, wenn ein Block vorhanden ist.
  	void 					delblock		(void);              		//!< Loescht eine Block aus dem Empfangspuffer
  	void 					read			(char* Dst, int n);			//!< liest einen Block aus Rcv und schreibt ihn auf Dst

	virtual void 			Execute			(void);	        			//!< Statemaschine aufrufen
  	virtual void 			wrblockanfang	(unsigned char cx); 		//!< schreibt den Blockanfang und cx in den FiFo CRCtrm=0
  	virtual void 			wrchar			(unsigned char cx);      	//!< schreibt cx in den FiFo und berechnet CRCtrm
  	virtual void 			wrshort			(unsigned short cx); 		//!< schreibt 2 Byte (low byte first) in den FiFo und berechnet CRCtrm
  	virtual void 			wrint			(int cx);            		//!< schreibt 4 Byte (low byte first) in den FiFo und berechnet CRCtrm
  	virtual void 			wrblockende		(void);              		//!< schreibt CRCtrm (low byte first) in den FiFo und das Blockende

};

#endif // RS232_H
