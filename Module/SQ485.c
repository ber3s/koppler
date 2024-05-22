// ***************************************************************************
//
// RS485 Netzschnittstelle
// 01.02.2006
//
//****************************************************************************

#include <tools.h>
#include <LPC_SysControl.h>
#include <timer.h>
#include <cstring>
#include <sq485.h>

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
*****************************************************************************/

void TSQNetz::Execute()
{ int i;
  static unsigned long temp;
  if (++TaktCnt < Takt) return;
  TaktCnt=0;
  temp=Tx; Tx=SystemZeit(); dt=Tx-temp; if (dt>0x7FFFFFFFUL) dt=(~dt)+1;
  CHWCcnt+=dt;
  if (CHWCcnt>=Param->CHWCtakt) { CHWCcnt-=Param->CHWCtakt; if (CHWC>=100) CHWC=0; else ++CHWC; };
  dTime(&RcvTimeout);  // muss gemacht werden da die Zeit im RxD Interrupt gebraucht wird
  if (TimeOutChar<0x80000000) TimeOutChar+=dt;
  if (TimeOutBlk<0x80000000) TimeOutBlk+=dt;
  for (i=1;i<=SQ_NET_MAXANZAHL;++i) if (Timeout[i]<0x80000000) Timeout[i]+=dt;
  if (Mode) (this->*Mode)();
}

bool TSQNetz::isNetOn()
{ return (TimeOutBlk<0x100000); }


TSQNetz::TSQNetz(PSQParam AParam,PChannels ARxCh,PChannels ATxCh, unsigned int ATakt,unsigned char Abs,unsigned char Verb, unsigned char Anz)
{  memset(this,0,sizeof(TSQNetz));
   Param=AParam;
   RxCh=ARxCh;
   TxCh=ATxCh;
   Takt=ATakt;
   Self=Abs;
   State.Daten.N.Anz=Anz;
   Timeout[0]=Param->SLV_TIMEOUT;
   TimeOutChar=0;
   TimeOutBlk=0;
   MasterBlkCnt=0;
   Mode=&TSQNetz::wait;
}

// holt das nächste Zeichen, das gesendet werden soll
int TSQNetz::getch() {
 char cx;
 PBlock pnt;
 if (AAtrm) { AAtrm=false; return 0xAA; };
 pnt=TrmPnt;
 if (pnt==0) return -1;
 if (pnt->State==BLK_READY) pnt->State=BLK_TRM;
 if (pnt->State!=BLK_TRM) return -2;
 switch (pnt->cnt) {
  case -5:  cx=pnt->Kopf.Adr; pnt->CRC=calc_crc(cx,0); pnt->cnt=-4; break;
  case -4:  cx=pnt->Kopf.Abs; pnt->CRC=calc_crc(cx,pnt->CRC); pnt->cnt=-3; break;
  case -3:  cx=pnt->Kopf.Lng; pnt->CRC=calc_crc(cx,pnt->CRC); pnt->cnt=-2; break;
  case -2:  cx=pnt->Kopf.Nr;  pnt->CRC=calc_crc(cx,pnt->CRC); pnt->cnt=-1; break;
  case -1:  cx=pnt->Kopf.Typ; pnt->CRC=calc_crc(cx,pnt->CRC); pnt->cnt=0;  break;
  default: if (pnt->cnt==pnt->Kopf.Lng) cx=(pnt->CRC>>8)&0xFF;
           else if (pnt->cnt==pnt->Kopf.Lng+1) cx=(pnt->CRC&0xFF);
           else if (pnt->cnt>pnt->Kopf.Lng+1) {
            if (pnt->Kopf.Adr==SQ_GLB_ADR) {
             GlobalNr=pnt->Kopf.Nr; GlobalQuit=0;
            };
            pnt->State=BLK_TRMOK;
            TimeOutBlk=0;
            TrmPnt=0;
            return BLK_TRMOK;
           } else {
            cx=(*pnt->Buff)[pnt->cnt];
            pnt->CRC=calc_crc(cx,pnt->CRC);
           };
           ++pnt->cnt; break;
 }; /* switch */
 TimeOutChar=0;
 if (cx==0xAA) AAtrm=true;
 return cx;
}

void TSQNetz::seterrnumber(char Nr, char Last)
{ ErrNumber=Nr; LastChar=Last;
}

bool odd(unsigned char cx)
{ int i;
  i=0;
  while (cx) { i+=cx&1; cx>>=1; };
  return (i&1);
}

int TSQNetz::wrstatus()
{ TSQNet_TeilnehmerStatus *Pnt;
  TSQStat val;
  unsigned char cx;

  if (!RxEmpty()) {
   val.all=RxD();
   if (odd(val.all)) val.unglt=0; else val.unglt=1;
  } else val.all=SQ_UNGLT;
  if (!OnSyncNetz) { Mode=&TSQNetz::wait; return 2; };
  if ((StatIdx>Temp.Daten.N.Anz+1) || (StatIdx>SQ_NET_MAXANZAHL+1)) { OnSyncNetz(this,3); Mode=&TSQNetz::wait; return 2; };
  if (StatIdx==Temp.Daten.N.Anz+1) OnSyncNetz(this,3); else OnSyncNetz(this,2);

  if (StatIdx==Self-1) {   // eigenen Status senden
   SelfState.line=0;
   if (OnGetState) OnGetState(this);
   if (!State.OR.unglt) SelfState.line=1;
   if (Daten) SelfState.rqst=1; else SelfState.rqst=0;
   SelfState.OK=1;
   SelfState.unglt=0;
   cx=SelfState.all;
   if (!odd(cx)) cx|=0x80;
   RS485on(); TxStart(cx);
  };

  Pnt=&Temp.State[StatIdx];
  if (Pnt->status.all==val.all)
   { if (Pnt->count < MAX_STAT_COUNT) ++Pnt->count; }
  else Pnt->count=0;

  if (StatIdx<0) ;
  else if (StatIdx==0) {
   if (!val.unglt) Temp.OR.rqst|=val.rqst;
   Pnt->status=val;
  } else {
   if (StatIdx==Temp.Daten.N.Anz+1) {
    val.unglt=!val.unglt;
   } else {
    if (val.OK) {
     if (Pnt->OK < 3) ++Pnt->OK;
    } else {
     if (Pnt->OK) --Pnt->OK;
    };
    if (val.unglt) {  /* 80 wird gesetzt, wenn 2x unglt */
     if (Pnt->Line<2) ++Pnt->Line; else Pnt->Line|=SQ_UNGLT;
     Pnt->status.unglt=1;
     Temp.OR.all|=SQ_UNGLT;
     Temp.AND.all&=SQ_UNGLT;
    } else {
     Pnt->Line=0; Timeout[StatIdx]=0;
     Pnt->status=val;
     Temp.OR.all|=val.all;
     Temp.AND.all&=val.all;
    };
   };
  };
  if (StatIdx==Temp.Daten.N.Anz+1) {
   State=Temp; Flag=true; Mode=&TSQNetz::wait; return 1;
  } else {
   ++StatIdx; return 0;
  };
}

// speichert ein empfangenes Zeichen
int TSQNetz::putch(unsigned short cx) {
 PBlock rcv;
 PSQDaten Dat;
 int n;
 if (cx==0xAA) {
   if (AArcv) AArcv=false; else { AArcv=true; return 0; };
 } else {
  if (AArcv) { AArcv=false; if (RcvPnt) RcvPnt->State=BLK_WAIT; RcvPnt=0; };
 };
 TimeOutChar=0;
 rcv=RcvPnt;
 if (rcv==0) {
  n=0; rcv=&caDummy; caDummy.LngX=SQ_BLK_MAXLENGTH; caDummy.Buff=(unsigned char(*)[1])&Temp.Daten;
  if (cx!=0xFF) {
   do {
    if (((*RxCh[n])->AdrX)==(cx & (*RxCh[n])->Msk)
         && ((*RxCh[n])->State==BLK_WAIT) ) { rcv=*RxCh[n]; break; };
   } while (*RxCh[++n]);
   if (rcv==&caDummy) seterrnumber(4,cx);
  };
  rcv->cnt=-5;
  RcvPnt=rcv;
 } else if (rcv->State!=BLK_RCV) {
  RcvPnt=0; seterrnumber(1,cx); return -4;
 }
 switch (rcv->cnt) {
   case -5: rcv->CRC=calc_crc(cx,0); rcv->Kopf.Adr=cx; rcv->cnt=-4; rcv->State=BLK_RCV; break;
   case -4: rcv->CRC=calc_crc(cx,rcv->CRC); rcv->Kopf.Abs=cx; rcv->cnt=-3;  break;
   case -3: rcv->CRC=calc_crc(cx,rcv->CRC);
            if (cx<=rcv->LngX) { rcv->Kopf.Lng=cx; rcv->cnt=-2; } else { rcv->State=BLK_WAIT; RcvPnt=0; seterrnumber(2,cx); return -3; };
            break;
   case -2: rcv->CRC=calc_crc(cx,rcv->CRC); rcv->Kopf.Nr=cx; rcv->cnt=-1; break;
   case -1: rcv->CRC=calc_crc(cx,rcv->CRC); rcv->Kopf.Typ=cx; rcv->cnt=0; break;
   default: if (rcv->cnt==rcv->Kopf.Lng) rcv->CRC-=cx<<8;
            else if (rcv->cnt>rcv->Kopf.Lng) {
             rcv->CRC-=cx;
             if (rcv->CRC!=0) { rcv->State=BLK_WAIT; RcvPnt=0; seterrnumber(3,cx); return -2; };
             TimeOutBlk=0;
             if (rcv->Kopf.Adr==0xFF) {
              switch (rcv->Kopf.Typ) {
               case 'N':
                Mode=&TSQNetz::rcvState;
                StatIdx=-1;
                if (OnSyncNetz) OnSyncNetz(this,1);
                Temp.Kopf=rcv->Kopf;
                Temp.AND.all=0xFF; Temp.OR.all=0;
                Dat=(PSQDaten)rcv->Buff;
                if (Dat->N.Anz<17) Temp.Daten.N.Anz=Dat->N.Anz; else Temp.Daten.N.Anz=16;
                if ((CHWC>85)||(CHWC<55)) CHWC=Dat->N.CHWC;
                if (MasterBlkCnt<1000) ++MasterBlkCnt;
               break;
               case 'U': break;
               case 'R': if (rcv->Kopf.Abs==Self) { RFlag=4; Mode=&TSQNetz::trmD; }; break;
              };
             };
             rcv->State=BLK_OK; RcvPnt=0;
             return BLK_OK;
            } else {
             rcv->CRC=calc_crc(cx,rcv->CRC);
             (*rcv->Buff)[rcv->cnt]=cx;
            };
            ++rcv->cnt; break;
 };
 return 0;
}

void TSQNetz::trmnext(void)
{ int temp;
  if (!TrmPnt) TxStop();
  else { temp=getch(); if (temp>=0) TxD(temp); else TxStop(); };
}

unsigned short TSQNetz::clrerr(void)
{ unsigned short temp;
  temp=geterr(); RxD();
  return temp;
}

void TSQNetz::rcvnext(void)
{ while (!RxEmpty()) putch(RxD()); }

void TSQNetz::trmstart()	// macht den Netzblock fertig und schickt ihn los
{ short temp;
  temp=getch();		// erstes Zeichen holen
  if (temp>=0) {
   RS485on();
   TxStart(temp); if (OnTxStart) OnTxStart(this);
  };
}

// sucht einen Datenblock zum Senden
void TSQNetz::finddatenblock(void)
{ unsigned char k;
  if (Daten) return; 	// Es steht schon ein Block zur Verfügung
  k=DataIdx;		// Suche nach dem letzten beginnen
  do {
   ++k; if (!(*TxCh[k])) k=0;
   if ((*TxCh[k])->State==BLK_READY) {
    DataIdx=k; Daten=*TxCh[k];
    return;
   };
  } while (DataIdx!=k);
  Daten=0;
}

bool TSQNetz::ismaster()
{ return (Self==1); }

void TSQNetz::rcvState()
{
}

void TSQNetz::trmD()
{ if (RFlag) --RFlag; // einen Netzinterrupt abwarten bevor gesendet wird
  else {
   if (Daten) {
    TrmPnt=Daten; Daten=0;
    RS485on(); AAtrm=false; TxStart(0xAA); if (OnTxStart) OnTxStart(this);
   };
   Mode=&TSQNetz::wait;
  };
}

void TSQNetz::wait()
{  if (TrmPnt==0) {
    if (sofort) {
     Daten=sofort; sofort=0; Daten->State=BLK_READY; RFlag=2; Mode=&TSQNetz::trmD;
    } else finddatenblock();
   };
}
