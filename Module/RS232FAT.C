
#include <lpc_fifo.h>
#include <rs232fat.h>
#include <tools.h>
#include <timer.h>
#include <LPC2138_sys_cnfg.h>

#define BLOCKANFANG 0x100
const unsigned long NewBlk=(100 msec);
const unsigned long ConnectTimeOut=(1 sec);

bool xyx=false;
/*
; Berechnung des CCITT-Polynoms
; unsigned int CCITT(unsigned int CCITT, unsigned char X)
;	MOV  	R7,CCITTlow
;	MOV  	R6,CCITThigh
;	MOV  	R5,X
;	LCALL	_CCITT

_CCITT: MOV     A,R5
        XRL     A,R6
        MOV     R4,A    X1=X xor Hi
        CLR     C
        RRC     A
        RRC     A
        RRC     A
        RRC     A
        XRL     A,R4
        MOV     R5,A   ; R5=(A shr 4) xor A
        RR      A
        RR      A
        RR      A
        MOV     R4,A   ; R4=X
        ANL     A,#1FH
        XRL     A,R7
        MOV     R7,A
        MOV     A,R4
        RR      A
        ANL     A,#0F0H
        XRL     A,R7
        MOV     R6,A
        MOV     A,R4
        ANL     A,#0E0H
        XRL     A,R5
        MOV     R7,A
        RET

unsigned short CCITT1(unsigned char x, unsigned short cc)
{ unsigned char R4,A,R7,R5,R6;
  R6=cc >> 8;
  R7=cc & 0xFF;
  R5=x;
  A=R5 ^ R6; R4=A;
  A=(A >> 4) | ((A & 7)<<5);
  A=A ^ R4; R5=A;
  A=((A & 7)<<5) | (A>>3); R4=A;
  R7=(A & 0x1F) ^ R7;
  R6=((((R4 & 1)<<7) | (R4>>1)) & 0xF0) ^ R7;
  R7=(R4 & 0xE0) ^ R5;
  return (R6<<8)|R7;
}
unsigned short xxx;
unsigned short yyy;
const char AA1[20] = { 0xE5, 0x16, 0x1, 0x1E, 0x67, 0x67, 0x67, 0x9F, 0, 0, 0, 0xFF, 0xFF,0xFF, 0, 0, 0, 2, 0x21, 0xB8 };
void CCITest()
{ int i;
  xxx=0; for (i=0;i<sizeof(AA1);i++) xxx=CCITT(AA1[i],xxx);
  yyy=0; for (i=0;i<sizeof(AA1);i++) yyy=CCITT1(AA1[i],yyy);
  xxx=0;
}
*/

unsigned short CCITT(unsigned char x, unsigned short cc)
{ unsigned char R4,R7,R5;
  R7=cc & 0xFF;
  R4=x ^ (cc >> 8);
  R5=((R4 >> 4) | ((R4 & 7)<<5)) ^ R4;
  R4=((R5 & 7)<<5) | (R5>>3);
  R7=(R4 & 0x1F) ^ R7;
  return ( ((((((R4 & 1)<<7) | (R4>>1)) & 0xF0) ^ R7)<<8) | ((R4 & 0xE0) ^ R5));
}

TRS232FAT::TRS232FAT(int Lng) {
 initfifo(Rcv,sizeof(Rcv));
 initfifo(Trm,sizeof(Trm));
 BlkLng=Lng;
 reset();
 BlkCnt=0;
// CCITest();
}

void TRS232FAT::reset() {
 while (fifocnt(Rcv)) rdfifo(Rcv);
 setTimeStamp(&ConnectTime);
 connected=false;
 cnt=-1; BlkOK=false;
}

void TRS232FAT::delblock() {
 while (fifocnt(Rcv) && (fifoch(Rcv,0)>=BLOCKANFANG)) rdfifo(Rcv);
 while (fifocnt(Rcv) && (fifoch(Rcv,0)<BLOCKANFANG)) rdfifo(Rcv);
 BlkOK=false;
}

void TRS232FAT::del(unsigned int n) {
 for (n=n;n;--n) rdfifo(Rcv);
// BlkOK=false;
}

void TRS232FAT::wrblockanfang(unsigned char cx)
{ wrfifo(Trm,BLOCKANFANG|cx); CRCtrm=0;
}

void TRS232FAT::wrchar(unsigned char cx)
{ wrfifo(Trm,cx); CRCtrm=CCITT(cx,CRCtrm);
}

void TRS232FAT::wrblockende()
{ wrfifo(Trm,(CRCtrm>>8)&0xFF);
  wrfifo(Trm,CRCtrm&0xFF);
}

short TRS232FAT::getch() {
 return rdfifo(Trm);
}

void TRS232FAT::txd() {
 short cx;
 cx=getch();
 if (cx<0) TxStop(); else TxD(cx);
}

void TRS232FAT::putch(unsigned char cx) {
 unsigned long Tx;
 Tx=dTime(&RcvTimeout);
 setTimeStamp(&RcvTimeout);
// if (Tx > NewBlk) wrfifo(Rcv,BLOCKANFANG|(Tx/10000)); else wrfifo(Rcv,Tx/10000);
 if (xyx) return;
 if (Tx > NewBlk) wrfifo(Rcv,BLOCKANFANG|cx); else wrfifo(Rcv,cx);
}

void TRS232FAT::rxd() {
 while (!RxEmpty()) putch(RxD());
}

void TRS232FAT::read(PBlock Dst) {
 int n;
 Dst->State=BLK_OFF;
 Dst->cnt=-5;
 Dst->Kopf.Adr=0x01;
 Dst->Kopf.Abs=0x02;
 Dst->Kopf.Lng=BlkLng;
 Dst->Kopf.Nr=0;
 Dst->Kopf.Typ='I';
 for (n=0;n<BlkLng;++n) (*Dst->Buff)[n]=rdfifo(Rcv);
 BlkOK=false;
}

int TRS232FAT::scann() {
 short cx;
 unsigned int lng;
 if (BlkOK) return 1;
 if (fifocnt(Rcv)>=(Rcv[0]-1)) { cnt=-1; rdfifo(Rcv); };
 if (cnt<0) {
  cx=fifoch(Rcv,0);
  if (cx>=BLOCKANFANG) { cx=cx&0xFF; cnt=0; } else { rdfifo(Rcv); return -1; };
 } else {
  if (cnt>=fifocnt(Rcv)) return -1;
  cx=fifoch(Rcv,cnt);
  if (cx>=BLOCKANFANG) { cx=cx&0xFF; del(cnt); cnt=0; };
 };
 switch (cnt++) {
  case 0: CRCrcv=CCITT(cx,0); break;
  default: lng=BlkLng+1;
   if (cnt==lng) CRCrcv-=cx<<8;
   else if (cnt>=lng+1) {
    if ((CRCrcv-cx)==0) {
     BlkOK=true; cnt=-1;
     if (BlkCnt<100) ++BlkCnt;
     setTimeStamp(&ConnectTime); connected=true; return 1;
    } else { del(cnt); cnt=-1; };
   } else CRCrcv=CCITT(cx,CRCrcv);
  break;
 };
 return 0;
}

void TRS232FAT::writekmd(unsigned char cx) {
 CRCtrm=0;
 wrchar(cx);
 wrblockende();
}

void TRS232FAT::write(unsigned char *P, unsigned char Lng) {
 CRCtrm=0;
 while (Lng--) wrchar(*P++);
 wrblockende();
}

void TRS232FAT::Execute() {
 if ((fifocnt(Trm)>0) && (!isTxOn())) { TxStart(getch()); };
 do ; while (scann()==0);
 if (dTime(&ConnectTime)>=ConnectTimeOut) connected=false;
 if (!connected) BlkCnt=0;
}

