/** \file states.cpp

*/

#include <states.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NetzTyp_t NetzTyp;

/******************************************************************************/
unsigned char const AYANTRA=0x50;
unsigned char const ANTWORTADR=0x55;
unsigned int const STARTZEIT=100;       // Solange muss system_running()==true sein damit gestartet=true wird.
                                       // Angabe in 0.1s
unsigned int const ERRDELAY=300;       // Verzögerung der Fehlermeldung, wenn das Terminal angesteckt ist
                                       // Angabe in 0.1s
const int QRYREPEATTIME =  50;  //!< Nach dieser Zeit wird die Abfrage wiederholt, wenn keine Antwort kommt
const int QRYINTERVALL  = 300;  //!< In diesem Abstand erfolgt die Abfrage der Informationen
const int TRMTIMEOUT = 50;      //!< Timeout für das Senden eines Blockes
const int STATINTERVALL = 3000; //!< In diesem Abstand werden Statusmeldungen ausgegeben 5min
const int QUITTIMEOUT = 100;    //!< Nach dieser Zeit ohne Quittung wird der Block erneut gesendet
const int PAUSE = 10;
const int MAXVERSUCHE = 2;      //!< Anzahl der Versuche Informationen von der Multisignal zu lesen
const int SPANNUNGDOWN = 106;    //!< Ausgabe Spannungsfehler unterste Grenze
const int SPANNUNGLOW = 116;    //!< Ausgabe Spannungsfehler
const int SPANNUNGOK  = 123;    //!< Rücknahme Spannungsfehler
const unsigned int UDELAY  = 144000;  //!< Delay für Rücknahme Spannungsfehler in 0,1s = 4h
const int TIMEOUTK = QRYINTERVALL * 2; //!< Timeout fuer die Abfrage eines K-Blockes
const int TIMEOUTR = QRYINTERVALL * 2; //!< Timeout fuer die Abfrage eines R-Blockes
const int SGNSPUREN=3;          //!< Anzahl der zusätzlichen Aufzeichnungsspuren, PhZt (2), PrgNr

TBlock caGlobal={ 20 };
unsigned char InfoBuff[50];
TBlock caInfo;
unsigned char InfoBuff1[50];
TBlock caInfo1;
unsigned char TxInfoBuff[50];
TBlock TxcaInfo;
unsigned char TxSofortBuff[50];
TBlock TxSofort;

/******************************************************************************/

TQuittung::TQuittung() { Cnt=0; };

/******************************************************************************/

bool TFrage::operator==(TBlock op2)
{  bool result;
   result=(Adr==op2.Kopf.Abs) && (Abs==op2.Kopf.Adr) && (Nr==op2.Kopf.Nr) && (Typ=op2.Kopf.Typ);
   if (Typ=='r') result=result && (MemAdr==((PKmdR)op2.Buff)->Adr);
   return result;
}


TFrage TFrage::operator=(TBlock op2)
{  Adr=op2.Kopf.Adr;
   Abs=op2.Kopf.Abs;
   Nr=op2.Kopf.Nr;
   Typ=op2.Kopf.Typ | 0x20;
   Lng=((PKmdR)op2.Buff)->Lng;
   MemAdr=((PKmdR)op2.Buff)->Adr;
   return *this;
}

/************************* TState *********************************************/

TState::TState(TMTSprotocol &P) {
 Prtk=&P; reset(); Vorteiler=0; initfifo(Protokoll,sizeof(Protokoll));
 Mode=&TState::wait;
 ErrMsg=0;
 ChgMsg=0;
 PrgMsg=0;
 Daten.Sekunden=0;
}

void TState::resetK(int n)
{  Daten.Kopf[n].Version=0xFFFF;       // Softwareversion
   Daten.Kopf[n].Spannung=0xFFFF;
//   Daten.Kopf[n].Signal=0xFF;          // aktuelles signal
//   Daten.Kopf[n].Inputs.All=0x8000;    // aktuelle Eingangspegel
//   Daten.Kopf[n].Outputs.All=0x8000;   // aktuelle Ausgangspegel
   Daten.Kopf[n].Error.all=0x80000000; // Fehlerbits

	 Daten.Kopf[n].Ulow=false;           // Ulow
	 Daten.Kopf[n].Frei=false;           // Freigabe
}

void TState::resetR(int n)
{  Daten.Kopf[n].Lamps=0xFF; }

void TState::reset()
{ int i;
  setTimeStamp((unsigned long *)&dt);
  Daten.waitlock();
   Vorteiler=0;
   for (i=0;i<6;++i) Daten.Time[i]=0xFF;
   Daten.Anzahl=0;
   Daten.Nr=0;
   Daten.VerbTyp=0xFF;
   Daten.Mode=0xFF;
	 Daten.Warnung=0;
   Daten.PrgNr=0x7F;
   Daten.PhZt=0xFFFF;
   for (i=0;i<MAXSGN;++i) {
    resetK(i); resetR(i);
    Daten.Kopf[i].Signal=0xFF;          // aktuelles signal
    Daten.Kopf[i].Inputs.All=0x8000;    // aktuelle Eingangspegel
    Daten.Kopf[i].Outputs.All=0x8000;   // aktuelle Ausgangspegel
    Daten.Kopf[i].Green=0xFFFF;         // letzte Grünzeit
    Daten.Kopf[i].Nr=0xFF;
    TmpGreen[i]=0;
   };
   Daten.TerminalNr=0xFF;
  Daten.unlock();
  Prg[0]=0; Prg[1]=0;
  StatusFlg=false;
  QryAdr=1; QryCnt=0; PrgCnt=0; QryVer=0;
  PrgCnt=0xFFFF; PrgAdr=0;
  RS485OK=false;
  Quit=false;
}

bool TState::prgavail()
{ unsigned short crc;
  int Lng;
  if ((Prg[0])>=0xA0) Lng=0x100; else Lng=(Prg[0]<<8)+Prg[1];
  if ((Lng<0x40) || (Lng>1000)) return false;
  crc=calc_memcrc(Prg,(unsigned char *)&Prg[Lng-3],0);
  crc=calc_crc(Prg[Lng-2],crc);
  crc=calc_crc(Prg[Lng-1],crc);
  return (crc==0);
}

bool TState::system_running()
{ return (((Daten.Mode==msmode_AUTO) || (Daten.Mode==msmode_MAN)) && (LSA->State.AND & NET_OK) );
}

void TState::trmstatus(TMTSprotocol &P)
{ trmstatus0(P,0x1122);
}

void TState::trmerror(TMTSprotocol &P)
{ trmstatus0(P,0x2122);
  Quittung.Typ=0x2122; Quittung.BlkCnt=BlkCnt-1; Quittung.Timeout=QUITTIMEOUT;
  if (Quittung.Cnt) { Quittung.waitlock(); --Quittung.Cnt; Quittung.unlock(); };
}

/*
*/

void TState::makeRblock(PBlock channel, unsigned char Adr, unsigned char Abs, void *Daten, unsigned char BLng,
			 unsigned short BAdr)
{
   *((unsigned char(*)[1])Daten)[0]=BLng;
   *((unsigned char(*)[1])Daten)[1]=BAdr>>8;
   *((unsigned char(*)[1])Daten)[2]=BAdr & 0xFF;
   makeblock(channel,Adr,Abs,3,1,'R',Daten);
}

void TState::trmprogram(TMTSprotocol &P, int Offset, int n)
{ char *Memo;
  int i,L;
  if (n>512) n=512;
  if (P.TrmRest()<(n+15)) return;
  Memo=(char *)malloc(0x1000);
  if (Memo==0) trmbusy(P,0x3222);
  else {
   L=convert(Memo, Offset, n);
	 n=strlen(Memo);
   P.wrblockanfang(0);
   P.wrshort(0x3222);
   P.wrshort(n+5);
   P.wrchar(BlkCnt);
   P.wrshort(L);           // Gesamtlänge des Programms
   P.wrshort(Offset);      // Offset
   for (i=0;i<n;++i) P.wrchar(Memo[i]);
   P.wrblockende();
   ++BlkCnt;
   free(Memo);
   if (Offset==0) PRGFLAG=false;
  };
}

void TState::trmlog(TMTSprotocol &P)
{ char *Memo;
  int n,i;
  Memo=(char *)malloc(900);  // reicht aus für 10 Zeilen
  if (Memo==0) trmbusy(P,0x3322);
  else {
   errlog(Memo);
   n=strlen(Memo);
   P.wrblockanfang(0);
   P.wrshort(0x3322);
   P.wrshort(n+5);
   P.wrchar(BlkCnt);
   P.wrchar(PrtkNr);  // Kopf von dem das Protokoll kommt
   P.wrchar(1);       // Anzahl Protokolleinträge
   P.wrshort(PrtkAdr-0x3000);            //Adresse von der der Protokolleintrag kommt
   for (i=0;i<n;++i) P.wrchar(Memo[i]);  // Protokolleintrag
   P.wrblockende();
   ++BlkCnt;
   free(Memo);
  };
// if (PrtkAdr>=0x3010) PrtkAdr-=0x10; else PrtkAdr=0x4FF0;
  Flags.Prtk.Flag=false;
}

void TState::trmbusy(TMTSprotocol &P, short Type)
{ P.wrblockanfang(0);
  P.wrshort(0xFFFF);
  P.wrshort(3);
  P.wrchar(BlkCnt);
  P.wrshort(Type);
  P.wrblockende();
  ++BlkCnt;
}


void TState::trmversion(TMTSprotocol &P)
{ int i;
  char Sx[20];
  P.wrblockanfang(0);
  P.wrshort(0x3122);
  P.wrshort(strlen(Version)+1);
  P.wrchar(BlkCnt);
  for (i=0;i<strlen(Version);++i) P.wrchar(Version[i]);
  sprintf(Sx,"_%04X",__checksum);
  for (i=0;i<strlen(Sx);++i) P.wrchar(Sx[i]);
  P.wrblockende();
  ++BlkCnt;
}

void TState::trmstatus0(TMTSprotocol &P, unsigned short Msg)
{ int i,n;
  TMSError e;
  PMultisignal D;
  int const headerlng = 17;  // Länge des ersten Blocks
  int const kopflng = 16;    // Länge der einzelnen Köpfe

  if (P.TrmRest()<(sizeof(TMultisignal)+15)) return;
  D=(PMultisignal)malloc(sizeof(TMultisignal)); if (D==0) return;
   Daten.waitlock();
   *D=Daten;
   Daten.unlock();
   P.wrblockanfang(0);
   P.wrshort(Msg);
   n=D->Anzahl;
   if (!((n>0) && (n<MAXSGN))) n=0;
   P.wrshort(headerlng+1+n*kopflng);
   P.wrchar(BlkCnt);
   P.wrchar(headerlng);
   P.wrchar(kopflng);
   for (i=0;i<6;++i) P.wrchar(D->Time[i]);
   P.wrchar(D->Anzahl);
   P.wrchar(D->Nr);
   P.wrchar(D->VerbTyp);
   P.wrchar(D->Mode);
   if (PRGFLAG) P.wrchar(D->PrgNr | 0x80); else P.wrchar(D->PrgNr & 0x7F);
   P.wrshort(D->PhZt);
   P.wrshort(D->Sekunden);
   for (i=0;i<n;++i) {
    P.wrchar(D->Kopf[i].Nr);
    P.wrshort(D->Kopf[i].Version);
    if (D->Kopf[i].Spannung!=0xFFFF) P.wrshort(D->Kopf[i].Spannung);
    else if ((i+1)==D->TerminalNr) P.wrshort(D->Terminal.Spannung);
    else P.wrshort(0xFFFF);
    P.wrchar(D->Kopf[i].Signal);
    P.wrshort(D->Kopf[i].Green);
    P.wrshort(D->Kopf[i].Inputs.All);
    P.wrshort(D->Kopf[i].Outputs.All);
    if (D->Kopf[i].Error.unavail==0) {
     e.all=D->Kopf[i].Error.all;
     if (Daten.Kopf[i].Ulow) e.Uerr=true;
    } else if ((i+1)==D->TerminalNr) {
      e.all=D->Terminal.Error.all;
      if (Daten.Kopf[i].Ulow) e.Uerr=true;
    } else { e.all=0; e.unavail=1; };
    P.wrint(e.all);
   };
   P.wrblockende();
  free(D);
  ++BlkCnt;
}

void TState::trmshortstatus(TMTSprotocol &P)    // 0x2212
{ int i,n;
  PMultisignal D;
  unsigned short U;

  if (P.TrmRest()<(sizeof(TMultisignal)+15)) return;
  D=(PMultisignal)malloc(sizeof(TMultisignal));
  if (D==0) return;
  Daten.waitlock();
  *D=Daten;
  Daten.unlock();
  P.wrblockanfang(0);
  P.wrshort(0x1222);
  n=D->Anzahl;
  if (!((n>0) && (n<MAXSGN))) n=0;
  P.wrshort(5);
  P.wrchar(BlkCnt);
  P.wrchar(D->Anzahl);
  P.wrchar(D->Mode);
  U=500;
  for (i=0;i<n;++i) {
   if (U==0xFFFF) break;
   if (D->Kopf[i].Spannung==0xFFFF) U=0xFFFF;
   else if (D->Kopf[i].Spannung<U) U=D->Kopf[i].Spannung;
  };
  P.wrshort(U);
  P.wrblockende();
  free(D);
  StatusFlg=false;
  ++BlkCnt;
}

void TState::trmsignal(TMTSprotocol &P, int n)
{ int l,Anz;
  Anz=Daten.Anzahl;
	if (NetzTyp==USAneu) {
		if (Anz>=MAXSGN) { trmbusy(P,0x3422); return; };
	} else {
		if (Anz>8) { trmbusy(P,0x3422); return; };
	}
	if (n>30) n=30; Anz+=SGNSPUREN;
  if (P.TrmRest()<(n*(Anz)+15)) { trmbusy(P,0x3422); return; };
  while (getfifo(Protokoll)<0x100) rdfifo(Protokoll);
  while (fiforest(Protokoll)<(3*(Anz))) {
   rdfifo(Protokoll); while (getfifo(Protokoll)<0x100) rdfifo(Protokoll);
  };
  l=fifocnt(Protokoll)/Anz;
  if (l>n) l=n;
  P.wrblockanfang(0);
  P.wrshort(0x3422);
  P.wrshort(Anz*l+2);
  P.wrchar(BlkCnt);
  P.wrchar(Anz);
  for (n=0;n<Anz*l;++n) P.wrchar(rdfifo(Protokoll)&0xFF);
  P.wrblockende();
  ++BlkCnt;
}

void TState::wait()
{
  if (Mode!=&TState::wait) {
   Mode=&TState::wait;
  } else {
   if (ErrMsg==1) {
    ErrMsg=2; QryCnt=TRMTIMEOUT+5; getInfo(); ChgMsg=0;
   } else if ((ErrMsg==0) && (ChgMsg==1)) {
    ChgMsg=2; QryCnt=TRMTIMEOUT+5; getInfo();
   } else if ((ErrMsg==0) && (ChgMsg==0) && (PrgMsg==1)) {
    QryCnt=TRMTIMEOUT+5; PrgMsg=2; getPrg();
   } else if ((ErrMsg==0) && (ChgMsg==0) && (Flags.Prtk.Nr)) {
    QryCnt=TRMTIMEOUT+5;
    PrtkNr=Flags.Prtk.Nr;
    if (Flags.Prtk.Line==0) getPrtkStart(); else getPrtk(); Flags.Prtk.Nr=0;
   } else if (QryCnt==0) {
    if (gestartet) { QryCnt=TRMTIMEOUT+5; getInfo(); };
   };
  };
}

void TState::getInfo()
{ int n;

  if (Mode!=&TState::getInfo) {
   Mode=&TState::getInfo;
   QryAdr=1; QryVer=0; Quit=false;
  };
  if (Quit) {
   Quit=false; QryVer=0; n=QryAdr; Daten.Kopf[QryAdr-1].Nr=n;
   if (QryAdr>=Daten.Anzahl) {
//    QryCnt=TRMTIMEOUT+5; getLamps();
    if (ErrMsg&2) ErrMsg|=4;
    if (ChgMsg&2) ChgMsg|=4;
    QryCnt=QRYINTERVALL; wait();
   } else {
    ++QryAdr; QryVer=0; QryCnt=TRMTIMEOUT+5; qryinfo(TxcaInfo);
   };
  } else if (QryVer>=MAXVERSUCHE) {
   Quit=false; QryVer=0;
   if (QryAdr>=Daten.Anzahl) {
//    QryCnt=TRMTIMEOUT+5; getLamps();
    if (ErrMsg&2) ErrMsg|=4;
    if (ChgMsg&2) ChgMsg|=4;
    QryCnt=QRYINTERVALL; wait();
   } else {
    ++QryAdr; QryVer=0; QryCnt=TRMTIMEOUT+5; qryinfo(TxcaInfo);
   };
  } else qryinfo(TxcaInfo);
}

void TState::getLamps()
{
  if (Mode!=&TState::getLamps) {
   Mode=&TState::getLamps;
   QryAdr=1; QryVer=0; Quit=false;
  };
  if (Quit || (QryVer>=MAXVERSUCHE)) {
   Quit=false; QryVer=0;
   if (QryAdr>=Daten.Anzahl) {
    if (ErrMsg&2) ErrMsg|=4;
    if (ChgMsg&2) ChgMsg|=4;
    if (!prgavail()) { QryCnt=TRMTIMEOUT+5; getPrg(); } else { QryCnt=QRYINTERVALL; wait(); };
   } else {
    ++QryAdr; QryVer=0; QryCnt=TRMTIMEOUT+5; qrylamps(TxcaInfo,&TxInfoBuff);
   };
  } else qrylamps(TxcaInfo,&TxInfoBuff);
}

void TState::getPrtkStart()
{ if (Mode!=&TState::getPrtkStart) {
   Mode=&TState::getPrtkStart; QryVer=0; Quit=false; PrtkAdr=0x2FFE; PrtkLng=2;
  };
  if (Quit) {
   QryCnt=TRMTIMEOUT+5; getPrtk();
  } else if (QryVer>=MAXVERSUCHE) {
    Flags.lock();
    Flags.BusyMsg=true; Flags.Prtk.Flag=false;
    Flags.unlock();
    wait();
  } else qryprtk(TxcaInfo,&TxInfoBuff);
}

void TState::getPrtk()
{ if (Mode!=&TState::getPrtk) {
   Mode=&TState::getPrtk; QryVer=0; Quit=false; PrtkLng=0x10;
  };
  if (Quit) {
   ++Flags.Prtk.Cnt;
   if (PrtkAdr>=0x3010) PrtkAdr-=0x10; else PrtkAdr=0x4FF0;
   if (Flags.Prtk.Cnt<Flags.Prtk.Amount) {
    Quit=false; QryCnt=TRMTIMEOUT+5; qryprtk(TxcaInfo,&TxInfoBuff);
   } else { Flags.PrtkMsg=true; wait(); };
  } else if (QryVer>=MAXVERSUCHE) {
    Flags.lock();
    Flags.BusyMsg=true; Flags.Prtk.Flag=false;
    Flags.unlock();
    wait();
  } else qryprtk(TxcaInfo,&TxInfoBuff);
}

void TState::getPrg()
{ int n;

  if (Mode!=&TState::getPrg) {
   Mode=&TState::getPrg;
   PrgCnt=0xFFFF; PrgAdr=0; Quit=false; QryVer=0;
  };
  if (Quit) {
   Quit=false;
   n=PrgAdr;
   if (PrgCnt==n) {
    if (n) {
     QryCnt=QRYINTERVALL;
     if (PrgMsg&2) {
      PrgMsg|=4; PRGFLAG=prgavail();
     } else if (prgavail()) {
      PRGFLAG=true;
      Flags.waitlock();
#ifndef NOAUTOMSG
      Flags.ChgMsg=true;
#endif
      StatCnt=STATINTERVALL; Flags.unlock();
     };
     wait();
    } else {
     PRGFLAG=false;
     Prg[0]=0; Prg[1]=0;
     if (PrgMsg&2) PrgMsg|=4;
     wait();
    };
   } else QryCnt=TRMTIMEOUT+5;
  } else if (QryVer>=(MAXVERSUCHE*2)) {
   PrgCnt=0xFFFF; QryCnt=QRYINTERVALL; wait();
  };
  qryprg(TxcaInfo);
}

/************************* TMSState *******************************************/

TMSState::TMSState(TMTSprotocol &P) : TState(P)
{ StartCnt=STARTZEIT;
  ErrDelay=0;
	Ulimit=SPANNUNGLOW;
  gestartet=false;
  RS485OK=false;
}

int TMSState::convert(char *P, int Offset, int n)
{ return MSprgscript((unsigned char *)&Prg,P,Offset,n);
}

void TMSState::errlog(char *P)
{ int i;
  int h;
  if (Flags.Prtk.Amount>10) h=10;
  else if (Flags.Prtk.Amount==0) h=1; else h=Flags.Prtk.Amount;
  for (i=0;i<h;++i) {
   MSUSAConvertLogEntry(Log[i],P);
   if (i<(h-1)) strcat(P,"\r\n");
   P+=strlen(P);
  };
}

/*! \brief Abfrage des Fehlerprotokolls

    Es wird ein R-Block zum Lesen von TState#PrtkLng Bytes von Adresse TState#PrtkAdr an Blk zum Senden uebergeben.
    \param[in] Blk Benutzter Blockpuffer der RS485
    \param[in] B   Puffer fuer die Sendedaten
*/
void TMSState::qryprtk(TBlock &Blk, void *B)
{ int Lng,Adr;
  if ((Blk.State!=BLK_READY) && (Blk.State!=BLK_TRM) && (QryCnt<=TRMTIMEOUT)) {
   QryCnt=QRYREPEATTIME+TRMTIMEOUT; ++QryVer; Quit=false;
   Lng=PrtkLng; Adr=PrtkAdr;
   makeRblock(&Blk,PrtkNr,AYANTRA,B,Lng,Adr); Frage=Blk;
  } else if (QryCnt==0) { ++QryVer; QryCnt=QRYREPEATTIME+TRMTIMEOUT; };
}

/*! \brief Abfrage der vorhandenen Lampen

    Es wird ein R-Block zum Registerlesen auf Adresse 0x69 an Blk zum Senden uebergeben
    \param[in] Blk Benutzter Blockpuffer der RS485
    \param[in] B   Puffer fuer die Sendedaten
*/
void TMSState::qrylamps(TBlock &Blk, void *B)
{ if ((Blk.State!=BLK_READY) && (Blk.State!=BLK_TRM) && (QryCnt<=TRMTIMEOUT)) {
   QryCnt=QRYREPEATTIME+TRMTIMEOUT; ++QryVer; Quit=false;
   makeRblock(&Blk,QryAdr,AYANTRA,B,1,0x69); Frage=Blk;
  } else if (QryCnt==0) { ++QryVer; QryCnt=QRYREPEATTIME+TRMTIMEOUT; };
}

void TMSState::qryinfo(TBlock &Blk)
{ if ((Blk.State!=BLK_READY) && (Blk.State!=BLK_TRM) && (QryCnt<=TRMTIMEOUT)) {
   QryCnt=QRYREPEATTIME+TRMTIMEOUT; ++QryVer; Quit=false;
   makeblock(&Blk,QryAdr,AYANTRA,0,2,'K',0); Frage=Blk;
  } else if (QryCnt==0) { ++QryVer; QryCnt=QRYREPEATTIME+TRMTIMEOUT; };
}

void TMSState::qryprg(TBlock &Blk)
{ int n;
  if ((Blk.State!=BLK_READY) && (Blk.State!=BLK_TRM) && (QryCnt<=TRMTIMEOUT)) {
   QryCnt=QRYREPEATTIME+TRMTIMEOUT; ++QryVer; Quit=false;
   if (PrgCnt==0xFFFF) {
    makeRblock(&Blk,1,AYANTRA,&TxInfoBuff,2,0x200);
    Frage=Blk;
   } else {
    n=PrgCnt;
    if ((n-PrgAdr)>0x10) n=0x10; else n=(n-PrgAdr);
    makeRblock(&Blk,1,AYANTRA,&TxInfoBuff,n,0x200+PrgAdr);
    Frage=Blk;
   };
  } else if (QryCnt==0) { ++QryVer; QryCnt=QRYREPEATTIME+TRMTIMEOUT; };
}

void TMSState::processRS232()
{ P0xAEE1 P1;
  P0xAEE2 P2;
  int n;
  switch (Prtk->scann()) {
   case 0: break;
   case 0xAEE1:
    P1=(P0xAEE1)malloc(sizeof(T0xAEE1));
    if (P1) {
     Prtk->read((char *)P1,sizeof(T0xAEE1));
     switch (swap(P1->Rqst)) {
      case 0x2212:
       if ((P1->Lng>2) && (P1->Flags & 1)) {
        if (Conflict()) {
         for (n=0;n<MAXSGN;++n) { resetK(n); resetR(n); };
         StatCnt=STATINTERVALL; trmshortstatus(*Prtk);
        } else {
         StatusFlg=true;
         Flags.waitlock(); Flags.Read=true; StatCnt=STATINTERVALL; Flags.unlock();
        };
       } else {
        trmshortstatus(*Prtk);
       };
      break;
      case 0x2211:
       if ((P1->Lng>2) && (P1->Flags & 1)) {
        if (Conflict()) {
         for (n=0;n<MAXSGN;++n) { resetK(n); resetR(n); };
         StatCnt=STATINTERVALL; trmstatus(*Prtk);
        } else {
         StatusFlg=false;
         Flags.waitlock(); Flags.Read=true; StatCnt=STATINTERVALL; Flags.unlock();
        };
       } else {
        StatCnt=STATINTERVALL; trmstatus(*Prtk);
       };
      break;
      case 0x2231: trmversion(*Prtk); break;
      case 0x2232:
       if (P1->Lng>=4) {
        if (Conflict()) trmbusy(*Prtk,0x3222);
        else if (P1->Offset==0xFFFF) {
         Prg[0]=0; Prg[1]=0;
         Flags.waitlock(); Flags.ReadPrg=true; StatCnt=STATINTERVALL; Flags.unlock();
        } else trmprogram(*Prtk,P1->Offset,P1->DataLng);
       };
      break;
      case 0x2233:
       if (Conflict()) trmbusy(*Prtk,0x3322);
       else {
        Flags.waitlock();
         if ((!Flags.Prtk.Flag) && (P1->Nr) && (P1->Nr<MAXSGN)) {
          Flags.Prtk.Flag=true; Flags.Prtk.Nr=P1->Nr; Flags.Prtk.Line=P1->Line;
          if (P1->Amount>10) Flags.Prtk.Amount=10; else Flags.Prtk.Amount=P1->Amount;
          if (P1->Amount==0) Flags.Prtk.Amount=1;
          Flags.Prtk.Cnt=0;
         };
        Flags.unlock();
       };
      break;
      case 0x2234: trmsignal(*Prtk,60); break;
     };
     free(P1);
    } else Prtk->delblock();
   break;
   case 0xAEE2:
    P2=(P0xAEE2)malloc(sizeof(T0xAEE2));
    if (P2) {
     Prtk->read((char *)P2,sizeof(T0xAEE2));
     if ((P2->Typ==Quittung.Typ) && (P2->BlkCnt==Quittung.BlkCnt)) {
      Quittung.waitlock(); Quittung.Typ=0; Quittung.Cnt=0; Quittung.unlock();
     };
     free(P2);
    } else Prtk->delblock();
   break;
   default: Prtk->delblock(); break;
  };

  switch (swap(Quittung.Typ)) {
   case 0x2221: if (Quittung.Timeout==0) {
                 Quittung.Timeout=QUITTIMEOUT;
               #ifdef NOAUTOMSG
                 if (Quittung.Cnt==0) Quittung.Typ=0; else ErrMsg|=4;
               #else
                 ErrMsg|=4;
               #endif
                }; break;
  };
  Flags.waitlock(); if (Flags.PrgMsg) {
   Flags.PrgMsg=false; StatCnt=STATINTERVALL; Flags.unlock();
   if (PRGFLAG) trmprogram(*Prtk,0,0); else trmprogram(*Prtk,0,30);
  } else Flags.unlock();
  Flags.waitlock(); if (Flags.BusyMsg) {
   Flags.BusyMsg=false; StatCnt=STATINTERVALL; Flags.unlock(); trmbusy(*Prtk,0);
  } else Flags.unlock();
  Flags.waitlock(); if ((Flags.StatMsg) && !Pause) {
    Flags.StatMsg=false; StatCnt=STATINTERVALL; Flags.unlock();
    if (StatusFlg) trmshortstatus(*Prtk); else trmstatus(*Prtk); Pause=PAUSE;
  } else Flags.unlock();
  Flags.waitlock(); if ((Flags.ErrMsg) && !Pause) {
   Flags.ErrMsg=false; StatCnt=STATINTERVALL; Flags.unlock(); trmerror(*Prtk); Pause=PAUSE;
  } else Flags.unlock();
  Flags.waitlock(); if ((Flags.ChgMsg) && !Pause) {
   Flags.ChgMsg=false; StatCnt=STATINTERVALL; Flags.unlock();
   if (StatusFlg) trmshortstatus(*Prtk); else trmstatus(*Prtk); Pause=PAUSE;
  } else Flags.unlock();
  Flags.waitlock(); if (Flags.PrtkMsg) {
   Flags.PrtkMsg=false; StatCnt=STATINTERVALL; Flags.unlock(); trmlog(*Prtk);
  } else Flags.unlock();
}

bool TMSState::Conflict()
{ return ((LSA->MTconflict) || ((LSA->State.OR & NET_MT)==0));
}

void TMSState::execute()
{ int Tx,n;
// int i;

  T+=dTimeSet((unsigned long *)&dt);
  dT=0;
  while (T>(100 msec)) {
   T-=(100 msec);
	 ++dT;
   if (MasterSync<10) {
    if ((++MasterSync)>=10) {
     if (Vorteiler>5) Vorteiler=9; else Vorteiler=0;
    };
   };
   ++Vorteiler; if (Vorteiler>=10) {
    Vorteiler=0;
    Daten.waitlock();
     ++Daten.Sekunden;
     if ((Daten.Anzahl<MAXSGN) && (Daten.Anzahl>0)) {
      while (fiforest(Protokoll)<30) {
       rdfifo(Protokoll); while ((getfifo(Protokoll)<0x100) && fifocnt(Protokoll)) rdfifo(Protokoll);
      };
      wrfifo(Protokoll,(Daten.PhZt & 0xFF) | 0x100);
      wrfifo(Protokoll,(Daten.PhZt >> 8) & 0xFF);
      wrfifo(Protokoll,Daten.PrgNr);
      for (n=0;n<Daten.Anzahl;++n) wrfifo(Protokoll,Daten.Kopf[n].Signal);
      for (n=0;n<Daten.Anzahl;++n) if (Daten.Kopf[n].Signal=='#') ++TmpGreen[n];
     };
    Daten.unlock();
   };
  };
  Tx=dT;
//  if (Frage.Timeout>Tx) Frage.Timeout-=Tx; else Frage.Timeout=0;
  if (ErrDelay>Tx) ErrDelay-=Tx; else ErrDelay=0;
 	if (Udelay>Tx) Udelay-=Tx; else Udelay=0;
  if (QryCnt>Tx) QryCnt-=Tx; else QryCnt=0;
  if (Quittung.Timeout>Tx) Quittung.Timeout-=Tx; else Quittung.Timeout=0;
  for (n=0;n<MAXSGN;++n) {
   if (TimeOutK[n]>Tx) TimeOutK[n]-=Tx; else { if (TimeOutK[n]) resetK(n); TimeOutK[n]=0; };
   if (TimeOutR[n]>Tx) TimeOutR[n]-=Tx; else { if (TimeOutR[n]) resetR(n); TimeOutR[n]=0; };
  };
  if (Daten.TerminalTout>Tx) Daten.TerminalTout-=Tx;
  else { if (Daten.TerminalTout) Daten.TerminalNr=0xFF; Daten.TerminalTout=0; };

   if (system_running()) {
    if (StartCnt>Tx) StartCnt-=Tx; else StartCnt=0;
   } else { if (StartCnt) StartCnt=STARTZEIT; };
   if ((Pause>Tx) && (Pause<=PAUSE)) Pause-=Tx; else Pause=0;
   if (StatCnt>Tx) StatCnt-=Tx; else StatCnt=0;

  Flags.waitlock();
   if (Flags.ReadPrg) { PrgMsg=(PrgMsg | 1) & 7; Flags.ReadPrg=false; };
#ifdef NOAUTOMSG
   if (Flags.Read) { ChgMsg|=1; Flags.Read=false; };  //Statusanfrage mit "Neu lesen=1"
   if (StatCnt==0) { StatCnt=STATINTERVALL; };
#else
   if (Flags.Read) { ChgMsg|=1; Flags.Read=false; };
 #ifdef NOCHGMSG
   if (StatCnt==0) { StatCnt=STATINTERVALL; };
 #else
   if (StatCnt==0) { Flags.StatMsg=true; StatCnt=STATINTERVALL; };
 #endif
#endif
   if (ErrMsg & 4) {
    if (!Conflict() || (ErrDelay==0)) {
     Flags.ErrMsg=true; ErrMsg&=1; StatCnt=STATINTERVALL;
    };
   };
   if (ChgMsg & 4) { Flags.ChgMsg=true; ChgMsg&=1; StatCnt=STATINTERVALL; };
   if (PrgMsg & 4) { Flags.PrgMsg=true; PrgMsg&=1; };
   if (RS485OK) {
    if (LSA->isNetOn()==0) {
     reset(); Flags.ErrMsg=true;
#ifdef NOAUTOMSG
     /*ErrMsg=4;*/ Quittung.waitlock(); Quittung.Cnt=3; Quittung.unlock();
#endif
     StatCnt=STATINTERVALL;
    };
   } else RS485OK=(LSA->isNetOn() && (Daten.Mode!=msmode_error) && (Daten.Mode!=0xFF));
  Flags.unlock();

  if (Conflict()) {
#ifdef NOAUTOMSG
   if (ErrMsg&3) { ErrMsg=4; Quittung.waitlock(); Quittung.Cnt=3; Quittung.unlock(); }
#else
   if (ErrMsg&3) ErrMsg=4;
#endif
   if (ChgMsg&3) ChgMsg=4;
   Flags.Prtk.Flag=false;
   wait();
  } else if (Mode) (this->*Mode)();
//  testbit(gestartet);
}

/*! Wird aufgerufen, wenn ein K-Block von der Steuerung Nr empfangen wurde. \n
    Ist die Spannung < SPANNUNGLOW wird das Flag Ulow im Kopfstatus gesetzt und eine Fehlermeldung angewiesen \n
    Ist die Spannung > SPANNUNGOK wird das Flag Ulow zurueckgesetzt \n
    \param[in] Nr Kopfnummer
*/
void TMSState::SetUlow(int Nr)
{ volatile int Ux;
	if (gestartet) {
		Ux=Daten.Kopf[Nr].Spannung;
		if (Ux<SPANNUNGLOW) {
		  Daten.Kopf[Nr].Ulow=true;
		};
		if (Ux<Ulimit) {
			if (Ulimit>=SPANNUNGDOWN) {
#ifdef NOAUTOMSG
		    Quittung.waitlock(); Quittung.Cnt=3; Quittung.unlock();
#endif
    		ErrMsg|=1;
    		Ulimit=Ulimit-2;
			};
   };
	 if (Ux>SPANNUNGOK) {
		 Daten.Kopf[Nr].Ulow=false;
		 if ((Udelay==0) && (Ulimit!=SPANNUNGLOW)) {
			 Ulimit=SPANNUNGLOW;
			 ChgMsg|=1;
		 };
	 } else {
		 Udelay=UDELAY;
	 };
	} else {
		Ulimit=SPANNUNGLOW;
		Daten.Kopf[Nr].Ulow=false;
		Udelay=0;
	};
}

void TMSState::processblk_k2(TBlock &Blk)
{ PKmdK2 P;
  TMSError Err;
  int Nx;
  int a,b,x,d;
	// K-Kommando 2
  if ((Blk.Kopf.Adr==AYANTRA)
			 && (Blk.Kopf.Abs==QryAdr)
			 && (Blk.Kopf.Typ=='k')
			 && (Blk.Kopf.Nr==2))
	{
   P=(PKmdK2)Blk.Buff;
	 Nx=QryAdr-1;
   Daten.Kopf[Nx].Spannung=P->power; SetUlow(Nx);
   x=P->versionLO; b=0; d=1;
   do { a=x % 16; x=x/16; b+=a*d; d*=10; } while (x);  // Hex-Zahl als Dezimalzahl darstellen
   Daten.Kopf[Nx].Version=P->versionHI*10000 + b*100 + P->subversion;

   if (ErrMsg==0) Err.all=0; else Err=Daten.Kopf[Nx].Error;
   Err.all&=0x7FFFFFFF;  // unavail loeschen
   Err.all|=swap(P->fehler);
   Daten.Kopf[Nx].Error=Err;

   TimeOutK[Nx]=TIMEOUTK;
   if (Frage==Blk) { Quit=true; QryVer=0; };
  };
}


/*
( VARIABLE  Fehler    ( s. LCNetz beinhaltet die Integrierten Fehler )
          8000 EQU ^Sample    (  ES O )
          4000 EQU ^CErr      ( CESI  )
          2000 EQU ^NoPrg     ( CESI  )
          1000 EQU ^SgnErr    ( CESI  )  ( Synchronisationsfehler )
           800 EQU ^NotOK     ( CE  O )  ( OK liegt nicht von allen an )
           400 EQU ^NetzErr   (  ES O )  ( Timeout #NetzErr überschritten )
           200 EQU ^Feind     ( CESI  )  ( Feindliches Signalbild )
           100 EQU ^Qmode     (       )
            80 EQU ^UhrErr    ( CESI  )  ( Uhrenfehler )
            40 EQU ^O.Rot     ( CESI  )
            20 EQU ^O.Gelb    ( CESI  )
            10 EQU ^O.Gruen   ( CESI  )
             8 EQU ^UErr      ( CESI  )  ( kurze Spannungsunterschreitung )
             4 EQU ^L.Rot     ( CESI  )
             2 EQU ^L.Gelb    (       )
             1 EQU ^L.Gruen   (       )
*/

/*! Wird aufgerufen sobald ein Datenblock ueber die RS485 empfangen wurde und wertet ihn aus.
    'R' und 'K' werden ignoriert
    Die Inhalte der Bloecke werden in die Strucktur "Daten", die Informationen ueber das Gesamtsystem und die
    einzelnen Koepfe enthaelt, eingefuegt.
    Waehrend des Einfuegens der Informationen ist der Zugriff auf "Daten" von anderen Tasks aus gesperrt.

    Ist der empfangene Block eine Antwort auf eine Anforderung, wird Quit=true gesetzt.
*/
void TMSState::processblk(TBlock &Blk)
{ PKmdR R;
  PAntwort PA;
  int Nx;
  if ((Blk.Kopf.Typ=='R') || (Blk.Kopf.Typ=='K')) return;
  Daten.waitlock();
  // Zeit von der Anlage
  if ((Blk.Kopf.Adr==0x71) && (Blk.Kopf.Abs==1) && (Blk.Kopf.Typ=='T')) {
   Daten.Time[5]=*(Blk.Buff)[0];
   Daten.Time[4]=*(Blk.Buff)[1];
   Daten.Time[3]=*Blk.Buff[2];
   Daten.Time[2]=*Blk.Buff[3];
   Daten.Time[1]=*Blk.Buff[4];
   Daten.Time[0]=*Blk.Buff[5];
  };

/*
	// K-Kommando 2
  if ((Blk.Kopf.Adr==AYANTRA)
			 && (Blk.Kopf.Abs==QryAdr)
			 && (Blk.Kopf.Typ=='k')
			 && (Blk.Kopf.Nr==2))
	{
   P=(PKmdK2)Blk.Buff;
	 Nx=QryAdr-1;
   Daten.Kopf[Nx].Spannung=P->power; SetUlow(Nx);
   x=P->versionLO; b=0; d=1;
   do { a=x % 16; x=x/16; b+=a*d; d*=10; } while (x);  // Hex-Zahl als Dezimalzahl darstellen
   Daten.Kopf[Nx].Version=P->versionHI*10000 + b*100 + P->subversion;

   if (ErrMsg==0) Err.all=0; else Err=Daten.Kopf[Nx].Error;
   Err.all&=0x7FFFFFFF;  // unavail loeschen
   Err.all|=swap(P->fehler);
   Daten.Kopf[Nx].Error=Err;

   TimeOutK[Nx]=TIMEOUTK;
   if (Frage==Blk) { Quit=true; QryVer=0; };
  };
*/
	processblk_k2(Blk);

  R=(PKmdR)Blk.Buff;
  // Lampen
  if ((Blk.Kopf.Adr==AYANTRA) && (Blk.Kopf.Abs==QryAdr) && (Blk.Kopf.Typ=='r')
           && (R->Lng==1) && (R->Adr==0x6900)) {
   Daten.Kopf[QryAdr-1].Lamps=R->mem[0];
   TimeOutR[QryAdr-1]=TIMEOUTR;
   if (Frage==Blk) { Quit=true; QryVer=0; };
  };

  // Log-Eintrag
  if ((Blk.Kopf.Adr==AYANTRA) && (Blk.Kopf.Abs==PrtkNr) && (Blk.Kopf.Typ=='r')
           && (R->Lng==PrtkLng) && (R->Adr==swap(PrtkAdr))) {
   if (Frage==Blk) {
    if (PrtkLng==2) PrtkAdr=swap(R->PrgLng)+0x3000;
    else if ((Flags.Prtk.Cnt>=0) && (Flags.Prtk.Cnt<10)) memcpy(&Log[Flags.Prtk.Cnt],&(R->mem),0x10);
    Quit=true; QryVer=0;
   };
  };

  // Programmzeile
  if ((Blk.Kopf.Adr==AYANTRA) && (Blk.Kopf.Abs==1) && (Blk.Kopf.Typ=='r')
      && (R->Lng==2) && (R->Adr==0x0002) && (PrgCnt==0xFFFF)) {
   PrgCnt=swap(R->PrgLng); if (PrgCnt>0xA000) PrgCnt=0x100; PrgAdr=0;
   Quit=true; QryVer=0;
  } else if ((Blk.Kopf.Adr==AYANTRA) && (Blk.Kopf.Abs==1) && (Blk.Kopf.Typ=='r')
      && (swap(R->Adr)==PrgAdr+0x200) && ((R->Lng+PrgAdr) < sizeof(Prg)) && (PrgCnt!=0xFFFF)) {
   memcpy(&Prg[PrgAdr],&(R->mem),R->Lng);
   PrgAdr+=R->Lng;
   if (Frage==Blk) { Quit=true; QryVer=0; };
  };

  // Blöcke an das Mikroterminal
  if ((Blk.Kopf.Adr==ANTWORTADR) && ((Blk.Kopf.Typ=='?') || (Blk.Kopf.Typ=='H') || (Blk.Kopf.Typ=='D'))
      && (Blk.Kopf.Lng>=4) ) {
   PA=(PAntwort)Blk.Buff; Nx=Blk.Kopf.Abs-1;
   if ((Nx>=0) && (Nx<Daten.Anzahl)) {
    Daten.TerminalNr=Nx+1;

    Daten.Kopf[Nx].Version=0xFFFF;       // Softwareversion zurücksetzen

    Daten.Terminal.Spannung=PA->power; SetUlow(Nx);
    Daten.Terminal.Error.all=swap(PA->fehler);

/*    if (ErrMsg==0) Err.all=0; else Err=Daten.Kopf[Nx].Error;
    Err.all|=swap(PA->fehler);
    Daten.Kopf[Nx].Error=Err;
*/
    Daten.TerminalTout=TIMEOUTK;
/*    Nx=PA->lng; if (Nx>=sizeof(Daten.PrgName)) Nx=sizeof(Daten.PrgName)-1;
    memcpy(&Daten.PrgName,&(PA->name),Nx); Daten.PrgName[Nx]=0;
*/
   };
  };

  // Kennzeichnung, das neues Programm geladen wird
  if ((Blk.Kopf.Typ=='l') || (Blk.Kopf.Typ=='L')) {
   if (Mode==&TMSState::getPrg) wait();
   Prg[0]=0; Prg[1]=0; PrgCnt=0xFFFF; QryCnt=QRYINTERVALL;
  };

  Daten.unlock();

  // Anlagenstart erlauben
//  if (Blk.Kopf.Typ=='C') { if (StartCnt) --StartCnt; startsystem(); };

}


/* Wird aufgerufen sobald das Vorliegen eines Netzblockes gemeldet wird (Flag von der RS485). \n
   Wenn sich der Mode gegenueber dem letzten Aufruf geaendert hat wird eine Statusmeldung angefordert (ChgMsg |= 1). \n
   War der letzte Mode ein Mode ohne Fehler und ist der aktuelle Mode == error wird eine Fehlermeldung
   angefordert (ErrMsg |= 1). Ausserdem wird bei einer Fehlermeldung der Wiederholungszaehler Quittung.Cnt auf 3 gesetzt
   und auf eine Quittung gewartet.
*/
void TMSState::processblk(TMSNetz & MS)
{ int n, tmp;
  TMS_STATE st;
  bool frei;
  if ((MS.State.State[1] & NET_LINE)==0) return;
  if (ErrMsg || ChgMsg || Quittung.Cnt) return;
  Daten.waitlock();
   Daten.Anzahl=MS.Anzahl;
   Daten.VerbTyp=MS.VerbTyp;
   if (Daten.PrgNr!=MS.State.MasterDaten.PrgNr) {
    Daten.PrgNr=MS.State.MasterDaten.PrgNr;
#ifndef NOAUTOMSG
    ChgMsg|=1;
#endif
   };
   if (MasterSync>=10) MasterSync=(MS.State.MasterDaten.Cnt*15625)/100000;

   n=MS.State.MasterDaten.quelng;
   Daten.PhZt=MS.State.MasterDaten.All[n+2];
   tmp=MS.State.MasterDaten.mode;
   st.All=MS.State.AND;
   if (st.OK==0) tmp=msmode_error;
   else {
    if (tmp>=0x80) {
     switch (tmp) {
      case msmode_fehler: tmp=msmode_error; break;
      case msmode_off:    tmp=msmode_OFF; break;
      case msmode_rot:    tmp=msmode_ROT; break;
      case msmode_blink:  tmp=msmode_BLINK; break;
      case msmode_blink2rot: tmp=Daten.Mode; break;

      case msmode_rtbl:   tmp=msmode_ROTBL; break;
      case msmode_gelb: case msmode_gelb2rot:
                          tmp=Daten.Mode; break; //  msmode_GELB; break;
      case msmode_qrz:    tmp=msmode_QRZ; break;
      case msmode_yebl:   tmp=msmode_GELBBL; break;
      default: tmp=0xFF; break;
     };
    } else {
     if (StartCnt==0) if (!gestartet) { gestartet=true; ChgMsg|=1; };
     if (MS.State.MasterDaten.man) tmp=msmode_MAN;
     else tmp=msmode_AUTO;
    };
   };

   if ((Daten.Mode!=tmp) && gestartet) {
    if (((tmp==msmode_error) || (tmp==0xFF)) && (Daten.Mode!=0xFF)) {
     Daten.Mode=tmp; ErrDelay=ERRDELAY;
     for (n=0;n<MAXSGN;++n) { resetR(n); resetK(n); };

#ifdef NOAUTOMSG
     Quittung.waitlock(); Quittung.Cnt=3; Quittung.unlock();
#endif
     ErrMsg|=1;
#ifdef NOCHGMSG
    } else Daten.Mode=tmp;
#else
    } else { Daten.Mode=tmp; ChgMsg|=1; };
#endif
   }
	 Daten.Mode=tmp;

   if ((Daten.Anzahl<MAXSGN) && (Daten.Anzahl>0)) {
    for (n=0;n<Daten.Anzahl;++n) {
     st.All=MS.State.State[n+1];
     if (st.line==0) {
      Daten.Kopf[n].Nr=0xFF;
      Daten.Kopf[n].Signal=0xFF;
      Daten.Kopf[n].Inputs.All=0x8000;
      Daten.Kopf[n].Outputs.All=0x8000;
     } else {
      Daten.Kopf[n].Nr=n+1;
      if ((n==1) && ((st.bitnew) || (st.Prg==0))) {
       Prg[0]=0; Prg[1]=0; PrgCnt=0xFFFF; //QryCnt=QRYINTERVALL;
      };
      if (Conflict()) Daten.Nr=0; else if (st.MT) Daten.Nr=n+1;
      frei=(st.frei) || (st.signal&1);
      if ( !frei && Daten.Kopf[n].Frei ) Daten.Kopf[n].Green=TmpGreen[n];
      Daten.Kopf[n].Frei=frei;
      if (!frei) TmpGreen[n]=0;
      switch (Daten.Mode) {
       case msmode_AUTO: case msmode_MAN: case msmode_QRZ:
        switch (st.signal) {
         case 0: Daten.Kopf[n].Signal='.'; break;
         case 1: Daten.Kopf[n].Signal='#'; break;
         case 3: case 5: case 7:
                 Daten.Kopf[n].Signal='*'; break;
         case 6: Daten.Kopf[n].Signal='X'; break;
         case 2: Daten.Kopf[n].Signal='/'; break;
         case 4: Daten.Kopf[n].Signal='-'; break;
         default: Daten.Kopf[n].Signal='?'; break;
        };
       break;
       case msmode_OFF:    Daten.Kopf[n].Signal='.'; break;
       case msmode_ROT:    Daten.Kopf[n].Signal='-'; break;
       case msmode_ROTBL:  Daten.Kopf[n].Signal='='; break;
       case msmode_GELB:   Daten.Kopf[n].Signal='/'; break;
       case msmode_GELBBL: Daten.Kopf[n].Signal=':'; break;
       default: Daten.Kopf[n].Signal='?'; break;
      };
      Daten.Kopf[n].Inputs.All=0;
      Daten.Kopf[n].Inputs.MT=st.MT;
      Daten.Kopf[n].Inputs.sensor=st.sensor;
      Daten.Kopf[n].Inputs.taster=st.taster;
      Daten.Kopf[n].Inputs.sensor2=st.sensor2;
      Daten.Kopf[n].Outputs.All=0;
      Daten.Kopf[n].Outputs.rot=((st.signal & 4)!=0);
      Daten.Kopf[n].Outputs.gelb=((st.signal & 2)!=0);
      Daten.Kopf[n].Outputs.gruen=((st.signal & 1)!=0);
     };
    };
   };

  Daten.unlock();
}

/************************* TMSStateNeu ****************************************/

TMSStateNeu::TMSStateNeu(TMTSprotocol &P) : TMSState(P)
{
}

void TMSStateNeu::processblk_k2(TBlock &Blk)
{ PKmdK2new P;
  TMSError Err;
  int Nx;
  int a,b,x,d;
	// K-Kommando 2
  if ((Blk.Kopf.Adr==AYANTRA)
			 && (Blk.Kopf.Abs==QryAdr)
			 && (Blk.Kopf.Typ=='k')
			 && (Blk.Kopf.Nr==2))
	{
   P=(PKmdK2new)Blk.Buff;
	 Nx=QryAdr-1;
   Daten.Kopf[Nx].Spannung=P->power; SetUlow(Nx);
   x=P->versionLO; b=0; d=1;
   do { a=x % 16; x=x/16; b+=a*d; d*=10; } while (x);  // Hex-Zahl als Dezimalzahl darstellen
   Daten.Kopf[Nx].Version=P->versionHI*10000 + b*100 + P->subversion;

   if (ErrMsg==0) Err.all=0; else Err=Daten.Kopf[Nx].Error;
   Err.all&=0x7FFFFFFF;  // unavail loeschen
   Err.all|=swap(ConvertError(P->fehler,P->Loff,P->Lon));
   Daten.Kopf[Nx].Error=Err;

   TimeOutK[Nx]=TIMEOUTK;
   if (Frage==Blk) { Quit=true; QryVer=0; };
  };
}

void TMSStateNeu::processblk(TMSNetz & MS)
{ int n, tmp;
  msstate_t st;
	msstate_t st_or;
  bool frei;
  if ((MS.State.State[1] & NET_LINE)==0) return;
  if (ErrMsg || ChgMsg || Quittung.Cnt) return;
  Daten.waitlock();
   Daten.Anzahl=MS.Anzahl;
   Daten.VerbTyp=MS.VerbTyp;
   if (Daten.PrgNr!=MS.State.MasterDaten.PrgNr) {
    Daten.PrgNr=MS.State.MasterDaten.PrgNr;
#ifndef NOAUTOMSG
    ChgMsg|=1;
#endif
   };
   if (MasterSync>=10) MasterSync=(MS.State.MasterDaten.Cnt*15625)/100000;

   Daten.PhZt=MS.State.MasterDaten.PhZtNeu;
   tmp=MS.State.MasterDaten.ModeNeu & 0xFF;
   st.All=MS.State.AND;
	 st_or.All=MS.State.OR;
   if (st.OK==0) tmp=msmode_error;
   else {
    if (tmp>=0x80) {
     switch (tmp) {
      case msmode_fehler: tmp=msmode_error; break;
      case msmode_off:    tmp=msmode_OFF; break;
      case msmode_rot:    tmp=msmode_ROT; break;
      case msmode_blink:  tmp=msmode_BLINK; break;
      case msmode_blink2rot: tmp=Daten.Mode; break;

      case msmode_rtbl:   tmp=msmode_ROTBL; break;
      case msmode_gelb: case msmode_gelb2rot:
                          tmp=Daten.Mode; break; //  msmode_GELB; break;
      case msmode_qrz:    tmp=msmode_QRZ; break;
      case msmode_yebl:   tmp=msmode_GELBBL; break;
      default: tmp=0xFF; break;
     };
    } else {
     if (StartCnt==0) if (!gestartet) { gestartet=true; ChgMsg|=1; };
     if (MS.State.MasterDaten.man) tmp=msmode_MAN;
     else tmp=msmode_AUTO;
    };
   };

   if ((Daten.Mode!=tmp) && gestartet) {
    if (((tmp==msmode_error) || (tmp==0xFF)) && (Daten.Mode!=0xFF)) {
     Daten.Mode=tmp; ErrDelay=ERRDELAY;
     for (n=0;n<MAXSGN;++n) { resetR(n); resetK(n); };

#ifdef NOAUTOMSG
     Quittung.waitlock(); Quittung.Cnt=3; Quittung.unlock();
#endif
     ErrMsg|=1;
#ifdef NOCHGMSG
    } else Daten.Mode=tmp;
#else
    } else { Daten.Mode=tmp; ChgMsg|=1; };
#endif
   }
	 Daten.Mode=tmp;

	 if ((st_or.warnung!=0) && (Daten.Warnung==0) && gestartet) ChgMsg|=1;
	 if (st_or.warnung!=0) Daten.Warnung=1; else Daten.Warnung=0;

   if ((Daten.Anzahl<MAXSGN) && (Daten.Anzahl>0)) {
    for (n=0;n<Daten.Anzahl;++n) {
     st.All=MS.State.State[n+1];
     if (st.line==0) {
      Daten.Kopf[n].Nr=0xFF;
      Daten.Kopf[n].Signal=0xFF;
      Daten.Kopf[n].Inputs.All=0x8000;
      Daten.Kopf[n].Outputs.All=0x800;
     } else {
      Daten.Kopf[n].Nr=n+1;
      if ((n==1) && ((st.neu) || (st.Prg==0))) {
       Prg[0]=0; Prg[1]=0; PrgCnt=0xFFFF; //QryCnt=QRYINTERVALL;
      };
      if (Conflict()) Daten.Nr=0; else if (st.MT) Daten.Nr=n+1;
      frei=(st.frei) || (st.gruen);
      if ( !frei && Daten.Kopf[n].Frei ) Daten.Kopf[n].Green=TmpGreen[n];
      Daten.Kopf[n].Frei=frei;
      if (!frei) TmpGreen[n]=0;
      switch (Daten.Mode) {
       case msmode_AUTO: case msmode_MAN: case msmode_QRZ:
        switch (st.gruen | (st.gelb<<1) | (st.rot<<2)) {
         case 0: Daten.Kopf[n].Signal='.'; break;
         case 1: Daten.Kopf[n].Signal='#'; break;
         case 3: case 5: case 7:
                 Daten.Kopf[n].Signal='*'; break;
         case 6: Daten.Kopf[n].Signal='X'; break;
         case 2: Daten.Kopf[n].Signal='/'; break;
         case 4: Daten.Kopf[n].Signal='-'; break;
         default: Daten.Kopf[n].Signal='?'; break;
        };
       break;
       case msmode_OFF:    Daten.Kopf[n].Signal='.'; break;
       case msmode_ROT:    Daten.Kopf[n].Signal='-'; break;
       case msmode_ROTBL:  Daten.Kopf[n].Signal='='; break;
       case msmode_GELB:   Daten.Kopf[n].Signal='/'; break;
       case msmode_GELBBL: Daten.Kopf[n].Signal=':'; break;
       default: Daten.Kopf[n].Signal='?'; break;
      };
      Daten.Kopf[n].Inputs.All=0;
      Daten.Kopf[n].Inputs.MT=st.MT;
      Daten.Kopf[n].Inputs.sensor=st.sensor;
      Daten.Kopf[n].Inputs.taster=st.taster;
      Daten.Kopf[n].Inputs.sensor2=st.sensor2;
      Daten.Kopf[n].Outputs.All=0;
      Daten.Kopf[n].Outputs.rot=(st.rot!=0);
      Daten.Kopf[n].Outputs.gelb=(st.gelb!=0);
      Daten.Kopf[n].Outputs.gruen=(st.gruen!=0);
     };
    };
   };

  Daten.unlock();
}


