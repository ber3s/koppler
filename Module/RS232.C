
#include <cstring>
#include <lpc_fifo.h>
#include <rs232.h>
#include <tools.h>

#define BLOCKANFANG 0x100

/*! Prueft ob ein Block im Empfangspuffer der RS232 vorhanden ist und gibt den Typ zurueck. \n \n
    Rueckgabe:
    <TABLE>
    <TR><TD> 0   </TD><TD> Noch kein vollstaendiger Block vorhanden </TD></TR>
    <TR><TD> -1  </TD><TD> Fehler, Der Speicher wird bis zur naechsten Blockanfangskennung geloescht </TD></TR>
    <TR><TD> Typ </TD><TD> Block vorhanden </TD></TR>
    </TABLE>
*/
int TRS232Rcv::scann() {

  	short cx;
 	unsigned int lng;
	unsigned int n;

	if (OK) {
	  	return Typ;
	}

 	if (fifocnt(Buf) >= (Buf[0]-1)) { 	// Im Puffer ist nur noch ein Zeichen frei (?)
	  	cnt = -1;
		rdfifo(Buf);	// Rückgabwert wird nicht behandelt, gelesenes Zeichen wird verworfen
	};

 	if (cnt < 0) {		// kann mit vorheriger Abfrage zusammengefasst werden
  		cx = rdfifo(Buf);
  		if (cx >= BLOCKANFANG) {
		  	Typ = cx & 0xFF;
			cnt = 0;
			return 0;

		} else if (cx < 0) {
		  	return -1;

		} else {
		  	return 0;
		}

 	} else {
	  	if (cnt >= fifocnt(Buf)) {
			return -1;
		}
  		cx = fifoch(Buf,cnt);
  		// war vor Umschreiben so: if (cx>=BLOCKANFANG) { Typ=cx&0xFF; for (n=cnt+1;n;--n) rdfifo(Buf); OK=false; cnt=0; return 0; };
		if (cx >= BLOCKANFANG) {
		  	Typ = cx & 0xFF;
			for (n = cnt + 1 ; n ; --n) {
			  	rdfifo(Buf);
			}
			OK = false;
			cnt = 0;
			return 0;
		};
 	};

 	switch (cnt++) {
  		// cnt war = -1
		case 0:
		  	CRC = calc_crc(cx, 0);
			break;

  		case 1:
		  	CRC = calc_crc(cx, CRC);
			break;

  		case 2:
		  	if (cx > (Buf[0]/3)) { for (n=cnt+1;n;--n) rdfifo(Buf); cnt=-1; } else { CRC = calc_crc(cx, CRC); };
			break;

  		default:
		  	lng = fifoch(Buf, blLng) + 6;
  			if (cnt == lng) {
			  	CRC -= cx << 8;
			} else if (cnt >= lng + 1) {
    			if ((CRC - cx) == 0) {
				  	OK = true;
					cnt = -1;
					return Typ;
				} else {
				  	for (n = cnt + 1 ; n ; --n) {
					  	rdfifo(Buf);
					}
				  	OK = false;
					cnt = -1;
				};

   			} else {
			  	CRC = calc_crc(cx, CRC);
			}

			break;
 	};

	return 0;
}



void TRS232Rcv::reset() {
 	cnt = -1;
	OK = false;
}




void TRS232Rcv::delblock() {

  	int n = (fifoch(Buf,blLng)+7)&0xFF;

	for (n = n ; n ; --n) {
	  	if (rdfifo(Buf) == 0xAA) {
			rdfifo(Buf);
		}
	};

	OK = false;
}


void TRS232Rcv::read(PBlock Dst) {

  	int n;
 	unsigned short CRC;

	Dst->State = BLK_OFF;
 	Dst->cnt = -5;
 	Dst->Kopf.Adr = rdfifo(Buf);
 	Dst->Kopf.Abs = rdfifo(Buf);
 	Dst->Kopf.Lng = rdfifo(Buf);
 	Dst->Kopf.Nr = rdfifo(Buf);
 	Dst->Kopf.Typ = rdfifo(Buf);

	if (Dst->Buff) {
	  	for (n = 0;n<Dst->Kopf.Lng;++n) {
			(*Dst->Buff)[n] = rdfifo(Buf);
	  	}

	  	CRC = rdfifo(Buf) << 8;
  		CRC += rdfifo(Buf);
  		Dst->CRC = CRC;

	} else {
	  	for (n = 0 ; n < Dst->Kopf.Lng+2 ; ++n) {
		  	rdfifo(Buf);
	  	}
 	};

	OK = false;
}



//! Standardkonstruktor
TRS232::TRS232() {

  	memset(this, 0, sizeof(TRS232));

	// Fifos initialisieren (Empfang, Senden, Kopplerkommandos)
	initfifo(Rcv, sizeof(Rcv));
 	initfifo(Trm, sizeof(Trm));
 	initfifo(Kmd, sizeof(Kmd));

	KMD.Buf = (short*)&Kmd;
 	RCV.Buf = (short*)&Rcv;
 	reset();
}



void TRS232::reset() {

  	AArcv = 0;
	AAtrm = 0;
 	RCV.reset();
	rcvkmd = false;
 	KMD.reset();
}



void TRS232::wrblockanfang(unsigned char cx) {
  	wrfifo(Trm, BLOCKANFANG | cx);
	CRCtrm = 0;
}



void TRS232::wrchar(unsigned char cx) {
// ????? { if (cx==0xAA) wrfifo(Trm,BLOCKANFANG|cx); else wrfifo(Trm,cx); CRCtrm=calc_crc(cx,CRCtrm);
	wrfifo(Trm, cx);
	CRCtrm = calc_crc(cx, CRCtrm);
}



void TRS232::wrblockende() {
  	wrfifo(Trm, (CRCtrm >> 8) & 0xFF);
  	wrfifo(Trm, CRCtrm & 0xFF);
}



short TRS232::getch() {

  	short cx;

	// AA-Prefix (?) zurückgeben, falls vorhanden
	if (AAtrm) {
  		cx = AAtrm;
		AAtrm = 0;

	// sonst: Zeichen aus dem FIFO lesen, ggf. AA-Prefix setzen
 	} else {
  		cx = rdfifo(Trm);
  		if (cx >= BLOCKANFANG) {
   			AAtrm = cx & 0xFF;
			return 0xAA;

  		} else if (cx == 0xAA) {
		  	AAtrm = 0xAA;
		}
 	};

 	return cx;
}


//! Auslesen eines Zeichens aus dem FIFO, Senden des Zeichens über den UART
void TRS232::txd() {

  	// Abholen des Zeichens
  	short cx = getch();

  	if (cx < 0) {
		TxStop();
	} else {
	  	TxD(cx);	// Hardwarezugriff
	}
}




void TRS232::putch(unsigned char cx) {

  	if (AArcv) {
  		AArcv=false;
  		if (cx == 0xAA) {
		  	if (rcvkmd) {
			  	wrfifo(Kmd, 0xAA);
			} else {
			  	wrfifo(Rcv, 0xAA);
			}

  		} else {
		  	//! \todo	Welche Bewandtnis hat das "or"?
   			if ((cx == NET_KMD) or (cx == NET_SOFORT)) {
			  	wrfifo(Kmd, BLOCKANFANG | cx);
				rcvkmd = true;
			} else {
			  	wrfifo(Rcv, BLOCKANFANG | cx);
				rcvkmd = false;
			};
  		};

 	} else {
	  	if (cx == 0xAA) {
		  	AArcv = 1;
		} else if (rcvkmd) {
		  	wrfifo(Kmd,cx);
		} else {
		  	wrfifo(Rcv,cx);
		}
 	};
}


void TRS232::rxd() {

  	while (!RxEmpty()) {
		putch(RxD());
	}
}



void TRS232::write(PBlock Src, unsigned char cx) {

  	int n;
 	wrblockanfang(cx);

	wrchar(Src->Kopf.Adr);
 	wrchar(Src->Kopf.Abs);
 	wrchar(Src->Kopf.Lng);
 	wrchar(Src->Kopf.Nr);
 	wrchar(Src->Kopf.Typ);

	for (n = 0 ; n < Src->Kopf.Lng ; ++n) {
	  	wrchar((*Src->Buff)[n]);
	}

	wrblockende();
}



void TRS232::writekmd(unsigned char Typ, unsigned char Nr) {

  	wrblockanfang(NET_KMD);
 	wrchar(0);                // Adr
 	wrchar(0);                // Abs
 	wrchar(0);                // Lng
 	wrchar(Nr);
 	wrchar(Typ);
	// for (n=0;n<Src->Kopf.Lng;++n) wrchar((*Src->Buff)[n]);
 	wrblockende();
}



void TRS232::wrblock(unsigned char Netz, unsigned char Adr, unsigned char Abs, unsigned char Lng, unsigned char Nr,
                                 unsigned char Typ, void* Daten) {

	int n;
  	wrblockanfang(Netz);
  	wrchar(Adr);
  	wrchar(Abs);
  	wrchar(Lng);
  	wrchar(Nr);
  	wrchar(Typ);

	for (n = 0 ; n < Lng ; ++n) {
	  	wrchar(*((PBuff)Daten)[n]);
	}

	wrblockende();
}




/*void TRS232::makeSQN(TSQNetz &SQ)
{
}
*/


void TRS232::Execute() {
 	if ((fifocnt(Trm) > 0) && (!isTxOn())) {
	  	AAtrm = 0;
		TxStart(getch());
	};
}


/*************** TMTSprotocol **************************/

unsigned char const MTS_flag = 0x7E;
unsigned char const MTS_esc = 0x7D;

typedef enum {
 				rcv_normal,
				rcv_anfang,
				rcv_esc,
				rcv_anfesc
} Trcvstates;



TMTSprotocol::TMTSprotocol() {

  	memset(this, 0, sizeof(TMTSprotocol));
 	initfifo(Rcv, sizeof(Rcv));
 	initfifo(Trm, sizeof(Trm));
 	reset();
}



void TMTSprotocol::reset() {
 	EscTrm = 0;
	EscRcv = 0;
}


//! holt ein Zeichen aus dem Puffer
short TMTSprotocol::getch() {

  	short cx;
 	if (EscTrm) {
  		cx = EscTrm;
		EscTrm = 0;

 	} else {
  		cx = rdfifo(Trm);
  		if (cx >= BLOCKANFANG) {
		  	cx = MTS_flag;

		} else if ((cx == MTS_flag) || (cx == MTS_esc)) {
   			EscTrm = cx ^ 0x20;
			cx = MTS_esc;
  		};
 	};

 	return cx;
}


//! schreibt ein empfangenes Zeichen in den Puffer
void TMTSprotocol::putch(unsigned char cx) {

  	if (EscRcv) {
  		wrfifo(Rcv,(cx ^ 0x20));
		EscRcv = 0;

 	} else if (cx == MTS_flag) {
	  	wrfifo(Rcv, MTS_flag | BLOCKANFANG);

	} else if (cx == MTS_esc) {
	  	EscRcv = 1;

	} else {
	  	wrfifo(Rcv,cx);}
}


//! Gibt die verfügbare Sendepuffergröße zurück
int TMTSprotocol::TrmRest() {
  	return fiforest(Trm);
}



void TMTSprotocol::wrblockanfang(unsigned char cx) {
  	wrfifo(Trm, BLOCKANFANG);
	CRCtrm = 0;
}



void TMTSprotocol::wrchar(unsigned char cx) {
  	wrfifo(Trm, cx);
	CRCtrm = calc_crc(cx, CRCtrm);
}



void TMTSprotocol::wrshort(unsigned short cx) {
  	wrfifo(Trm, cx & 0xFF);
	CRCtrm = calc_crc(cx & 0xFF, CRCtrm);

  	wrfifo(Trm, (cx>>8) & 0xFF);
	CRCtrm = calc_crc((cx>>8) & 0xFF, CRCtrm);
}



void TMTSprotocol::wrint(int cx) {
  	wrchar((cx >> 0) & 0xFF);
  	wrchar((cx >> 8) & 0xFF);
  	wrchar((cx >> 16) & 0xFF);
  	wrchar((cx >> 24) & 0xFF);
}



void TMTSprotocol::wrblockende() {
  	wrfifo(Trm, CRCtrm & 0xFF);        // low byte first
  	wrfifo(Trm, (CRCtrm >> 8) & 0xFF);
  	wrfifo(Trm, 1 | BLOCKANFANG);
}



void TMTSprotocol::Execute() {
 	if ((fifocnt(Trm) > 0) && (!isTxOn())) {
	  	EscTrm = 0;
		TxStart(getch());
	};
}



void TMTSprotocol::txd() {
 	short cx = getch();

	if (cx < 0) {
	  	TxStop();
	} else {
	  TxD(cx);
	}
}



void TMTSprotocol::rxd() {
  	while (!RxEmpty()) {
		putch(RxD());
	}
}


/*! Sucht im Empfangspuffer nach vollstaendigen Bloecken \n
    Rueckgabe:
    <TABLE>
    <TR><TD> 0 </TD><TD> Noch kein vollstaendiger Block vorhanden </TD></TR>
    <TR><TD> ID </TD><TD> Vollstaendiger Block mit Kennung ID vorhanden </TD></TR>
    </TABLE>
*/
int TMTSprotocol::scann() {                 // Liefert die Blockkennung, wenn ein Block vorhanden ist

  	int n,x;
 	bool Flg;

	n = fifocnt(Rcv);
 	while (n && (fifoch(Rcv, 0) != (MTS_flag|BLOCKANFANG))) {
	  	rdfifo(Rcv);
		--n;
	};

	if (n < 7) {
	  	return 0;
	}
 	CRCrcv = 0;
	n = 1;

	do {
  		Flg = (fifocnt(Rcv) > n);

  		if (Flg) {
   			x = fifoch(Rcv, n);
   			if (x != (MTS_flag | BLOCKANFANG)) {
    			CRCrcv = calc_crc(x, CRCrcv);
				++n;

   			} else if ((CRCrcv != 0) || (n == 1)) {
    			rdfifo(Rcv);
				return 0;

   			} else {
    			rdfifo(Rcv);
    			return ((fifoch(Rcv, 0) << 8) | fifoch(Rcv, 1));
   			};
  		};

	} while (Flg);

 	return 0;
}




void TMTSprotocol::delblock() {

  	// lösche so lange Zeichen aus dem Empfangspuffer, wie noch Zeichen vorhanden
	// sind und das erste Zeichen im Puffer != BLOCKANFANG und != MTS_flag ist
	while (fifocnt(Rcv) && (fifoch(Rcv, 0) != (MTS_flag | BLOCKANFANG))) {
		rdfifo(Rcv);
  	}

	// Lösche zwei Zeichen aus dem Puffer, wenn das erste == MTS_flag | BLOCKANFANG ist.
	// Anderenfalls wird nur das erste gelöscht
	if (rdfifo(Rcv) == (MTS_flag | BLOCKANFANG)) {
	  	rdfifo(Rcv);
	}
}




// wird nicht verwendet
//! Liest den Empfangspuffer aus und beschreibt das Ziel Dst
//! liest einen Block aus Rcv und schreibt ihn auf Dst
void TMTSprotocol::read(char* Dst, int n)	{

  	int i = 0;

	// Lese so lange Zeichen aus, wie der Fifo sie noch liefern kann,
	// das erste Zeichen im Fifo gültig ist (!= MTS_flag, != BLOCKANFANG)
	// und die Laufvariable n noch größer als 0 ist
  	while ((fifocnt(Rcv)) && (fifoch(Rcv, 0) != (MTS_flag | BLOCKANFANG)) && n) {
		// Schreibe das ausgelesene Zeichen in Dst, dekrementiere die Laufvariable
	  	Dst[i++] = rdfifo(Rcv);
		--n;
  	};

	// Ist die Laufvariable gleich 0 (die vorherige Schleife wurde nicht durch
	// eine andere Bedigunung unterbrochen), lösche das nächste Zeichen im Fifo
  	if (n == 0) {
		rdfifo(Rcv);  // Flag löschen
	}
}
