// ***************************************************************************
//
// RS485 Netzschnittstelle
// 01.02.2006
//
//****************************************************************************

#include <RS485.h>
#include <tools.h>
#include <timer.h>

#include <cstring>



/*****************************************************************************
*  Netzsoftware
*  06.02.2006
* ( NetzMode  )
* 0 EQU [inaktiv]
* 1 EQU [mstst]     ( Master sucht nächsten Datentransfer, ggf. Datenübertragung starten )
* 2 EQU [slvst]     ( Slave startet Datenübertragung )
* 3 EQU [dtrm]      ( Daten werden gesendet )
* 4 EQU [start]     ( Master startet Statusblock )
* 5 EQU [mtrm]      ( Warten bis Statusblock gesendet )
* 6 EQU [wait]      ( Warten auf Statusmeldung oder Datenblock )
* 7 EQU [wmt]       ( Lücke für MT )
* 8 EQU [bstart]    ( Startet die Ausgabe des Statusblocks )
* 9 EQU [dstart]    ( Startet Daten und danach Statusblock )
* A EQU [slvtrm]    ( Datenblock wird ausgegeben ohne anschl. Statusblock )
* B EQU [pingantw]  ( Antwortet auf einen Ping )
* C EQU [wstart]    ( Läßt eine Pause zwischen Daten und Statusblock )
* D EQU [mtstart]   ( Datenblockausgabe wird gestartet ohne anschl. Statusblock )
* E EQU [mtping]    ( MT Ping wird ausgegeben )
* F EQU [rptping]   ( Zyklus wird nach TO mit dem übernächsten Teilnehmer neu gestartet )
*
* Netzblock

Adr
	80	^NBlk		1=Netzblock, 0=Datenblock
	40	^repeat		1=nächster Teilnehmer sendet automatisch seinen Status
	20	^Verbx		00=Quarz, 01=Kabel, 10=Funk
	10	--
	08	^Teilnx		Absender
	04	--			wenn =0 Wiederholung der Abfrage mit dem Teilnehmer in ^GlbNr
	02	--
	01	--

Abs
	80	^Anz		Anzahl der Netzteilnehmer (nur im Masterblock)
	40	--
	20	--
	10	--
	08	^GlbNr		- TeilnNr der Daten senden darf  (Masterblock)
	04	--			- Nr des globalen Datenblocks der empfangen wurde (Slaveblock)
	02	--			- Adresse des Teilnehmers der antworten soll bei Ping oder
	01	--	  		Neustart (Masterblock)

Lng
	alle Bits		Länge des Statusblocks

Stat1
	80				leer
	40	^rqst		Anforderung für Datenverkehr
	20	^S1.Prg		Gültiges Programm vorhanden
	10	^MT			Microtreminal ist angesteckt
	08	^OK			Teilnehmer ist OK
	04	^frei		Teilnehmer zeigt Freigabesignal
	02	^S1.New		Neues Programm ist vorhanden
	01	^Q.flg		Quarzersatzmode ist erforderlich


Stat1
	80	^Sensor		Sensorsignal liegt an  (Sensor auf Masse)
	40	^taster		Anforderungstaster  (Taster auf Masse)
	20	^sensor2	Sensorsignal 2 (Sensor auf Rückmeldung)
	10	^IntErr		Interner Fehler liegt vor.
	08	^aux2		frei
	04	^rot
	02	^gelb
	01	^grün


----------------------- nur Masterblock ----------------------
*****************************************************************************/
/***  braucht max. ca 140µs ****/
void TMSNetz::Execute() {

  	int i;
  	unsigned long temp;

	if ((++TaktCnt < Takt) && (Mode != &TMSNetz::waittrm)) {
	  	return;
	}

	TaktCnt = 0;
  	temp = Tx;
	Tx = SystemZeit();
	dt = Tx - temp;
	if (dt > 0x7FFFFFFFUL) {
	  	dt = (~dt) + 1;
	}

	dTime(&RcvTimeout);  // muss gemacht werden, da die Zeit im RxD Interrupt gebraucht wird

	if (TimeOutChar < 0x80000000) {
    __disable_interrupt();
		TimeOutChar += dt;   //dTime(&TimeStampChar);
    __enable_interrupt();
	}
  	if (TimeOutBlk < 0x80000000) {
    __disable_interrupt();
	  	TimeOutBlk += dt;   //dTime(&TimeStampBlock);
    __enable_interrupt();
	}

	for (i = 1 ; i <= RS485_NET_MAXANZAHL ; ++i) {
	  	if (Timeout[i] < 0x80000000) {
		  	Timeout[i] += dt;
		}
	}

	if (Mode) {
	  	(this->*Mode)();
	}

}


TMSNetz::TMSNetz(PNetParam AParam, PChannels ARxCh, PChannels ATxCh, unsigned int ATakt, 	unsigned char Abs,
                 TVerbindung Verb,//unsigned char Verb,
                 unsigned char Anz)			{

	memset(this, 0, sizeof(TMSNetz));
   	Param = AParam;
   	RxCh = ARxCh;
   	TxCh = ATxCh;
   	Takt = ATakt;
   	Self = Abs;
   	VerbTyp = Verb;
   	Anzahl = Anz;
   	Timeout[0] = GetRcvTimeOut();
	//   Timeout[1]=0x80000000;
   	TimeOutChar = 0;
   	TimeOutBlk = 0;

	if (Abs == 1) {
	  	mstst();
	} else {
	  	Mode = &TMSNetz::wait;
	}

	MasterBlkCnt = 0;
   	TasteCnt = 0;
	 ExtFlag=false;

}



unsigned long TMSNetz::GetTrmTimeOut() {

  	if (VerbTyp == FUNK) {
		return Param->RCV_TERMINAL_FUNK;
  	} else {
		return Param->RCV_TERMINAL_KABEL;
	}

}



unsigned long TMSNetz::GetSlvTimeOut() {

  	if (VerbTyp == FUNK) {
	  	return Param->SLV_TIMEOUT_FUNK;
	} else {
	  	return Param->SLV_TIMEOUT_KABEL;
	}

}



unsigned long TMSNetz::GetRcvTimeOut() {

  	if (VerbTyp == FUNK) {
	  	return Param->RCV_TIMEOUT_FUNK;
	} else {
	  	return Param->RCV_TIMEOUT_KABEL;
	}

}



//! \brief 	holt das nächste Zeichen, das gesendet werden soll
int TMSNetz::getch() {

  	char cx;
 	PBlock pnt;
 	short cnt;

 	pnt = TrmPnt;
 	if (pnt == 0) {
	  	return -1;
	}

	if (pnt->State == BLK_READY) {
	  	pnt->State = BLK_TRM;
	}

	if (pnt->State != BLK_TRM) {
	  	return -2;
	}


	switch (pnt->cnt) {
  		case -5:  cx = pnt->Kopf.Adr; 	pnt->CRC = calc_crc(cx, 0); 		pnt->cnt = -4; 	break;
  		case -4:  cx = pnt->Kopf.Abs; 	pnt->CRC = calc_crc(cx, pnt->CRC); 	pnt->cnt = -3; 	break;
  		case -3:  cx = pnt->Kopf.Lng; 	pnt->CRC = calc_crc(cx, pnt->CRC); 	pnt->cnt = -2; 	break;
  		case -2:  cx = pnt->Kopf.Nr;  	pnt->CRC = calc_crc(cx, pnt->CRC); 	pnt->cnt = -1; 	break;
  		case -1:  cx = pnt->Kopf.Typ; 	pnt->CRC = calc_crc(cx, pnt->CRC); 	pnt->cnt = 0;  	break;

		default:
		  	cnt = pnt->cnt;

           	if (cnt == pnt->Kopf.Lng) {
			  	cx = (pnt->CRC >> 8) & 0xFF;

			} else if (cnt == pnt->Kopf.Lng + 1) {
			  	cx = (pnt->CRC & 0xFF);

//			} else if (cnt == pnt->Kopf.Lng + 2) {
//          cx = 0x00;

			} else if (cnt > pnt->Kopf.Lng + 1) {
            	if (pnt->Kopf.Adr == GLB_ADR) {
             		if (ismaster()) {
					  	GlobalNr = pnt->Kopf.Nr;
						GlobalQuit = 0;
					};
            	};

				pnt->State = BLK_TRMOK;
            	TrmPnt = 0;
				TimeOutBlk = 0;
	    		return BLK_TRMOK;

           	} else {
            	cx = (*pnt->Buff)[cnt];
            	pnt->CRC = calc_crc(cx, pnt->CRC);
           	};

			++pnt->cnt;
			break;

	};


 	TimeOutChar = 0;
 	return cx;
}




void TMSNetz::seterrnumber(char Nr, char Last) {
  	ErrNumber = Nr;
	LastChar = Last;
}



//! \brief	speichert ein empfangenes Zeichen
int TMSNetz::putch(unsigned short cx) {

  PBlock rcvchannel;
 	int n;
 	short cnt;
 	PBlock P;

 	if (Mode == &TMSNetz::waittrm) {
	  	return -6;  // Es wird gerade der eigene Netzblock gesendet
	}

  TimeOutChar=0;
  if (RcvPnt==(PBlock)1) return -100;
 	rcvchannel = RcvPnt;

	if (rcvchannel==0) {

	  	if (cx & 0x80) {
   			if (TempRcvBuf.State == BLK_OK) {
			  	seterrnumber(1, cx);
					RcvPnt=(PBlock)1; // Blockiert den Empfang bis zum nächsten Blockanfang
					return -5;
				};
   		rcvchannel = &TempRcvBuf;
			TempRcvBuf.Buff = (uint8_t(*)[1])&TempMasterDaten;
			TempRcvBuf.LngX = BLK_MASTERDAT_LENGTH;

  		} else {
   			n = 0;
			rcvchannel = &caDummy;
			caDummy.LngX = BLK_MAXLENGTH;
			caDummy.Buff = (unsigned char(*)[1])&DummyBuff;

			do {
    			P = *RxCh[n];
    			if (((P->AdrX) == (cx & P->Msk)) && (P->State == BLK_WAIT)) {
				  	rcvchannel = P;
					break;
				};
   			} while (*RxCh[++n]);

		};

   		rcvchannel->cnt = -5;
		RcvPnt = rcvchannel;
  		if (rcvchannel == &caDummy) {
		  	seterrnumber(2, cx);
		}

	} else if (rcvchannel->State != BLK_RCV) {
  		RcvPnt = 0;
		seterrnumber(3, cx);
  		return -4;
 	};

	cnt = rcvchannel->cnt;

// if (cnt==-5) {
//  if ((cx & 0x80)==0) xxx[xxi++]=(cx | 0xFF00);
// } else if ((rcvchannel->Kopf.Adr & 0x80)==0) xxx[xxi++]=cx;

 	switch (cnt) {
   		case -5: rcvchannel->CRC = calc_crc(cx, 0); 				rcvchannel->Kopf.Adr = cx; 	rcvchannel->cnt = -4; 	rcvchannel->State=BLK_RCV; 	break;
   		case -4: rcvchannel->CRC = calc_crc(cx, rcvchannel->CRC); 	rcvchannel->Kopf.Abs = cx; 	rcvchannel->cnt = -3; 								break;
   		case -3: rcvchannel->CRC = calc_crc(cx, rcvchannel->CRC);
            if (cx <= rcvchannel->LngX) {
			  	rcvchannel->Kopf.Lng = cx;
				rcvchannel->cnt = -2;

			} else {
			  	rcvchannel->State = BLK_WAIT;
				RcvPnt = 0;
				seterrnumber(4, cx);
				return -3;
			};
            break;

		case -2: rcvchannel->CRC = calc_crc(cx, rcvchannel->CRC); 	rcvchannel->Kopf.Nr = cx; 	rcvchannel->cnt=-1; 	break;
   		case -1: rcvchannel->CRC = calc_crc(cx, rcvchannel->CRC); 	rcvchannel->Kopf.Typ = cx; 	rcvchannel->cnt=0; 		break;

		default:
		  if (cnt == rcvchannel->Kopf.Lng) {
				rcvchannel->CRC -= cx << 8;

		  } else if (cnt>rcvchannel->Kopf.Lng) {
        rcvchannel->CRC -= cx;
        if (rcvchannel->CRC != 0) {
				 	rcvchannel->State = BLK_WAIT;
					RcvPnt = 0;
					seterrnumber(5, cx);
					return -2;
				};

				if (rcvchannel->Kopf.Adr == GLB_ADR) {
				  if (!ismaster()) {
						GlobalNr = rcvchannel->Kopf.Nr;
				  }
				};

				if (RcvPnt==&TempRcvBuf) {
					RcvBuf=TempRcvBuf;
					memcpy(&tmp.MasterDaten,&TempMasterDaten,sizeof(tmp.MasterDaten));
					RcvBuf.Buff=(uint8_t(*)[1])&(tmp.MasterDaten);
					TempRcvBuf.State=BLK_WAIT;
					RcvBuf.State=BLK_OK;
				} else {
					rcvchannel->State = BLK_OK;
				};
				RcvPnt = 0;
       	return BLK_OK;

			} else {
             	rcvchannel->CRC = calc_crc(cx, rcvchannel->CRC);
             	(*rcvchannel->Buff)[cnt] = cx;
            };

			++rcvchannel->cnt;
			break;

	};

	return 0;
}

void TMSNetz::trmnext(void) {

  	int temp;

	// Sendezeiger ungültig: Übertragung anhalten
	if (!TrmPnt) {
	  TxStop();

	// Zeichen aus dem Fifo auslesen und übertragen
	} else {
	  	temp = getch();
		if (temp >= 0) {
		  	TxD(temp);
		} else {
		  	TxStop();
		}
	};
}



unsigned short TMSNetz::clrerr(void) {

  	unsigned short temp = geterr();
  	RxD();
  	return temp;
}



void TMSNetz::rcvnext(void)
{
 if (dTime(&RcvTimeout)>Param->NEWBLK) {
  if (RcvPnt>(PBlock)100) RcvPnt->State=BLK_OFF;
	RcvPnt=0;
 };
 setTimeStamp(&RcvTimeout);
 while (!RxEmpty()) putch(RxD());
}


//! macht den Netzblock fertig und schickt ihn los
void TMSNetz::trmstart() {

  	short temp = getch();		// erstes Zeichen holen

	if (temp >= 0) {
   		RS485on();
   		TxStart(temp);
		if (OnTxStart) {
		  	OnTxStart(this);
		}
  }; // else TxRS485stop();
}



//! sucht einen Datenblock zum Senden
void TMSNetz::finddatenblock(void){

  	unsigned char k;
	// if (NET->State.State[NET->Self] & NET_RQST) return;

	if (Daten) {
	  	State.State[Self] |= NET_RQST;
		return;
	};

  	k = DataIdx;

	do {
   		++k;
		if (!(*TxCh[k])) {
		  	k = 0;
		}
   		if ((*TxCh[k])->State == BLK_READY) {
    		DataIdx = k;
			Daten = *TxCh[k];
    		State.State[Self] |= NET_RQST;
    		return;
   		};

	} while (DataIdx != k);

	Daten = 0;
  	State.State[Self] &= ~NET_RQST;
}


bool TMSNetz::ismaster() {
  	return (Self == 1);
}


bool TMSNetz::hasMT() { // return (State.OR & NET_MT);
  return true;
}



bool TMSNetz::isTeilnOn(int Nr) {
 return (Timeout[Nr] < GetSlvTimeOut());
}


// #pragma optimize=none
bool TMSNetz::finishstate() {

	unsigned int i;
  	// Test ob Status schon ausgewertet ist
	if (Flag && !Param->DataFlag) {
		return false;
	}

	State = tmp;

	if (State.OR & st_Taste) {
		TasteCnt = 3;
	} else if (TasteCnt) {
		--TasteCnt;
	}
	State.State[Self] |= NET_LINE;
	Timeout[Self] = 0;
	for (i = 1 ; i <= Anzahl ; i++) {
		if (State.State[i] & NET_LINE) {
			Timeout[i] = 0;
		}
		tmp.State[i] &= ~NET_LINE;
	};
	tmp.AND = 0xFFFFFFFF;
	tmp.OR = 0;

	return true;
}



//! fasst den Block-Kopf in einer Variablen zusammen (NET_LINE = 0x8000, entspricht gesetztem höchstem Bit in Kopf.Nr)
TMSStat TMSNetz::makestatus(TBlockKopf& Kopf) {
{ TMSStat result;
  result=((TMSStat)Kopf.Adr<<40);
	result|=((TMSStat)Kopf.Abs<<32);
  result|=((TMSStat)Kopf.Nr<<8);
	result|=(TMSStat)Kopf.Typ;
	result|=NET_LINE;
	result|=(((TMSStat)Kopf.Ext)<<16);
  return result;
}
}



void TMSNetz::storeSelfStatus() {

  	TMSNetAdr cx;
  	TMSStat St;
  	cx.byte = TrmBuf.Kopf.Adr;

	if (Self == cx.bit.abs) {
   		St = makestatus(TrmBuf.Kopf);
   		tmp.State[Self] = St;	// Slavestatus
   		tmp.OR |= St;

		if (Self == MT_ADR) {
    		tmp.AND &= (St | ~(NET_RQST | NET_LINE));
   		} else {
		  	tmp.AND &= St;
		}
  	};
}



void TMSNetz::doTrmTimeout() {
  	TimeOutBlk = 0;
  	RS485off();
  	Mode = &TMSNetz::wait;
}



bool TMSNetz::datatosend() {
  	return (((DatPnt == Self) || Param->DataCycle) && (Daten));
}



unsigned char TMSNetz::getAdr() {

  	TMSNetAdr Adr;
  	Adr.bit.net = 1;
	Adr.bit.verb = VerbTyp;
	Adr.bit.abs = Self;
	Adr.bit.repeat = 1;

	return Adr.byte;
}




unsigned char TMSNetz::getAbs(unsigned char Pnt) {

  	TMSNetAbs Abs;
  	Abs.bit.glbnr = Pnt;
	Abs.bit.anz = Anzahl;

	return Abs.byte;
}



bool TMSNetz::TrmRdy() {
  	return (TxEmpty() && ((!TrmPnt) || (TrmPnt->State != BLK_TRM) && (TrmPnt->State!=BLK_READY)));
}



void TMSNetz::waittrm() {

  	if (TimeOutBlk>=Param->TRM_TIMEOUT) {
		doTrmTimeout();

	} else if (TrmRdy()) {
   		TimeOutBlk = 0;
   		RS485off();
   		Mode = &TMSNetz::wait;
  	};
}



void TMSNetz::waitdaten() {

  	if (TimeOutBlk >= Param->TRM_TIMEOUT) {
		doTrmTimeout();

  	} else if (TrmRdy()) {
   		TimeOutBlk = Param->BLKPAUSE;
   		Daten = 0;

		if (Next) {
		  	(this->*Next)();
		} else {
		  	RS485off();
			Mode = &TMSNetz::wait;
		};
  };
}



bool TMSNetz::isNetOn() {
  	return (Timeout[1] < GetSlvTimeOut());
}



bool TMSNetz::isTaste() {
  	return ((TasteCnt > 0) && isNetOn());
//  return ((State.OR & st_Taste) && isNetOn());
}



void TMSNetz::dstart() {

  	if (TimeOutBlk >= Param->BLKPAUSE) {
    	TimeOutBlk = 0;
    	TrmPnt = Daten;
    	trmstart();
    	Mode = &TMSNetz::waitdaten;
   	};
}


void TMSNetz::bstart() {

  	if (TimeOutBlk >= Param->BLKPAUSE) {
   		TrmPnt = &TrmBuf;
   		TimeOutBlk = 0;

		if (masterblkflg) {
    		masterblkflg = false;
    		// Funktionszeiger gültig
			if (OnGetMasterDat) {
			  	OnGetMasterDat(this);
			}
   		};

		trmstart();
   		Mode = &TMSNetz::waittrm;
  	};
}



void TMSNetz::pingantw() {
  	dopingantw();
}



void TMSNetz::dopingantw()
{  TMSStat Stat=0;
   TMSNetAdr Adr;
   static unsigned short Ext;
	 int Lng=0;
   if (Daten) {
    Next=&TMSNetz::pingantw; Mode=&TMSNetz::dstart;
   } else {
//    if (OnGetState) Stat=(OnGetState(this) & ~(NET_RQST | NET_LINE)); else Stat=0;
    if (OnGetState) Stat=OnGetState(this,Lng); else Stat=0;
    Stat|=(State.State[Self] & (NET_RQST | NET_LINE));
	  Ext=Stat>>16;
		TrmBuf.Kopf.Ext=Ext;
		if (Self==MT_ADR) Stat=Stat & ~((TMSStat)7) | Param->SubAdr;
		makeblock(&TrmBuf,getAdr(),getAbs(GlobalNr),Lng,Stat>>8,Stat&0xFF,&Ext);
    storeSelfStatus();
    Adr.byte=TrmBuf.Kopf.Adr; Adr.bit.repeat=0; TrmBuf.Kopf.Adr=Adr.byte;
    Next=0; Mode=&TMSNetz::bstart;
   };
}

void TMSNetz::slvb()
{  TMSStat Stat;
   static unsigned short Ext;
   int Lng=0;
   if (OnGetState) Stat=(OnGetState(this,Lng) & ~(NET_RQST | NET_LINE)); else Stat=0;
   Stat|=(State.State[Self] & NET_RQST); // | NET_LINE;
	 Ext=Stat>>16;
	 TrmBuf.Kopf.Ext=Ext;
   makeblock(&TrmBuf,getAdr(),getAbs(GlobalNr),Lng,Stat>>8,Stat&0xFF,&Ext);
   storeSelfStatus();
   Next=0; Mode=&TMSNetz::bstart;
}


void TMSNetz::slvst() {

  	finddatenblock();

	if (datatosend()) {
	  	if (Param->RepeatData) {
		  	Next = &TMSNetz::slvst;
		} else {
		  	Next=&TMSNetz::slvb;
		}
		Mode=&TMSNetz::dstart;

	} else slvb();
}



void TMSNetz::slvtrm(PBlock channel) {

  	TrmPnt = channel;
  	trmstart();
  	Mode = &TMSNetz::waitdaten;
}



void TMSNetz::msttrm()
{  TMSStat Stat;
   int Lng;
   masterblkflg=true;
   if (OnGetState) Stat=(OnGetState(this,Lng) & ~(NET_RQST | NET_LINE)); else Stat=0;
   Stat|=(State.State[Self]&NET_RQST);
	 TrmBuf.Kopf.Ext=Stat>>16;
	 State.MasterDaten.All[0]=Stat>>16;
	 State.MasterDaten.All[1]=Stat>>24;
   makeblock(&TrmBuf,getAdr(),getAbs(DatPnt),2,Stat>>8,Stat&0xFF,&State.MasterDaten);
   storeSelfStatus();
   Next=0; Mode=&TMSNetz::bstart;
}



void TMSNetz::MTdelay() {

  	if (TimeOutBlk >= Param->BLKPAUSE) {
//   TimeOutBlk=0;
   	mstst();
  	};
}



void TMSNetz::mstst() {

  	unsigned int n;
//   IO0SET=bitTEST;
   	Timeout[0] = GetRcvTimeOut();
   	Teiln = 2;

	if (State.OR & NET_RQST) {
	  	if ((DatPnt > RS485_NET_MAXANZAHL) || (DatPnt == 0)) {
			DatPnt = 1;
		}
    	n = DatPnt;

		do {

		  if (n >= MT_ADR) {
				n = 1;
			} else {
			  	++n;
				if (n > Anzahl) {
				  n = MT_ADR;
				}
			};

			if ((State.State[n] & (NET_RQST | NET_LINE))==(NET_RQST | NET_LINE)) {
			  	DatPnt = n;
				break;
			};

    	} while (n != DatPnt);

	};

	finddatenblock();

	if (datatosend()) {
	  	if (Param->RepeatData) {
		  	Next = &TMSNetz::mstst;
		} else {
			Next = &TMSNetz::msttrm;
		}
		Mode = &TMSNetz::dstart;

   	} else {
	  	msttrm();
	}

//   IO0CLR=bitTEST;
}



void TMSNetz::ping(unsigned char PingAdr) {

  	makeblock(&TrmBuf, getAdr(), PingAdr, 0, 0, 0, 0);
   	TrmBuf.Kopf.Adr &= 0xB0;
   	Next = 0;
	Mode = &TMSNetz::bstart;
}



void TMSNetz::MTping() {

  	Teiln = MT_ADR;
   	Timeout[0] = GetTrmTimeOut();
   	ping(MT_ADR);
}



void TMSNetz::repeat(unsigned char PingAdr) {

  	Teiln = PingAdr;
   	Timeout[0] = GetRcvTimeOut();
   	makeblock(&TrmBuf, getAdr(), PingAdr, 0, 0, 0, 0);
   	TrmBuf.Kopf.Adr &= 0xF0;
   	Next = 0; // Mode=bstart;
   	TrmPnt = &TrmBuf;
   	TimeOutBlk = 0;

	if (masterblkflg) {
	    masterblkflg = false;
    	// Funktionszeiger gültig
		if (OnGetMasterDat) {
		  	OnGetMasterDat(this);
		}
   	};

	trmstart();
   	Mode = &TMSNetz::waittrm;
}

void TMSNetz::wait() {
	int i;
	bool ping;
	TMSNetAdr KopfAdr;
	TMSNetAbs KopfAbs;

	if (RcvBuf.State==BLK_OK) {		// Hier werden nur Netzblöcke ausgewertet
		tmp.Kopf=RcvBuf.Kopf; RcvBuf.State=BLK_WAIT;
		if (OnNetRcv) OnNetRcv(this,tmp);
		KopfAdr.byte=tmp.Kopf.Adr; _Rep=KopfAdr.bit.repeat; _Abs=KopfAdr.bit.abs;
		KopfAbs.byte=tmp.Kopf.Abs; _Glb=KopfAbs.byte;

		if (MTTest()) return;

    if ((_Abs==Self) || (TrmBuf.Kopf.Lng>BLK_MAXLENGTH)) return;		// Der eigene Status wird nicht empfangen

		TimeOutBlk=0;
    if (_Abs>1) {
			if (tmp.Kopf.Lng>0) {
				tmp.Kopf.Ext=(tmp.MasterDaten.All[1]<<8) | (tmp.MasterDaten.All[0] & 0xFF);
			} else tmp.Kopf.Ext=0;
      tmp.State[_Abs]=makestatus(tmp.Kopf);	     // Slavestatus

			if ((((tmp.State[_Abs])>>24) & 0xFF)>10)
				i=0;
			else
				i=1;

			tmp.AND&=tmp.State[_Abs];
      tmp.OR|=tmp.State[_Abs];
      Teiln=_Abs+1;
		  if ((tmp.AND&0xFFFF)==0x8000) tmp.AND=0;
    } else if (_Abs==1) {
	    Timeout[0]=GetRcvTimeOut();
      if (MasterBlkCnt<1000) ++MasterBlkCnt;
      if (tmp.Kopf.Lng>0) {			// normaler Netzblock
				tmp.Kopf.Ext=(tmp.MasterDaten.All[1]<<8) | (tmp.MasterDaten.All[0] & 0xFF);
				if (ExtFlag) {
					for (i=2;i<(sizeof(State.MasterDaten)-2);++i)
						tmp.MasterDaten.All[i-2]=tmp.MasterDaten.All[i];
				};
	    	if (OnMasterRcv) OnMasterRcv(this,tmp);
      	if (!Flag || Param->DataFlag) {
       		tmp.State[1] = makestatus(tmp.Kopf);
       		tmp.OR |= tmp.State[1];
       		tmp.AND &= tmp.State[1];

					if ((tmp.AND & 0xFFFF) == 0x8000) {
					  tmp.AND = 0;
					}

					VerbTyp = KopfAdr.bit.verb;
       		Anzahl = KopfAbs.bit.anz;
       		DatPnt = _Glb;

					if (finishstate()) {
//        	State.MasterDaten = tmp.MasterDaten;
       			Flag = true;
       		};
				};
			};
		};

		if (tmp.OR & st_Taste) {
		  	TasteCnt = 3;
		}


		// Ping empfangen
		if (_Abs == 0) {
			  ping=true;
		  	if (ismaster()) {
				return;
			}
     		_Abs = _Glb-1;					// Repeatblock
    	} else ping=false;



		if (ismaster()) {
		 	if (_Glb == GlobalNr) {
				GlobalQuit |= 1 << (_Abs-1);
			};

			if (_Abs == MT_ADR) {                                // Mode=MTdelay;
      	Flag |= finishstate();
				mstst();

     	} else if (_Abs>=Anzahl) {
			 	if (hasMT()) {
					MTping();
				} else {
				  Flag |= finishstate();
					mstst();
				};
     	};

		} else if (_Abs == Self-1) {
    	if (_Rep) {
			  if (Self!=MT_ADR) slvst();
			} else {
				if ((Param->SubAdr==0) || ((tmp.Kopf.Typ & 7)==Param->SubAdr)) {
			  	finddatenblock();
					pingantw();
				};
			};
    };

	} else if (sofort) {
   	TimeOutBlk = 0;
   	TrmPnt = sofort;
		sofort = 0;
		TrmPnt->State = BLK_READY;
    trmstart();
    Mode = &TMSNetz::waitdaten;

	} else if (TimeOutChar >= Timeout[0]) {         // Timeout behandeln
    TimeOutChar = 0;
		// resblkpause();

		if (ismaster()) {
    	_Abs = Teiln;
     	if (_Abs == MT_ADR) {
      	Flag |= finishstate();
				mstst();

     	} else if (_Abs >= Anzahl) {
			  if (hasMT()) {
				 	MTping();
				}  else {
				 	Flag |= finishstate();
					mstst();
				};

			} else {
			  repeat(_Abs+1);
			};

		} else {
     	if (TimeOutBlk >= GetSlvTimeOut()) {
			  TimeOutBlk = 0;
			  Flag |= finishstate();
			};
    };

 	};
}




//! Testet ob ein weiters MT angesteckt ist
bool TMSNetzMTTest::MTTest() {
 bool result=false;

  	if ((Self == MT_ADR) && (Param->SubAdr==0)) {

			if ((_Abs == MT_ADR) && ((MTcntrl!=0) || MTconflict)) {
				                              // MT Antwort
				MTcnt = 0;
				MTconflict = true;

			} else if ((_Abs == 0) && (_Glb == MT_ADR) && ((MTcntrl==2) || MTconflict)) {
				if (MTcntrl==2) MTcntrl=1;    // MT Ping
				result=true;

			} else if (_Abs == 1) {
				if (MTcntrl==1) MTcntrl=0;
				if (MTcnt>0x28) MTcnt=0; else ++MTcnt;
				if (MTcnt == 0x14) MTcntrl = 2;

				if (MTcnt == 0) {
			  	MTcntrl = 2;
					MTconflict = false;
				};
			};
		};

	return result;
}

void TMSNetzAyantra::dopingantw() {
   TMSStat Stat;
   TMSNetAdr Adr;
   static unsigned short Ext;
	 int Lng=0;
   if ((Self==MT_ADR) && (MasterBlkCnt<10)) { Mode=&TMSNetzAyantra::wait; return; };
   if (Daten) {
    Next=&TMSNetzAyantra::pingantw; Mode=&TMSNetzAyantra::dstart;
   } else {
    if (OnGetState) Stat=OnGetState(this, Lng); else Stat=0;
    Stat|=(State.State[Self] & (NET_RQST | NET_LINE));
 	  Ext=Stat>>16;
		TrmBuf.Kopf.Ext=Ext;
		if (Self==MT_ADR) Stat=Stat & ~((TMSStat)7) | Param->SubAdr;
		makeblock(&TrmBuf,getAdr(),getAbs(GlobalNr),Lng,Stat>>8,Stat&0xFF,&Ext);
    storeSelfStatus();
    Adr.byte=TrmBuf.Kopf.Adr; Adr.bit.repeat=0; TrmBuf.Kopf.Adr=Adr.byte;
    Next=0; Mode=&TMSNetzAyantra::bstart;
   };
}
