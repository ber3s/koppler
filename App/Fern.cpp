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

#define	RS485senden bit6
#define Fehlerbit bit4
#define OKLEDbit bit5

/**************************************************************/
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
  1, 1, 2,
  (unsigned int)&__checksum_begin,
  (unsigned int)&__checksum_end,
   __VER__
};

const unsigned int checksumstart = (unsigned int)&__checksum_begin;
const unsigned int checksumend = (unsigned int)&__checksum_end;
/**************************************************************/

inline void initports() { IO0CLR=bit4 | bit5; IO0DIR|=bit4 | bit5; IO0CLR=bit4 | bit5; }

inline void SetFehler() { IO0CLR=bit4; }
inline void ResetFehler() { IO0SET=bit4; }
inline void SetOKLED() { IO0SET=bit5; }
inline void ResetOKLED() { IO0CLR=bit5; }
volatile bool MSerkannt=0;
/* Der Peripheral Clock = CPU Clock da VPDIV=1 an SYS_Init(...) uebergeben wird
   FOSC  = 14745600 Hz
   FCCLK = FOSC*4 = 58982400 Hz
   FPCLK = FCCLK  ( VPBDIV1 an SYS_Init(...) uebergeben )
   FWCLK = FPCLK/4
   TimeOut = WDTC/FWCLK [s]
   WDTC = TimeOut [s] * FWCLK
*/

/*
void WDTInit (void){
 WDTC = 147456;     // =10ms   0xFF=17µs
 WDMOD_bit.WDEN=1;
 WDMOD_bit.WDRESET=1;
 WDFEED = 0xAA;     //
 WDFEED = 0x55;
// printf("Hallo %c","Du");
}

void WDTFeed( void )
{ WDFEED = 0xAA;    // Feeding sequence
  WDFEED = 0x55;
}
*/


/*************************************************************************
 *  Gloabale Variable
 ************************************************************************/
unsigned long IRQFLAG=0;
enum { SQ2=1, MULTI, USA, EURO, FATTYP, STORE=0xFF } NetzTyp;   // 1 2 3 4 5
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
//  void RS485on() { IO0SET=RS485senden; };
  void RS485on() { IO0CLR=RS485senden; };
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

class TMSNetzLPC : public TMSNetz {
 protected:
//  void RS485on() { IO0SET=RS485senden; };
  void RS485on() { IO0CLR=RS485senden; };
  void RS485off() { IO0CLR=RS485senden; };
  void TxStart(char Cx) { U1IER_bit.THREIE = true; U1THR=Cx; };
  bool TxEmpty() { return (U1LSR_bit.TEMT!=0); };
 public:
  TMSNetzLPC(PNetParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,TVerbindung Verb, unsigned char Anz)
   : TMSNetz(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { };
  void TxD(char Cx) { U1THR=Cx; };
  void TxStop() { U1IER_bit.THREIE = false; IO0CLR=RS485senden; };
  bool RxEmpty() { return (U1LSR_bit.DR==0); };
  unsigned short RxD() { return U1RBR; };
  unsigned short geterr() { return U1LSR; };	// wird aufgerufen wenn die Serielle Schnitstelle einen Fehler entdeckt
  void initports() { IO0CLR=RS485senden; IO0DIR|=RS485senden; };
};

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
const TDCB_UART configUART1FAT = { BD38400, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
// const TDCB_UART configUART0 = { BD56700, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };

TBlock caGlobal={ 20 };
unsigned char InfoBuff[50];
TBlock caInfo;		// Typ MultiSignalBlock
unsigned char InfoBuff1[50];
TBlock caInfo1;		// Typ MultiSignalBlock
unsigned char TxInfoBuff[50];
TBlock TxcaInfo;
unsigned char TxSofortBuff[50];
TBlock TxSofort;

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
};

const TSQParam SQParam19200 = {
    30 msec,	// unsigned short TRM_TIMEOUT;	// Nach dieser Zeit muss das Senden abgeschlossen sein
  4000 usec,	// unsigned short BLKPAUSE;	// Pause zwischen zwei Blöcken
   100 msec,	// unsigned long  SLV_TIMEOUT;	// Nach dieser Zeit ohne gültigen Block, macht der Slave Timeout
    10 msec,	// unsigned int   CHWCtakt;
};

const PBlock SQRxChannels[]={ &caInfo,&caGlobal,&caInfo1,
                                 0};
const PBlock SQTxChannels[]={ &TxcaInfo,
                                 0};

// TNetz(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { };
TMSNetzLPC MSUSA(&MSParam19200,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,10,0xF,FUNK,1);
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
// Empfangen ca. 60µs pro Teilnehmer damit ist bei 56700 wahrscheinlich Schluss
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
   case IIR_THRE: MSUSA.trmnext(); break;	// continue sending data
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
 #ifdef FLASH                                     //DIR0 P0          DIR1 P1
  if ( SYS_Init(FOSC, FCCLK, VPBDIV1, USER_FLASH, 0,     0xFFFFFFFF, 0,   0xFFFFFFFF) ) return 1;
 #else
  if ( SYS_Init(FOSC, FCCLK, VPBDIV1, USER_RAM,   0,     0xFFFFFFFF, 0,   0xFFFFFFFF) ) return 1;
 #endif

// Serielle Schnittstelle 0 initialisieren damit die Pegel richtig stehen
//  UART_Init(UART0,&configUART0);
  initports();

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

/* serielle Schnittstelle 0 für PC */
//  UART_Init(UART0,&configUART0);
//  VIC_SetVectoredIRQ(irq_uart0,VIC_Slot2,VIC_UART0);
//  VIC_EnableInt(1<<VIC_UART0);

  // Portbit P1:25 für Microterminalkennzeichnung
  // MULTI -> High
  // SQ -> Low
  IO1DIR=bit25;
  IO1CLR=bit25;

  return 0;
}

void loadN(TSQNetz &SQ, TRS232 &PC)
{ /* int n;

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
*/
}

void loadMulti(TMSNetzLPC &MS, TRS232 &PC)
{ int i,n;
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
  PC.wrchar(n*sizeof(TMSStat)+6);
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
   PC.wrchar(St>>24);
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

// Kann evt. zu Konflikten führen, da das Beschreiben des FiFo im Interrupt passiert.
void doOnNetRcv(TMSNetz *NET, TMSNet_State &State)
{ if ((State.Kopf.Adr==0x90) && (State.Kopf.Abs==0x0F)) PC.wrblock(NetzTyp,0x90,0x0F,0,State.Kopf.Nr,State.Kopf.Typ,0);
  if (State.Kopf.Adr==0x9D) PC.wrblock(NetzTyp,0x9D,0,0,0,0,0);
  MSerkannt=true;
}

void doOnGetState(TSQNetz *SQ2)
{ SQ2->SelfState.schalter=SQ2_Schalter;
  SQ2->SelfState.all|=0x70;
};

TMSStat doOnGetStateFAT(TMSNetz *NET, int & lng)	// wird aufgerufen bevor der Statusblock gesendet wird
{ return PC.Status;
};

int main()
{ char Typ=0;
  unsigned long dt;
  volatile int ErrCnt;

  SysInit();
  while (CheckFlashCRC(0)) ;
  idle();
  setreceive(&caInfo,0x00,0,sizeof(InfoBuff),&InfoBuff);
  setreceive(&caInfo1,0x00,0,sizeof(InfoBuff1),&InfoBuff1);
  MS.OnNetRcv=doOnNetRcv;       // ermöglicht die Inbetriebnahme
  MSUSA.OnNetRcv=doOnNetRcv;
  FAT.OnGetState=doOnGetStateFAT;
  SQ.OnGetState=doOnGetState;
  if (memcrc(&Parameter0,(int)&Parameter0.CRC-(int)&Parameter0)!=Parameter0.CRC) StoreParameter();
//  SetNetzTyp(Parameter0.Typ);
  SetNetzTyp(SQ2);
//  WDTInit();
  __enable_interrupt();
//  setTimeStamp(&dt);  while (dTime(&dt)<(2000 msec));
  while (Typ==0) {
   if (Typ==0) {
    SetNetzTyp(MULTI); setTimeStamp(&dt);
    while ((Typ==0) && (dTime(&dt)<(2000 msec))) {
     if (MS.MasterBlkCnt>=2) Typ=MULTI;
//     WDTFeed();
    };
   };
   if (Typ==0) {
    SetNetzTyp(SQ2); setTimeStamp(&dt);
    while ((Typ==0) && (dTime(&dt)<(2000 msec))) {
     if (SQ.MasterBlkCnt>=3) Typ=SQ2;
//    WDTFeed();
    };
   };
  };

  setTimeStamp(&dt);
  while (true) {
//   WDTFeed();

   if (dTime(&dt)>(1000 msec)) {
      setTimeStamp(&dt);
      if (ErrCnt) --ErrCnt;
   }
   switch (NetzTyp) {
    case SQ2:
     switch (SQ.State.Daten.N.ActSt) {
      case mdSensor: case mdAuto: case mdMan: case mdBlink: case mdOff:
      case mdGelb: case mdLernen: case mdQuarz: case mdRot: case mdRotblink:
      case mdgelbblink:
       if (SQ.isNetOn()) {
          ErrCnt=10; ResetFehler(); SetOKLED();
       } else {
          if (ErrCnt==0) { SetFehler(); ResetOKLED(); };
       };
      break;
      default: if (ErrCnt==0) { SetFehler(); ResetOKLED(); }; break;
     };
    break;
    case MULTI:
     if ((MS.State.AND & NET_OK) && MS.isNetOn()) {
      ErrCnt=10; ResetFehler(); SetOKLED();
     } else {
      if (ErrCnt==0) { SetFehler(); ResetOKLED(); };
     };
    break;
/*  case USA: if (MSUSA.Flag) { loadMulti(MSUSA,PC); MSUSA.Flag=false; }; break;
    case FATTYP: if (FAT.Flag) { loadMulti(FAT,PC); FAT.Flag=false; }; break;
*/
   };
  };
}
