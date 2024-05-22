/** \file Ayantra.cpp
    \brief Main Datei
*/

/************************************************************************************
*
*  Koppler zwischen PC und SQ-Netzen
*  03.11.2006   Bernd Petzold
*
*************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <states.hpp>
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

#define bitWATCHDOG   bit23
volatile int KICKDOG;

void kickdog() {
 if ((KICKDOG&0x3)==0x3) {
  if (IO0PIN & bitWATCHDOG) IO0CLR=bitWATCHDOG; else IO0SET=bitWATCHDOG;
  KICKDOG=0;
 };
}
void StartUpCRCWatchdog() { KICKDOG=0xFF; kickdog(); }

#define	RS485senden bit6

#ifdef NOAUTOMSG
 #ifdef NOCHGMSG
  const char Version[] = "Mobile Traffic Interface 4.0.2_0003";
  #define VARIANTE 3
 #else
  const char Version[] = "Mobile Traffic Interface 4.0.2_0002";
  #define VARIANTE 2
 #endif
#else
 const char Version[] = "Mobile Traffic Interface 4.0.2_0001";
 #define VARIANTE 1
#endif

#define FLASHMEM _Pragma("location=\"FLASH\"")

typedef struct {
 unsigned short __VERSION;
 char __DATUM[14];
 short __MAJOR;
 short __MINOR;
 short __RELEASE;

 unsigned int checksumstart;
 unsigned int checksumend;
 int compiler;
} TVERSIONINFO;

FLASHMEM __root const TVERSIONINFO VERSIONINFO = {
  VARIANTE,
  __DATE__,
  4, 0, 2,
  (unsigned int)&__checksum_begin,
  (unsigned int)&__checksum_end,
   __VER__
};

const unsigned int checksumstart = (unsigned int)&__checksum_begin;
const unsigned int checksumend = (unsigned int)&__checksum_end;


/*************************************************************************
 *  Gloabale Variable
 ************************************************************************/
unsigned long IRQFLAG=0;

TMSState * MSState=0;

bool SQ2_Schalter=false;

/*************************************************************************
 * IRQ subroutine
 * Darf nicht verändert werden, damit die Taskumschaltung funktioniert
 *************************************************************************/

#pragma optimize=none
extern "C" __irq __arm void IRQ_Handler(void)
{  IRQSETSYS;
   ((void(*)())VICVectAddr)();  // ruft die jeweilige irq auf
   IRQDISABLE;
   if (__taskcnt==0) if (IRQFLAG==0) iidle();
}
/************************************************************************/

class TSQNetzLPC : public TSQNetz {
 protected:
  void RS485on() { IO0SET=RS485senden; };
  void RS485off() { IO0CLR=RS485senden; };
  void TxStart(char Cx) { U1IER_bit.THREIE = true; U1THR=Cx; };
  bool TxEmpty() { return (U1LSR_bit.TEMT!=0); };
 public:
  TSQNetzLPC(PSQParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,unsigned char Verb, unsigned char Anz)
   : TSQNetz(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { };
  void TxD(char Cx) { U1THR=Cx; };
  void TxStop() { U1IER_bit.THREIE = false; IO0CLR=RS485senden; };
  bool RxEmpty() { return (U1LSR_bit.DR==0); };
  unsigned short RxD() { return U1RBR; };
  unsigned short geterr() { return U1LSR; };	// wird aufgerufen wenn die Serielle Schnitstelle einen Fehler entdeckt
  void initports() { IO0CLR=RS485senden; IO0DIR|=RS485senden; };
};

volatile int imax=0;
class TMSNetzLPC : public TMSNetzAyantra {
 protected:
  void RS485off() { IO0CLR=RS485senden; };
  bool TxEmpty() { return (U1LSR_bit.TEMT!=0); };
 public:
  TMSNetzLPC(PNetParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,TVerbindung Verb, unsigned char Anz)
   : TMSNetzAyantra(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { };
  void TxD(char Cx) { U1THR=Cx; };
  void TxStop() {
		volatile int i=0;
		while (U1LSR_bit.TEMT==0) ++i;
		if (i>imax) imax=i;
		for (i=0;i<10;++i) ;
		U1IER_bit.THREIE = false;
		IO0CLR=RS485senden;
	};
  bool RxEmpty() { return (U1LSR_bit.DR==0); };
  unsigned short RxD() { return U1RBR; };
  unsigned short geterr() { return U1LSR; };	// wird aufgerufen wenn die Serielle Schnitstelle einen Fehler entdeckt
  void initports() { IO0CLR=RS485senden; IO0DIR|=RS485senden; };
  void RS485on() { IO0SET=RS485senden; };
  void TxStart(char Cx) { U1IER_bit.THREIE = true; U1THR=Cx; };
};

#define escchar 0x7F
#define escflag 0x7D

class TMSNetzTranspLPC : public TMSNetzLPC {
	private:
		uint8_t RXesc;
		uint8_t TXesc;
	protected:
  	void trmstart();
	public:
  	void trmnext();
		void rcvnext();
  TMSNetzTranspLPC(PNetParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,TVerbindung Verb, unsigned char Anz)
   : TMSNetzLPC(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { ExtFlag=true; };
};

void TMSNetzTranspLPC::trmnext(void)
{ int temp;
  if (TrmPnt==0) TxStop();
  else {
		if (TXesc!=0) {
			temp=TXesc;
			TXesc=0;
		} else {
			temp=getch();
			if ((temp==escchar) || (temp==escflag)) {
				TXesc=temp ^ 0x20;
				temp=escchar;
			};
		};
		if (temp>=0) TxD(temp); else TxStop();
	};
}


void TMSNetzTranspLPC::rcvnext(void)
{ int temp;
	while (!RxEmpty()) {
		temp=RxD();
		switch (temp) {
			case escflag:
				RXesc=0;
		  	if (RcvPnt>(PBlock)100) RcvPnt->State=BLK_OFF;
				RcvPnt=0;
  			setTimeStamp(&RcvTimeout);
			break;
			case escchar:
				RXesc=temp;
			break;
			default:
				if (RXesc!=0) {
					RXesc=0;
					temp^=0x20;
				};
				putch(temp);
			break;
		};
	};
}

//! macht den Netzblock fertig und schickt ihn los
void TMSNetzTranspLPC::trmstart()
{ TXesc=0;
	RS485on();
  TxStart(escflag); if (OnTxStart) OnTxStart(this);
}

class TRS232LPC: public TMTSprotocol {
//class TRS232LPC: public TRS232 { // TMTSprotocol {
 protected:
  void TxStart(unsigned char cx) { U0THR=cx; U0IER_bit.THREIE = true; };
  bool TxEmpty() { return (U0LSR_bit.TEMT!=0); };
  void TxStop() { U0IER_bit.THREIE = false; };
  void TxD(char Cx) { U0THR=Cx; };
  bool RxEmpty() { return (U0LSR_bit.DR==0); };
  unsigned char RxD() { return U0RBR; };
  virtual bool isTxOn() { return U0IER_bit.THREIE; };
 public:
  TRS232LPC() : TMTSprotocol() { Status=0; };
//  TRS232LPC() : TRS232() { Status=0; };
  unsigned short Status;
  short clrerror() { return U0LSR; };
};

#define FIQ_PERIODE 140 usec	// Timer0 Interruptperiode

const TDCB_UART configUART1SQ = { BD19200, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART1x = { BD19200, WordLength8, OneStopbit, NoParity, FIFORX1, 0 };
const TDCB_UART configUART1MS = { BD9600, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART1USA = { BD19200, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART1FAT = { BD38400, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART1new = { BD57600, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART0 = { BD57600, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };

const TNetParam MSParam9600 = {
    50 msec,	// TRM_TIMEOUT	Nach dieser Zeit muss das Senden abgeschlossen sein
  1000 usec,	// BLKPAUSE	Pause zwischen zwei Blöcken
  3000 usec,	// NEWBLK	Nach dieser Zeit ohne Zeichen wird ein neuer Block im Empfang gestartet
 		// Kabel
  3500 usec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
   100 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
    30 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
		// Funk
   200 msec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
   500 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
  1000 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
       false,	// DataCycle	Daten dürfen sofort gesendet werden
       true,	// DataFlag	Flag verriegelt den Masterblock nicht
			 0,
};

const TNetParam MSParam19200 = {
    30 msec,	// TRM_TIMEOUT	Nach dieser Zeit muss das Senden abgeschlossen sein
  1000 usec,	// BLKPAUSE	1500 Pause zwischen zwei Blöcken
  1500 usec,	// NEWBLK	2000 Nach dieser Zeit ohne Zeichen wird ein neuer Block im Empfang gestartet
		// Kabel
  3000 usec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
   100 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
   600 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
		// Funk
   200 msec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
   500 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
    10 sec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
       false,	// DataCycle	Daten dürfen sofort gesendet werden
       true,	// DataFlag	Flag verriegelt den Masterblock nicht
			 0,
};

const TNetParam FATParam38400 = {
    20 msec,	// TRM_TIMEOUT	Nach dieser Zeit muss das Senden abgeschlossen sein
  1000 usec,	// BLKPAUSE	Pause zwischen zwei Blöcken
   600 usec,	// NEWBLK	Nach dieser Zeit ohne Zeichen wird ein neuer Block im Empfang gestartet
 // Kabel
  1500 usec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
     2 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
    30 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
 // Funk
  2000 usec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
  2500 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
   100 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
       true,	// Daten dürfen sofort gesendet werden
       true,	// Flag verriegelt den Masterblock nicht
       false,   // Es wird immer nur ein Datenblock gesendet
			 0,
};

const TNetParam MSParam57600 = {
    30 msec,	// TRM_TIMEOUT	Nach dieser Zeit muss das Senden abgeschlossen sein
   700 usec,	// BLKPAUSE	1500 Pause zwischen zwei Blöcken
   500 usec,	// NEWBLK	2000 Nach dieser Zeit ohne Zeichen wird ein neuer Block im Empfang gestartet
		// Kabel
  1500 usec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
    30 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
    70 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
		// Funk
  2000 msec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
  2500 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
  1000 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
       false,	// DataCycle	Daten dürfen sofort gesendet werden
       true,	// DataFlag	Flag verriegelt den Masterblock nicht
       false, // Es wird immer nur ein Datenblock gesendet
			 3,
};


const TSQParam SQParam19200 = {
    30 msec,	// unsigned short TRM_TIMEOUT;	// Nach dieser Zeit muss das Senden abgeschlossen sein
  4000 usec,	// unsigned short BLKPAUSE;	// Pause zwischen zwei Blöcken
   100 msec,	// unsigned long  SLV_TIMEOUT;	// Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
    10 msec,	// unsigned int   CHWCtakt;
};

const PBlock SQRxChannels[]={ &caInfo,&caGlobal,&caInfo1,0};
const PBlock SQTxChannels[]={ &TxcaInfo,0};

// TNetz(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { };
TMSNetzLPC       MSUSAx(&MSParam19200,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,10,0xF,FUNK,1);
TMSNetzLPC       MSx(&MSParam9600,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,10,0xF,FUNK,1);
TSQNetzLPC       SQx(&SQParam19200,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,2,0,0,6);
TMSNetzLPC       FATx(&FATParam38400,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,5,0xF,FUNK,1);
TMSNetzTranspLPC MSUSAneux(&MSParam57600,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,2,0xF,FUNK,1);
TRS232LPC PC;

TMSNetz *LSA;

/* Konstanten                                                                              */
/*******************************************************************************************/
const unsigned char TASK_TIME = 1 msec / ( FIQ_PERIODE );  // Tasklaufzeit

#define RS485_uart0_int  1<<VIC_UART0
#pragma optimize=none
/* \brief Interrupt RS232
   Interrupt der RS232: \n
   IIR_THRE:	Sendeinterrupt, naechsten Zeichen aus FiFo holen oder Senden beenden
   IIR_RSL:	Fehlerbehandlungsroutine aufrufen
   IIR_CTI, IIR_RDA: Empfangsinterrupt, Zeichen von der RS232 holen und in den FiFo schreiben
*/
void irq_uart0()	// darf nicht unterbrochen werden wegen hoher Geschwindigkeit
{ switch((U0IIR>>1)&0x7) {
   case IIR_THRE: PC.txd(); break;		// continue sending data
   case IIR_RSL:  PC.clrerror(); break;	// error manage
   case IIR_CTI:				// time out  evt. getrennt behandeln
   case IIR_RDA:  PC.rxd(); break;		// receive data
   default: break;
  };
  VICVectAddr = 0;    			// Clear interrupt in VIC.
}

#pragma optimize=none
void irq_uart1_LSA()
{ VIC_DisableInt(1<<VIC_UART1);		// weitere UART1 Interrupts verbieten
  VICVectAddr = 0;    			// Clear interrupt in VIC.
  IRQFLAG|=(1<<VIC_UART1);
  enable_IRQ();				// andere Interrupts zulassen
  switch((U1IIR>>1)&0x7) {
   case IIR_THRE: LSA->trmnext(); break;	// continue sending data
   case IIR_RSL:  LSA->clrerr(); break;
   case IIR_CTI:			// time out  evt. getrennt behandeln
   case IIR_RDA:  LSA->rcvnext(); break;
  default: break;
  }
  disable_IRQ();
  IRQFLAG&=~(1<<VIC_UART1);
  VIC_EnableInt(1<<VIC_UART1);		//UART1 Interrupts wieder einschalten
}

// ca. 60µs
void irq_timer0()
{ VICVectAddr=0;
  if (T0IR==0) {			// Durch Software ausgelöster Interrupt
   VICSoftIntClear=(1<<VIC_TIMER0);
   if ((IRQFLAG&(1<<VIC_TIMER0))==0) {
    IRQFLAG|=(1<<VIC_TIMER0);
    enable_IRQ();
    LSA->Execute();
    disable_IRQ();
    IRQFLAG&=~(1<<VIC_TIMER0);
   };
  } else {
//   IO1DIR=bit19;
//   IO1SET=bit19;
   TIMER0_ClearInt(0x1);
   SQx.wrstatus();
  };
  if (__taskcnt) --__taskcnt;

//  IO1CLR=bit19;
}

// wird aufgerufen beim Empfang eines SQ-Netzblockes
// Flag=1  Umschaltung auf Statusempfang
//     =2  Nächsten Status abwarten
//     =3  Umschalten auf Blockempfang
int OnSyncNetz(TSQNetz *NET,int Flag)
{ switch (Flag) {
   case 1:
    UART_Init(UART1,&configUART1x);
    TIMER0_SetMatch(CH0, TimerAction_Interrupt , SystemZeit()+3412);
   break;
   case 2:
    TIMER0_AddMatch(CH0, 1088);
   break;
   case 3:
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1SQ);
   break;
  };
  return 0;
}

/*************************************************************************
 * Function Name: IRQSub
 * Parameters: void
 * Return: void
 *
 * Description: FIQ subroutine  Periode=200µs
 *
 *************************************************************************/
void testbit(bool x)
{ if (x) { IO0DIR|=bit31; IO0SET=bit31; } else { IO0DIR|=bit31; IO0CLR=bit31; };
}
volatile short testpattern=0;
//#pragma vector=FIQV
extern "C" __irq __arm void FIQ_Handler (void)
{
/*  testbit(true); testbit(false);
  testbit(testpattern & 8);
  testbit(testpattern & 4);
  testbit(testpattern & 2);
  testbit(testpattern & 1);
  testbit(false);
*/
  VICSoftInt=(1<<VIC_TIMER0);  // Startet den normalen Timerinterrupt
  TIMER1_ClearInt(0x1);
}

//#pragma vector=UNDEFV
extern "C" __irq void UND_Handler()
{
}

//#pragma vector=SWIV
extern "C" __irq void SWI_ISR_Handler()
{
  VICSoftIntClear=(1<<VIC_SW);
}

//#pragma vector=PABORTV
extern "C" __irq void ABORT1_ISR_Handler()
{
}

//#pragma vector=DABORTV
extern "C" __irq void ABORT2_ISR_Handler()
{
}

void NonVectISR(void)
{
}


/*************************************************************************
 * Function Name: SysInit
 * Parameters: void
 * Return: int
 *
 * Description: Hardware initialize
 *
 *************************************************************************/
int SysInit(void)
{ // Initialize the system
                                                //DIR0 P0          DIR1 P1
  if ( SYS_Init(FOSC, FCCLK, VPBDIV1, USER_FLASH, 0,   0xFFFFFFFF, 0,   0xFFFFFFFF) ) return 1;

// Serielle Schnittstelle 0 initialisieren damit die Pegel richtig stehen
  UART_Init(UART0,&configUART0);


// Flash Zugriff optimieren
  MAMTIM_bit.CYCLES=3;
  MAMCR_bit.MODECTRL=2;

  SQx.initports();
  SQx.OnSyncNetz=OnSyncNetz;

  MSx.initports();

//  ADC_Init(ADUpow);
//  ADC_Init(ADUhell);

// initialize VIC
  VIC_Init();
  VIC_SetProtectionMode(UserandPrivilegedMode);
  // Enable interrupts non vectored interrupts
  VIC_EnableNonVectoredIRQ(NonVectISR);

// Timer1 für den Fast Interrupt fertigmachen muss für PWM geeignet sein
  TIMER1_Init(PRESCALER); // Prescaler auf TIMER_PRECISON µs einstellen
  TIMER1_Stop();
  TIMER1_Reset();
  VICIntSelect|=(1<<VIC_TIMER1);  // FIQ auswählen
  VIC_EnableInt(1<<VIC_TIMER1);
  TIMER1_SetMatch(CH0, TimerAction_Interrupt | TimerAction_ResetTimer, FIQ_PERIODE);
  TIMER1_Start();

// Timer0 für alle übrigen Aktionen, Timer1 liefert auch die Systemzeit, deshalb läuft er durch
  TIMER0_Init(PRESCALER);
  TIMER0_Stop();
  TIMER0_Reset();
//  TIMER0_SetMatch(CH0, TimerAction_Interrupt , TASK_PERIODE);
  VIC_SetVectoredIRQ(irq_timer0,VIC_Slot0,VIC_TIMER0);
  VIC_EnableInt(1<<VIC_TIMER0);
  TIMER0_Start();

// serielle Schnittstelle 0 für PC
  UART_Init(UART0,&configUART0);
  VIC_SetVectoredIRQ(irq_uart0,VIC_Slot2,VIC_UART0);
  VIC_EnableInt(1<<VIC_UART0);

  // Portbit P1:25 für Microterminalkennzeichnung
  // MULTI -> High
  // SQ -> Low
  IO1DIR|=bit25;
  IO1CLR=bit25;

  IO0DIR|=bitWATCHDOG;
  IO0CLR=bitWATCHDOG;

  return 0;
}


//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------

// Alle Angaben in 0,1 s
/*
class TMSInputsx {
 public:
  unsigned short MT: 1;
  unsigned short sensor: 1;
  unsigned short taster: 1;
  unsigned short sensor2: 1;
  operator short();              // ein Konvertierungsoperator
  TMSInputsx operator=(int op2);
};

TMSInputsx TMSInputsx::operator=(int op2) {
  MT=((op2 & 1)!=0);
  return *this;
}

TMSInputsx::operator short()
{ return (MT | (sensor<<1) | (taster<<2) | (sensor2<<3));
}
*/


TASK (ayantra,200)
{ MSState->trmversion(PC);
  while (true) {
   MSState->processRS232();
   PC.Execute();
   KICKDOG|=2;
   idle();
  };
}

void SetNetzTyp(int Typ)
{ switch (Typ) {
   case SQ2:
    VIC_DisableInt(1<<VIC_UART1);
    SQx.Self=0;
    IO1CLR=bit25;
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1SQ);
    VIC_SetVectoredIRQ(irq_uart1_LSA,VIC_Slot1,VIC_UART1);
    NetzTyp=SQ2;
//    Parameter.Typ=Typ;
    VIC_EnableInt(1<<VIC_UART1);
   break;
   case MULTI:
    VIC_DisableInt(1<<VIC_UART1);
    IO1SET=bit25;
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1MS);
    VIC_SetVectoredIRQ(irq_uart1_LSA,VIC_Slot1,VIC_UART1);
    NetzTyp=MULTI;
//    Parameter.Typ=Typ;
    VIC_EnableInt(1<<VIC_UART1);
   break;
   case USA:
    VIC_DisableInt(1<<VIC_UART1);
    IO1SET=bit25;
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1USA);
    VIC_SetVectoredIRQ(irq_uart1_LSA,VIC_Slot1,VIC_UART1);
    NetzTyp=USA;
		LSA=&MSUSAx;
//    Parameter.Typ=Typ;
    VIC_EnableInt(1<<VIC_UART1);
   break;
   case USAneu:
    VIC_DisableInt(1<<VIC_UART1);
    IO1SET=bit25;
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1new);
    VIC_SetVectoredIRQ(irq_uart1_LSA,VIC_Slot1,VIC_UART1);
    NetzTyp=USAneu;
		LSA=&MSUSAneux;
//    Parameter.Typ=Typ;
    VIC_EnableInt(1<<VIC_UART1);
   break;
   case FATTYP:
    VIC_DisableInt(1<<VIC_UART1);
    IO1SET=bit25;
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1FAT);
    VIC_SetVectoredIRQ(irq_uart1_LSA,VIC_Slot1,VIC_UART1);
    NetzTyp=FATTYP;
		LSA=&FATx;
//    Parameter.Typ=Typ;
    VIC_EnableInt(1<<VIC_UART1);
   break;
  };
}

// Kann evt. zu Konflikten führen, da das Beschreiben des FiFo im Interrupt passiert.
void doOnNetRcv(TMSNetz *NET, TMSNet_State &State)
{
//  if ((State.Kopf.Adr==0x90) && (State.Kopf.Abs==0x0F)) PC.wrblock(NetzTyp,0x90,0x0F,0,State.Kopf.Nr,State.Kopf.Typ,0);
//  if (State.Kopf.Adr==0x9D) PC.wrblock(NetzTyp,0x9D,0,0,0,0,0);
}

void doOnGetState(TSQNetz *SQ2)
{ SQ2->SelfState.schalter=SQ2_Schalter;
  SQ2->SelfState.all|=0x70;
};

TMSStat doOnGetStateFAT(TMSNetz *NET, int & lng)	// wird aufgerufen bevor der Statusblock gesendet wird
{ return PC.Status;
};

extern int __mem1[200]; //wird in taskasm.s79 definiert, ist der Stack des MainTask

// im Maintask fuktioniert taskwait nicht weil kein Speicherbereich zur Verfügung steht ???

/*! \brief Main Function

    Initialisiert die Hardware und startet die Interrupts und Tasks
    Setzt MSState zurueck.
    Sobald ein Netzblock empfangen wurde wird MSState.execute(..) aufgerufen.
    Sobald auf caInfo oder caInfo1 ein block empfangen wurde wird MSState.execute(..) aufgerufen
    und caInfo bzw. caInfo1 wieder auf Empfang gestellt.
    MSState.execute wird zyklisch aufgerufen
*/
int main()
{ unsigned long Cnt;
  int Typ;
	unsigned long dt;
  SysInit();
  while (CheckFlashCRC(StartUpCRCWatchdog)) ;
  memset(&__mem1,0xAA,sizeof(__mem1));
  memset(&ayantra.mem,0xAA,sizeof(ayantra.mem));
  idle();
  setreceive(&caInfo,0x00,0,sizeof(InfoBuff),&InfoBuff);
  setreceive(&caInfo1,0x00,0,sizeof(InfoBuff1),&InfoBuff1);
  MSx.OnNetRcv=doOnNetRcv;       // ermöglicht die Inbetriebnahme
  MSUSAx.OnNetRcv=doOnNetRcv;
  MSUSAneux.OnNetRcv=doOnNetRcv;
  FATx.OnGetState=doOnGetStateFAT;
  SQx.OnGetState=doOnGetState;

  SetNetzTyp(USAneu);
  Typ=0;
	__enable_interrupt();

  while (Typ==0) {
		if (Typ==0) {
    	SetNetzTyp(USA); setTimeStamp(&dt);
    	while ((Typ==0) && (dTime(&dt)<(2000 msec))) {
				StartUpCRCWatchdog();
     		if (LSA->MasterBlkCnt>=2) Typ=USA;
    	};
   	};
   	if (Typ==0) {
			LSA->MasterBlkCnt=0;
    	SetNetzTyp(USAneu); setTimeStamp(&dt);
    	while ((Typ==0) && (dTime(&dt)<(2000 msec))) {
				StartUpCRCWatchdog();
     		if (LSA->MasterBlkCnt>=2) Typ=USAneu;
    	};
   	};
  };


//	SetNetzTyp(USAneu); Typ=USAneu;

	switch (Typ) {
		case USAneu: MSState=new TMSStateNeu(PC); break;
		default: MSState=new TMSState(PC); break;
	};

	MSState->reset();
  setTimeStamp(&Cnt);  // 12
  taskinit(&ayantra.task,0,ayantra_start,0);

  while (true) {
    if (LSA->Flag) { MSState->processblk(*LSA); LSA->Flag=false; };


   switch (caInfo.State) {
    case BLK_OK:
     MSState->processblk(caInfo);
     setreceive(&caInfo,0x00,0,sizeof(InfoBuff),&InfoBuff); break;
    case BLK_RCV: case BLK_WAIT: break;
    default: setreceive(&caInfo,0x00,0,sizeof(InfoBuff),&InfoBuff); break; // Empfängt sämtliche Blöcke
   };

   switch (caInfo1.State) {
    case BLK_OK:
     MSState->processblk(caInfo1);
     setreceive(&caInfo1,0x00,0,sizeof(InfoBuff1),&InfoBuff1); break;
    case BLK_RCV: case BLK_WAIT: break;
    default: setreceive(&caInfo1,0x00,0,sizeof(InfoBuff1),&InfoBuff1); break; // Empfängt sämtliche Blöcke
   };
   kickdog();
   MSState->execute();
   KICKDOG|=1;
  };
}
