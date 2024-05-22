/************************************************************************************
*
*  Koppler zwischen PC und SQ-Netzen
*  03.11.2006   Bernd Petzold
*
*************************************************************************************/

#include <includes.h>
#include <task.h>
#include <timer.h>
#include <uart.h>
#include <LPC_fifo.h>
#include <RS232.h>
#include <RS485.h>
#include <SQ485.h>
#include <LPC_IAP.h>
#include <tools.h>
#include <stdint.h>

#define	RS485senden bit6

/************************************************************************/
#define VARIANTE 1
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
  1, 1, 4,
  (unsigned int)&__checksum_begin,
  (unsigned int)&__checksum_end,
   __VER__
};

const unsigned int checksumstart = (unsigned int)&__checksum_begin;
const unsigned int checksumend = (unsigned int)&__checksum_end;
/************************************************************************/

/*************************************************************************
 *  Gloabale Variable
 ************************************************************************/
unsigned long IRQFLAG=0;
enum { SQ2=1, MULTI, USA, EURO, FATTYP, USAneu, STORE=0xFF } NetzTyp;   // 1 2 3 4 5 6
bool SQ2_Schalter=false;
int FKmd=0;

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

bool isRS485on() { return ((IO0PIN & RS485senden)!=0); };

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
//  void TxStop() { U1IER_bit.THREIE = false; IO0CLR=RS485senden; };
  void TxStop() {
		volatile int i=0;
		while (U1LSR_bit.TEMT==0) ;
		for (i=0;i<10;++i) ;
		U1IER_bit.THREIE = false;
		IO0CLR=RS485senden;
	};
  bool RxEmpty() { return (U1LSR_bit.DR==0); };
  unsigned short RxD() { return U1RBR; };
  unsigned short geterr() { return U1LSR; };	// wird aufgerufen wenn die Serielle Schnitstelle einen Fehler entdeckt
  void initports() { IO0CLR=RS485senden; IO0DIR|=RS485senden; };
};

volatile int imax=0;
class TMSNetzLPC : public TMSNetz {
 protected:
  void RS485off() { IO0CLR=RS485senden; };
  bool TxEmpty() { return (U1LSR_bit.TEMT!=0); };
 public:
  TMSNetzLPC(PNetParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,TVerbindung Verb, unsigned char Anz)
   : TMSNetz(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { };
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

/******************************************************************************/

class TRS232LPC: public TRS232 {
 protected:
  void TxStart(unsigned char cx) { U0THR=cx; U0IER_bit.THREIE = true; };
  bool TxEmpty() { return (U0LSR_bit.TEMT!=0); };
  void TxStop() { U0IER_bit.THREIE = false; };
  void TxD(char Cx) { U0THR=Cx; };
  bool RxEmpty() { return (U0LSR_bit.DR==0); };
  unsigned char RxD() { return U0RBR; };
  virtual bool isTxOn() { return U0IER_bit.THREIE; };
 public:
  TRS232LPC() : TRS232() { Status=0; };
  unsigned short Status;
  short clrerror() { return U0LSR; };
};


#ifdef KICKSTART
#endif
#ifdef KOPPLER
#endif

#define FIQ_PERIODE 140 usec	// Timer0 Interruptperiode

const TDCB_UART configUART1SQ = { BD19200, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART1x = { BD19200, WordLength8, OneStopbit, NoParity, FIFORX1, 0 };
const TDCB_UART configUART1MS = { BD9600, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART1USA = { BD19200, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART1USAneu = { BD57600, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART1FAT = { BD38400, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART0 = { BD57600, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };

TBlock caGlobal={ 20 };
unsigned char InfoBuff[50];
TBlock caInfo;		// Typ MultiSignalBlock
unsigned char InfoBuff1[50];
TBlock caInfo1;		// Typ MultiSignalBlock
unsigned char TxInfoBuff[50];
TBlock TxcaInfo;
unsigned char TxSofortBuff[50];
TBlock TxSofort;
unsigned char TxTestBuff[50];
TBlock TxTest;

short TxCharBuff[20];

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
			 0,			// Erweiterungsadresse auf MT-Platz '0..7' 0=Alle
};

const TNetParam MSParam19200 = {
    30 msec,	// TRM_TIMEOUT	Nach dieser Zeit muss das Senden abgeschlossen sein
  1000 usec,	// BLKPAUSE	1500 Pause zwischen zwei Blöcken
  1500 usec,	// NEWBLK	2000 Nach dieser Zeit ohne Zeichen wird ein neuer Block im Empfang gestartet
		// Kabel
  3000 usec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
   100 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
    30 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
		// Funk
   200 msec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
   500 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
  1000 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
       false,	// DataCycle	Daten dürfen sofort gesendet werden
       true,	// DataFlag	Flag verriegelt den Masterblock nicht
			 0,			// Erweiterungsadresse auf MT-Platz '0..7' 0=Alle
};

const TNetParam MSParam57600 = {
    30 msec,	// TRM_TIMEOUT	Nach dieser Zeit muss das Senden abgeschlossen sein
   700 usec,	// BLKPAUSE	1500 Pause zwischen zwei Blöcken
   500 usec,	// NEWBLK	2000 Nach dieser Zeit ohne Zeichen wird ein neuer Block im Empfang gestartet
		// Kabel
  1500 usec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
    30 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
    30 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
		// Funk
  2000 msec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
  2500 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
   200 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
       false,	// DataCycle	Daten dürfen sofort gesendet werden
       true,	// DataFlag	Flag verriegelt den Masterblock nicht
       false, // Es wird immer nur ein Datenblock gesendet
			 2,			// Erweiterungsadresse auf MT-Platz '0..7' 0=Alle
};

const TNetParam FATParam38400 = {
    20 msec,	// TRM_TIMEOUT	Nach dieser Zeit muss das Senden abgeschlossen sein
  1000 usec,	// BLKPAUSE	Pause zwischen zwei Blöcken
   600 usec,	// NEWBLK	Nach dieser Zeit ohne Zeichen wird ein neuer Block im Empfang gestartet
 // Kabel
  1500 usec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
    50 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
    30 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
 // Funk
  2000 usec,	// RCV_TIMEOUT	Nach dieser Zeit ohne Zeichen startet der Master ein Ping
  2500 msec,	// RCV_TERMINAL	Wartezeit auf das Terminal (so lang wegen Reaktionszeit PC)
   100 msec,	// SLV_TIMEOUT	Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
       true,	// Daten dürfen sofort gesendet werden
       true,	// Flag verriegelt den Masterblock nicht
       false,   // Es wird immer nur ein Datenblock gesendet
       0,			// Erweiterungsadresse auf MT-Platz '0..7' 0=Alle
};

const TSQParam SQParam19200 = {
    30 msec,	// unsigned short TRM_TIMEOUT;	// Nach dieser Zeit muss das Senden abgeschlossen sein
  4000 usec,	// unsigned short BLKPAUSE;	// Pause zwischen zwei Blöcken
   100 msec,	// unsigned long  SLV_TIMEOUT;	// Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
    10 msec,	// unsigned int   CHWCtakt;
};

const PBlock SQRxChannels[]={ &caInfo,&caGlobal,&caInfo1,
                                 0};
const PBlock SQTxChannels[]={ &TxcaInfo, &TxTest,
                                 0};

// TNetz(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { };
TMSNetzLPC MSUSA(&MSParam19200,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,10,0xF,FUNK,1);
TMSNetzTranspLPC MSUSAneu(&MSParam57600,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,2,0xF,FUNK,1);
TMSNetzLPC MS(&MSParam9600,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,10,0xF,FUNK,1);
TSQNetzLPC SQ(&SQParam19200,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,2,0,0,6);
TMSNetzLPC FAT(&FATParam38400,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,5,0xF,FUNK,1);
TRS232LPC PC;

/* Konstanten                                                                              */
/*******************************************************************************************/
const unsigned char TASK_TIME = 1 msec / ( FIQ_PERIODE );  // Tasklaufzeit

#define RS485_uart0_int  1<<VIC_UART0
#pragma optimize=none
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

// Senden ca. 20µs pro Teilnehmer
// Empfangen ca. 60µs pro Teilnehmer damit ist bei 57600 wahrscheinlich Schluss
#pragma optimize=none
void irq_uart1_SQ()
{ VIC_DisableInt(1<<VIC_UART1);		// weitere UART1 Interrupts verbieten
  VICVectAddr = 0;    			// Clear interrupt in VIC.
  IRQFLAG|=(1<<VIC_UART1);
  enable_IRQ();				// andere Interrupts zulassen
  switch((U1IIR>>1)&0x7) {
   case IIR_THRE: SQ.trmnext(); break;	// continue sending data
   case IIR_RSL:  SQ.clrerr(); break;
   case IIR_CTI:			// time out  evt. getrennt behandeln
   case IIR_RDA:  SQ.rcvnext(); break;
  default: break;
  }
  disable_IRQ();
  IRQFLAG&=~(1<<VIC_UART1);
  VIC_EnableInt(1<<VIC_UART1);		//UART1 Interrupts wieder einschalten
}

#pragma optimize=none
void irq_uart1_MS()
{ VIC_DisableInt(1<<VIC_UART1);		// weitere UART1 Interrupts verbieten
  VICVectAddr = 0;    			// Clear interrupt in VIC.
  IRQFLAG|=(1<<VIC_UART1);
  enable_IRQ();				// andere Interrupts zulassen
  switch((U1IIR>>1)&0x7) {
   case IIR_THRE: MS.trmnext(); break;	// continue sending data
   case IIR_RSL:  MS.clrerr(); break;
   case IIR_CTI:			// time out  evt. getrennt behandeln
   case IIR_RDA:  MS.rcvnext(); break;
  default: break;
  }
  disable_IRQ();
  IRQFLAG&=~(1<<VIC_UART1);
  VIC_EnableInt(1<<VIC_UART1);		//UART1 Interrupts wieder einschalten
}

#pragma optimize=none
void irq_uart1_MSUSA()
{ VIC_DisableInt(1<<VIC_UART1);		// weitere UART1 Interrupts verbieten
  VICVectAddr = 0;    			// Clear interrupt in VIC.
  IRQFLAG|=(1<<VIC_UART1);
  enable_IRQ();				// andere Interrupts zulassen
  switch((U1IIR>>1)&0x7) {
   case IIR_THRE:
	 	if (fifocnt(TxCharBuff)>0) {
			MSUSA.TxD(rdfifo(TxCharBuff));
		} else {
			MSUSA.trmnext();
		};
	 break;	// continue sending data
   case IIR_RSL:  MSUSA.clrerr(); break;
   case IIR_CTI:			// time out  evt. getrennt behandeln
   case IIR_RDA:  MSUSA.rcvnext(); break;
  default: break;
  }
  disable_IRQ();
  IRQFLAG&=~(1<<VIC_UART1);
  VIC_EnableInt(1<<VIC_UART1);		//UART1 Interrupts wieder einschalten
}

#pragma optimize=none
void irq_uart1_MSUSAneu()
{ VIC_DisableInt(1<<VIC_UART1);		// weitere UART1 Interrupts verbieten
  VICVectAddr = 0;    			// Clear interrupt in VIC.
  IRQFLAG|=(1<<VIC_UART1);
  enable_IRQ();				// andere Interrupts zulassen
  switch((U1IIR>>1)&0x7) {
   case IIR_THRE: MSUSAneu.trmnext(); break;	// continue sending data
   case IIR_RSL:  MSUSAneu.clrerr(); break;
   case IIR_CTI:			// time out  evt. getrennt behandeln
   case IIR_RDA:  MSUSAneu.rcvnext(); break;
  default: break;
  }
  disable_IRQ();
  IRQFLAG&=~(1<<VIC_UART1);
  VIC_EnableInt(1<<VIC_UART1);		//UART1 Interrupts wieder einschalten
}

#pragma optimize=none
void irq_uart1_FAT()
{ VIC_DisableInt(1<<VIC_UART1);		// weitere UART1 Interrupts verbieten
  VICVectAddr = 0;    			// Clear interrupt in VIC.
  IRQFLAG|=(1<<VIC_UART1);
  enable_IRQ();				// andere Interrupts zulassen
  switch((U1IIR>>1)&0x7) {
   case IIR_THRE: FAT.trmnext(); break;	// continue sending data
   case IIR_RSL:  FAT.clrerr(); break;
   case IIR_CTI:			// time out  evt. getrennt behandeln
   case IIR_RDA:  FAT.rcvnext(); break;
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
    switch (NetzTyp) {
     case SQ2: SQ.Execute(); break;
     case MULTI: MS.Execute(); break;
     case USA: MSUSA.Execute(); break;
     case USAneu: MSUSAneu.Execute(); break;
     case FATTYP: FAT.Execute(); break;
    };
    disable_IRQ();
    IRQFLAG&=~(1<<VIC_TIMER0);
   };
  } else {
//   IO1DIR=bit19;
//   IO1SET=bit19;
   TIMER0_ClearInt(0x1);
   SQ.wrstatus();
  };
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

//#pragma vector=FIQV
extern "C" __irq __arm void FIQ_Handler (void)
{
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

//void getRS485MasterDat(TSQNetz *NET)
//{  NET->State.Daten.N.CHWC=SystemZeit() & 0xFF;
//   NET->TrmBuf.Kopf.Lng=2;
//}

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
#ifdef KICKSTART
 #ifdef FLASH                                     //DIR0 P0          DIR1 P1
  if ( SYS_Init(FOSC, FCCLK, VPBDIV1, USER_FLASH, 0,     0xFFFFFFFF, 0,   0xFFFFFFFF) ) return 1;
 #else
  if ( SYS_Init(FOSC, FCCLK, VPBDIV1, USER_RAM,   0,     0xFFFFFFFF, 0,   0xFFFFFFFF) ) return 1;
 #endif
#else
 #ifdef FLASH                                     //DIR0 P0          DIR1 P1
  if ( SYS_Init(FOSC, FCCLK, VPBDIV1, USER_FLASH, 0,     0xFFFFFFFF, 0,   0xFFFFFFFF) ) return 1;
 #else
  if ( SYS_Init(FOSC, FCCLK, VPBDIV1, USER_RAM,   0,     0xFFFFFFFF, 0,   0xFFFFFFFF) ) return 1;
 #endif
#endif

// Serielle Schnittstelle 0 initialisieren damit die Pegel richtig stehen
  UART_Init(UART0,&configUART0);


// Flash Zugriff optimieren
  MAMTIM_bit.CYCLES=3;
  MAMCR_bit.MODECTRL=2;

  SQ.initports();
  SQ.OnSyncNetz=OnSyncNetz;
//  SQ.OnGetMasterDat=getRS485MasterDat;

  MS.initports();


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
  IO1DIR=bit25;
  IO1CLR=bit25;

  return 0;
}

void loadN(TSQNetz &SQ, TRS232 &PC)
{ int n;
  if (fifocnt(PC.Trm)<100) {
   PC.wrblockanfang(SQ2);	// Block kommt von der SQ
   PC.wrchar(SQ.State.Kopf.Adr);
   PC.wrchar(SQ.State.Kopf.Abs);
   PC.wrchar(SQ.State.Kopf.Lng);
   PC.wrchar(SQ.State.Kopf.Nr);
   PC.wrchar(SQ.State.Kopf.Typ);
   for (n=0;n<SQ.State.Kopf.Lng;++n) PC.wrchar(*((PBuff)&SQ.State.Daten)[n]);
   PC.wrblockende();

   PC.wrblockanfang(SQ2);	// Block kommt von der SQ
   PC.wrchar(0xFF);
   PC.wrchar(0);
   PC.wrchar(SQ_NET_MAXANZAHL+4);
   PC.wrchar(1);
   PC.wrchar(0xFF);
   PC.wrchar(SQ.State.AND.all);
   PC.wrchar(SQ.State.OR.all);
   for (n=0;n<SQ_NET_MAXANZAHL+2;++n) PC.wrchar(SQ.State.State[n].status.all);
   PC.wrblockende();
  };
}

void loadMulti(TMSNetzLPC &MS, TRS232 &PC)
{ int i,n;
  uint8_t cx;
  TMSStat St;

  PC.wrblockanfang(MULTI);	// Block kommt von der MS
  PC.wrchar(MS.State.Kopf.Adr);
  PC.wrchar(MS.State.Kopf.Abs);
  n=MS.State.Kopf.Lng;
  PC.wrchar(n);
  PC.wrchar(MS.State.Kopf.Nr);
  PC.wrchar(MS.State.Kopf.Typ);
  for (i=0;i<n;++i) PC.wrchar(*((PBuff)&MS.State.MasterDaten)[i]);
  PC.wrblockende();

  PC.wrblockanfang(MULTI);	// Block kommt von der MS
  PC.wrchar(0xFF);
  PC.wrchar(0);
  n=MS.Anzahl; if (n>RS485_NET_MAXANZAHL-2) n=RS485_NET_MAXANZAHL-2;
  PC.wrchar(n*4+6);
  PC.wrchar(1);
  PC.wrchar(0xFF);
  PC.wrchar(MS.GlobalQuit>>8);
  PC.wrchar(MS.GlobalQuit&0xFF);
  PC.wrchar(MS.State.AND>>8);
  PC.wrchar(MS.State.AND&0xFF);
  PC.wrchar(MS.State.OR>>8);
  PC.wrchar(MS.State.OR&0xFF);
  for (i=1;i<=n;++i) {
   St=MS.State.State[i];
	 cx=St>>24;
	 if (cx>10)
		 cx=0;
	 else
		 cx=1;
	 cx=St>>16;
	 cx=St>>8;
	 cx=St & 0xFF;
   PC.wrchar((St>>24)&0xFF);
	 PC.wrchar((St>>16)&0xFF);
   PC.wrchar((St>>8)&0xFF);
   PC.wrchar(St & 0xFF);
  };
  PC.wrblockende();
}

typedef struct {
 int Typ;
 unsigned short CRC;
} TEinstellungen;

__no_init TEinstellungen Parameter0 @ 0x7000;
TEinstellungen Parameter = { SQ2, 0};

unsigned short memcrc(void *Adr,unsigned short n)
{ int i;
  unsigned short CRC;
  CRC=0;
  for (i=0;i<n;++i) {
   CRC=calc_crc(*((unsigned char *)Adr+i),CRC);
  };
  return CRC;
}

void StoreParameter()
{ Parameter.CRC=memcrc(&Parameter,(int)&Parameter.CRC-(int)&Parameter);
  if (memcmp(&Parameter0,&Parameter,sizeof(Parameter0))) {
   iap_prepare(7,7); iap_erase(7,7); iap_prepare(7,7);
   iap_write((int)&Parameter0,(int)&Parameter,256);
  };
}

void SetNetzTyp(int Typ)
{ switch (Typ) {
   case SQ2:
    VIC_DisableInt(1<<VIC_UART1);
    SQ.Self=0;
    IO1CLR=bit25;
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1SQ);
    VIC_SetVectoredIRQ(irq_uart1_SQ,VIC_Slot1,VIC_UART1);
    NetzTyp=SQ2;
    Parameter.Typ=Typ;
    VIC_EnableInt(1<<VIC_UART1);
   break;
   case MULTI:
    VIC_DisableInt(1<<VIC_UART1);
    IO1SET=bit25;
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1MS);
    VIC_SetVectoredIRQ(irq_uart1_MS,VIC_Slot1,VIC_UART1);
    NetzTyp=MULTI;
    Parameter.Typ=Typ;
    VIC_EnableInt(1<<VIC_UART1);
   break;
   case USA:
    VIC_DisableInt(1<<VIC_UART1);
    IO1SET=bit25;
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1USA);
    VIC_SetVectoredIRQ(irq_uart1_MSUSA,VIC_Slot1,VIC_UART1);
    NetzTyp=USA;
    Parameter.Typ=Typ;
    VIC_EnableInt(1<<VIC_UART1);
   break;
   case USAneu:
    VIC_DisableInt(1<<VIC_UART1);
    IO1SET=bit25;
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1USAneu);
    VIC_SetVectoredIRQ(irq_uart1_MSUSAneu,VIC_Slot1,VIC_UART1);
    NetzTyp=USAneu;
    Parameter.Typ=Typ;
    VIC_EnableInt(1<<VIC_UART1);
   break;
   case FATTYP:
    VIC_DisableInt(1<<VIC_UART1);
    IO1SET=bit25;
    TIMER0_SetMatch(CH0,0,0);
    UART_Init(UART1,&configUART1FAT);
    VIC_SetVectoredIRQ(irq_uart1_FAT,VIC_Slot1,VIC_UART1);
    NetzTyp=FATTYP;
    Parameter.Typ=Typ;
    VIC_EnableInt(1<<VIC_UART1);
   break;
   case STORE:
    StoreParameter();
   break;
  };
}

int BlkID(int ID) {
	switch (ID) {
		case SQ2:    return SQ2;
		case MULTI:  return MULTI;
		case USA:    return USA;
		case EURO:   return EURO;
		case FATTYP: return FATTYP;
		case USAneu: return USA;
		default:     return STORE;
};

}


// Kann evt. zu Konflikten führen, da das Beschreiben des FiFo im Interrupt passiert.
void doOnNetRcv(TMSNetz *NET, TMSNet_State &State)
{ if ((State.Kopf.Adr==0x90) && (State.Kopf.Abs==0x0F)) PC.wrblock(NetzTyp,0x90,0x0F,0,State.Kopf.Nr,State.Kopf.Typ,0);
  if (State.Kopf.Adr==0x9D) PC.wrblock(NetzTyp,0x9D,0,0,0,0,0);
}

void doOnGetState(TSQNetz *SQ2)
{ SQ2->SelfState.schalter=SQ2_Schalter;
  SQ2->SelfState.all|=0x70;
};

TMSStat doOnGetStateFAT(TMSNetz *NET, int & Lng)	// wird aufgerufen bevor der Statusblock gesendet wird
{ return PC.Status;
};

int main()
{ int OK;
  int i;
  unsigned short KmdLng;
  TBlock Kmd;
  unsigned char KmdBuff[10];
	unsigned long TO;
	volatile int Typ;

  SysInit();
  while (CheckFlashCRC(0));
  idle();
	initfifo(TxCharBuff,sizeof(TxCharBuff));
  setreceive(&caInfo,0x00,0,sizeof(InfoBuff),&InfoBuff);
  setreceive(&caInfo1,0x00,0,sizeof(InfoBuff1),&InfoBuff1);
  MS.OnNetRcv=doOnNetRcv;       // ermöglicht die Inbetriebnahme
  MSUSA.OnNetRcv=doOnNetRcv;
  MSUSAneu.OnNetRcv=doOnNetRcv;
  FAT.OnGetState=doOnGetStateFAT;
  SQ.OnGetState=doOnGetState;
  if (memcrc(&Parameter0,(int)&Parameter0.CRC-(int)&Parameter0)!=Parameter0.CRC) StoreParameter();
  SetNetzTyp(Parameter0.Typ);
  __enable_interrupt();
	setTimeStamp(&TO);
  while (true) {
		if (FKmd!=0) {
			if (!isRS485on()) {
				SetNetzTyp(FKmd);
				FKmd=0;
			};
		};
	 do OK=PC.KMD.scann(); while (OK==0);
   if (OK>0) {
    if (OK==NET_KMD) {
     KmdLng=fifoch(PC.KMD.Buf,blLng);
     Kmd.Buff=(unsigned char(*)[1])&KmdBuff;
     if (KmdLng<=(sizeof(KmdBuff)-2)) PC.KMD.read(&Kmd); else PC.KMD.delblock();
     Typ=Kmd.Kopf.Nr;
		 switch (Kmd.Kopf.Typ) {
      case 'R': SetNetzTyp(Typ); break;
      case '#': SQ.Self=Kmd.Kopf.Nr; break;
      case '!': if (KmdLng>=2) PC.Status=(KmdBuff[0]<<8)|KmdBuff[1]; break;
      case 'S': switch (Kmd.Kopf.Nr) {
       case 0: SQ2_Schalter=false; break;
       case 1: SQ2_Schalter=true; break;
       case 2: IO1CLR=bit24;
       case 3: IO1SET=bit24;
      };
			case 'F':
			  FKmd=NetzTyp;
				SetNetzTyp(USA);
			  for (i=0;i<KmdLng;++i) wrfifo(TxCharBuff,KmdBuff[i]);
				MSUSA.RS485on();
				MSUSA.TxStart(rdfifo(TxCharBuff));
			break;
     };
    } else if (OK==NET_SOFORT) {
     if (TxSofort.State!=BLK_TRM) {
      TxSofort.Buff=(unsigned char(*)[1])&TxSofortBuff;
      PC.KMD.read(&TxSofort);
      switch (NetzTyp) {
       case MULTI: MS.sofort=&TxSofort; break;
       case USA: MSUSA.sofort=&TxSofort; break;
       case USAneu: MSUSAneu.sofort=&TxSofort; break;
       case SQ2: SQ.sofort=&TxSofort; break;
       case FATTYP: FAT.sofort=&TxSofort; break;
       default: TxSofort.State=BLK_OFF;
      };
			setTimeStamp(&TO);
		 } else {
			 if (dTime(&TO)>3000000) TxSofort.State=BLK_OFF;
		 };
    } else PC.KMD.delblock();
   };

   do OK=PC.RCV.scann(); while (OK==0);
   if (OK>0) {
    if ((TxcaInfo.State!=BLK_READY) && (TxcaInfo.State!=BLK_TRM) ) {
     TxcaInfo.Buff=(unsigned char(*)[1])&TxInfoBuff;
     PC.RCV.read(&TxcaInfo);
		 TxcaInfo.State=BLK_READY;
    };
   };

/*
	 if (dTime(&TO)>5000000) {
     if ((TxTest.State!=BLK_READY) && (TxTest.State!=BLK_TRM) ) {
	   	setTimeStamp(&TO);
		 	makeblock(&TxTest,0x35,0x34,1,1,'O',&CRCOK);
		 };
	 };
*/

	 switch (NetzTyp) {
    case SQ2: if (SQ.Flag) { loadN(SQ,PC); SQ.Flag=false; }; break;
    case MULTI: if (MS.Flag) { loadMulti(MS,PC); MS.Flag=false; }; break;
    case USA: if (MSUSA.Flag) { loadMulti(MSUSA,PC); MSUSA.Flag=false; }; break;
    case USAneu: if (MSUSAneu.Flag) { loadMulti(MSUSAneu,PC); MSUSAneu.Flag=false; }; break;
    case FATTYP: if (FAT.Flag) { loadMulti(FAT,PC); FAT.Flag=false; }; break;
   };
   switch (caInfo.State) {
    case BLK_OK:
     PC.write(&caInfo,BlkID(NetzTyp)); setreceive(&caInfo,0x00,0,sizeof(InfoBuff),&InfoBuff); break;
    case BLK_RCV: case BLK_WAIT: break;
    default: setreceive(&caInfo,0x00,0,sizeof(InfoBuff),&InfoBuff); break; // Empfängt sämtliche Blöcke
   };
   switch (caInfo1.State) {
    case BLK_OK: PC.write(&caInfo1,BlkID(NetzTyp)); setreceive(&caInfo1,0x00,0,sizeof(InfoBuff1),&InfoBuff1); break;
    case BLK_RCV: case BLK_WAIT: break;
    default: setreceive(&caInfo1,0x00,0,sizeof(InfoBuff1),&InfoBuff1); break; // Empfängt sämtliche Blöcke
   };
   PC.Execute();
  };
}
