/*****************************************************************/
/*  Timer-Routinen      *******************************************/
/*****************************************************************/

#include <timer.h>
#include <LPC2138_sys_cnfg.h>

/*unsigned long SystemZeit() { return T0TC; }
void TIMER0_ClearInt(int IntType) { T0IR = (IntType & 0xFF); }
void TIMER1_ClearInt(int IntType) { T1IR = (IntType & 0xFF); }
*/

// __arm inline void TIMER1_ClearInt(int IntType) { T1IR = (IntType & 0xFF); }

void TIMER0_Init( unsigned long prescale )
{   // Clear interrupts flags	
    T0IR=0xFF;
    // Disable counting
    T0TCR=0;
    // Clear timer counter
    T0TC=0;
    // PR = Prescaler - 1
    T0PR=prescale-1;
    // Clear prescaler timer counter
    T0PC=0;
    // Reset Compare modules
    T0MCR=0;
    T0MR0=0;
    T0MR1=0;
    T0MR2=0;
    T0MR3=0;
    // Reset Capture modules
    T0CCR=0;
    // Reset External Compare module
    T0EMR=0;
    // Set Match register
}

void TIMER0_SetMatch( unsigned int MRNum, int action, unsigned long TimeValue )
{
    switch (MRNum) {
     case CH0: T0MR0 = TimeValue; break;
     case CH1: T0MR1 = TimeValue; break;
     case CH2: T0MR2 = TimeValue; break;
     case CH3: T0MR3 = TimeValue; break;
     default: return;
    }
    // Clear actions
    T0MCR &= ~(7<<(MRNum*3));
    if (action & TimerAction_ResetTimer) T0MCR |= TimerAction_ResetTimer << (3*MRNum);
    if (action & TimerAction_StopTimer)  T0MCR |= TimerAction_StopTimer << (3*MRNum);
    if (action & TimerAction_Interrupt)  T0MCR |= TimerAction_Interrupt << (3*MRNum);
    // Clear External action
    T0EMR &= ~(3 << (4 + 2*MRNum));
}

void TIMER0_AddMatch( unsigned int MRNum, unsigned long TimeValue) {  
  
  	volatile unsigned long* pnt;
    switch (MRNum) {
      	case CH0: pnt = &T0MR0; break;
      	case CH1: pnt = &T0MR1; break;
      	case CH2: pnt = &T0MR2; break;
      	case CH3: pnt = &T0MR3; break;
      	default: return;
   	}
#if MAX_TIMER0==0
    *pnt += TimeValue;
#else
    TimeValue += *pnt; 
	if (TimeValue >= MAX_TIMER0) {
	  	*pnt = TimeValue - MAX_TIMER0;
	} else {
	  *pnt = TimeValue;
	}
#endif
}

void TIMER1_Init(unsigned long prescale) {   
    T1IR = 0xFF;			// Clear interrupts flags	
    T1TCR = 0;				// Disable counting
    T1TC = 0;				// Clear timer counter
    T1PR = prescale - 1;	// PR = Prescaler - 1
    T1PC = 0;				// Clear prescaler timer counter

    T1MCR = 0;				// Reset Compare modules
    T1MR0 = 0;
    T1MR1 = 0;
    T1MR2 = 0;
    T1MR3 = 0;
    T1CCR = 0;				// Reset Capture modules
    T1EMR = 0;				// Reset External Compare module
    						// Set Match register
}



void TIMER1_SetMatch(unsigned int MRNum, int action, unsigned long TimeValue) {
    
  	switch (MRNum) {
     	case CH0: T1MR0 = TimeValue; break;
     	case CH1: T1MR1 = TimeValue; break;
     	case CH2: T1MR2 = TimeValue; break;
     	case CH3: T1MR3 = TimeValue; break;
     	default: return;
    }
    
	// Clear actions
    T1MCR &= ~(7<<(MRNum*3));
    if (action & TimerAction_ResetTimer) T1MCR |= TimerAction_ResetTimer << (3*MRNum);
    if (action & TimerAction_StopTimer)  T1MCR |= TimerAction_StopTimer << (3*MRNum);
    if (action & TimerAction_Interrupt)  T1MCR |= TimerAction_Interrupt << (3*MRNum);
    // Clear External action
    T1EMR &= ~(3 << (4 + 2*MRNum));
}

void TIMER1_AddMatch( unsigned int MRNum, unsigned long TimeValue )
{  volatile unsigned long *pnt;
   unsigned long Tx,dt;
    switch (MRNum) {
     case CH0: pnt=&T1MR0; break;
     case CH1: pnt=&T1MR1; break;
     case CH2: pnt=&T1MR2; break;
     case CH3: pnt=&T1MR3; break;
     default: return;
    };
    Tx=*pnt+TimeValue-10;
    dt=Tx-T1TC; if (dt>0x7FFFFFFF) dt=(~dt)-1;
    if (dt>TimeValue) *pnt=T1TC+dt; else *pnt+=TimeValue;
}

/*************************************************************************
 * Function Name: TIMER_Reset
 * Description: When next pclk arrives, only the TC and PC will be reset.
 *              Whilst other registers remain.
 *
 *************************************************************************/
void TIMER0_Reset() { T0TCR |= 2;   }
void TIMER1_Reset() { T1TCR |= 2;   }

/*************************************************************************
 * Function Name: TIMER_Start
 * Description: Start Timer or enable the Timer. if the Timer stop now, the Timer will
 *		resume running after calling this function.
 *
 *************************************************************************/
//void TIMER0_Start() { T0TCR &= ~2; T0TCR_bit.CE = true; }
//void TIMER1_Start() { T1TCR &= ~2; T1TCR_bit.CE = true; }
void TIMER0_Start() { T0TCR_bit.CR=0; T0TCR_bit.CE = true; }
void TIMER1_Start() { T1TCR_bit.CR=0; T1TCR_bit.CE = true; }

/*************************************************************************
 * Function Name: TIMER_Stop
 * Description: Just stop Timer or disable Timer, all registers remain.
 *
 *************************************************************************/
void TIMER0_Stop() { T0TCR_bit.CE = false; }
void TIMER1_Stop() { T1TCR_bit.CE = false; }

/*************************************************************************
 * Function Name: RTC_ClearInt
 * Description: Clear Timer interrupt.
 *
 *************************************************************************/
/*
__arm void TIMER0_ClearInt(int IntType) { T0IR = (IntType & 0xFF); }
void TIMER1_ClearInt(int IntType) { T1IR = (IntType & 0xFF); }
*/



/******************* dTime Ergänzungen ***********************************
* Berechnung einer Zeitdifferenz in µs, wenn der Timer1 entsprechend
* eingestellt ist
**************************************************************************/

volatile unsigned long BETRIEBSZEIT = 0;   // Betriebszeit in 0,1s Schritten
volatile unsigned long dtBETRIEBSZEIT = 0;
volatile unsigned long cntBETRIEBSZEIT = 0;

//unsigned long SystemZeit() { return T0TC; }
//void TIMER0_ClearInt(int IntType) { T0IR = (IntType & 0xFF); }



void incbetriebszeit() { 
  	
  	unsigned long tmp;
  	
	cntBETRIEBSZEIT += dTimeSet((unsigned long*)&dtBETRIEBSZEIT);
  	tmp = cntBETRIEBSZEIT / (100 msec); 
	cntBETRIEBSZEIT -= tmp * (100 msec);
  	
	BETRIEBSZEIT += tmp;
}

/***********************************************************************
 Gibt die Zeitdifferenz zu Stamp zurück
 Stamp muss vorher mit setTimeStamp gesetzt worden sein
***********************************************************************/
unsigned long dTime(unsigned long* Stamp) { 
  	
  	unsigned long tmp;
	unsigned long dt;
  	tmp = *Stamp;
  	
	if (tmp == 0) {
	  	return 0xFFFFFFFFUL;
	}
  	
	dt = SystemZeit() - tmp;
  	
	if (dt == 0xFFFFFFFFUL) {
	  	dt = 0;					// hier kann direkt ein return eingesetzt werden
	} else if (dt > 0x7FFFFFFFUL) {
	  	dt = (~dt) + 1;			// mit signed long nicht notwendig, dann kann auch return eingesetzt werden
	}
  	
	// kann durch vorhergehende Invertierung niemals aufgerufen werden:
	// dt < 0x7F.. --> Bedingung ist false
	// dt >= 0x80.. --> dt = ~dt+1 --> dt,max = 0x80 --> Bedingung ist false
	if (dt > 0xC0000000UL) {
	  	*Stamp = 0;
	}
  	
	return dt;
}

/***********************************************************************
 Gibt die Zeitdifferenz zu Stamp zurück und setzt die Zeit in Stamp auf die aktuelle Zeit
 Stamp muss vorher mit setTimeStamp gesetzt worden sein
***********************************************************************/
unsigned long dTimeSet(unsigned long* Stamp) { 	
  	
  	unsigned long tmp;
	unsigned long dt;
	unsigned long Tx;
  	
	Tx = SystemZeit();
  	tmp = *Stamp;
  	dt = Tx - tmp;
  	
	if (dt > 0x7FFFFFFFUL) {
	  	dt = (~dt) + 1;			// wäre mit signed long nicht notwendig
	}
  	
	*Stamp = Tx;
  	return dt;
}



void setTimeStamp(unsigned long* Stamp) { 
  	
  *Stamp = SystemZeit(); 
	if (*Stamp == 0) {
	  	*Stamp = 1;
	} 

}
