/************************************************************************************
*
*  Koppler zwischen PC und SQ-Netzen
*  03.11.2006   Bernd Petzold
*
*************************************************************************************/

#include <stdio.h>
#include <string.h>
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

#define	RS485senden bit6

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

class TMSNetzLPC : public TMSNetz {
 protected:
  void RS485on() { IO0SET=RS485senden; };
  void RS485off() { IO0CLR=RS485senden; };
  void TxStart(char Cx) { U1IER_bit.THREIE = true; U1THR=Cx; };
  bool TxEmpty() { return (U1LSR_bit.TEMT!=0); };
 public:
  TMSNetzLPC(PNetParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,unsigned char Verb, unsigned char Anz)
   : TMSNetz(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { };
  void TxD(char Cx) { U1THR=Cx; };
  void TxStop() { U1IER_bit.THREIE = false; IO0CLR=RS485senden; };
  bool RxEmpty() { return (U1LSR_bit.DR==0); };
  unsigned short RxD() { return U1RBR; };
  unsigned short geterr() { return U1LSR; };	// wird aufgerufen wenn die Serielle Schnitstelle einen Fehler entdeckt
  void initports() { IO0CLR=RS485senden; IO0DIR|=RS485senden; };
};

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

const TDCB_UART configUART1USA = { BD19200, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };
const TDCB_UART configUART0 = { BD56700, WordLength8, OneStopbit, NoParity, FIFORX1, IER_RBR | IER_RLS };

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

TMSNetzLPC MSUSA(&MSParam19200,(PChannels)&SQRxChannels,(PChannels)&SQTxChannels,10,0xF,FUNK,1);
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

// ca. 60µs
void irq_timer0()
{ VICVectAddr=0;
  if (T0IR==0) {			// Durch Software ausgelöster Interrupt
   VICSoftIntClear=(1<<VIC_TIMER0);
   if ((IRQFLAG&(1<<VIC_TIMER0))==0) {
    IRQFLAG|=(1<<VIC_TIMER0);
    enable_IRQ();
    MSUSA.Execute();
    disable_IRQ();
    IRQFLAG&=~(1<<VIC_TIMER0);
   };
  } else {
   TIMER0_ClearInt(0x1);
  };
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


  MSUSA.initports();


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


void SetNetzTyp()
{ VIC_DisableInt(1<<VIC_UART1);
  IO1SET=bit25;
  TIMER0_SetMatch(CH0,0,0);
  UART_Init(UART1,&configUART1USA);
  VIC_SetVectoredIRQ(irq_uart1_MSUSA,VIC_Slot1,VIC_UART1);
  NetzTyp=USA;
  VIC_EnableInt(1<<VIC_UART1);
}

int main()
{
  SysInit();
  idle();
  setreceive(&caInfo,0x00,0,sizeof(InfoBuff),&InfoBuff);
  setreceive(&caInfo1,0x00,0,sizeof(InfoBuff1),&InfoBuff1);
  SetNetzTyp();
  __enable_interrupt();
  while (true) {
   if (MSUSA.Flag) { /*loadMulti(MSUSA,PC);*/ MSUSA.Flag=false; };

   switch (caInfo.State) {
    case BLK_OK:
//     PC.write(&caInfo,NetzTyp);
     setreceive(&caInfo,0x00,0,sizeof(InfoBuff),&InfoBuff); break;
    case BLK_RCV: case BLK_WAIT: break;
    default: setreceive(&caInfo,0x00,0,sizeof(InfoBuff),&InfoBuff); break; // Empfängt sämtliche Blöcke
   };
   switch (caInfo1.State) {
    case BLK_OK:
//     PC.write(&caInfo1,NetzTyp);
     setreceive(&caInfo1,0x00,0,sizeof(InfoBuff1),&InfoBuff1); break;
    case BLK_RCV: case BLK_WAIT: break;
    default: setreceive(&caInfo1,0x00,0,sizeof(InfoBuff1),&InfoBuff1); break; // Empfängt sämtliche Blöcke
   };
   PC.Execute();
  };
}
