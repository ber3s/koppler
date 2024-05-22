/********************************************************************/
/*  Tools            ************************************************/
/********************************************************************/

#ifndef _TIMER
#define _TIMER

#include <iolpc2138.h>

/* Timer Control register bit descriptions */
#define TCR_ENABLE_BIT  0
#define TCR_RESET_BIT   1

// The channel name which is used in matching, in fact they represent
// corresponding Match Register
#define CH_MAXNUM       4
#define CH0             0
#define CH1             1
#define CH2             2
#define CH3             3

// The channel name which is used in capturing, in fact they represent
// corresponding Capture Register
#define CPCH_MAXNUM     4
#define CPCH0           0
#define CPCH1           1
#define CPCH2           2
#define CPCH3           3

//The actions when matching
#define TimerAction_Interrupt   0x1
#define TimerAction_ResetTimer  0x2
#define TimerAction_StopTimer   0x4

//Interrupt source type
#define TIMERMR0Int     0x01
#define TIMERMR1Int     0x02
#define TIMERMR2Int     0x04
#define TIMERMR3Int     0x08
#define TIMERCR0Int     0x10
#define TIMERCR1Int     0x20
#define TIMERCR2Int     0x40
#define TIMERCR3Int     0x80

#define TIMERALLInt     0xFF

inline unsigned long SystemZeit() { return T0TC; }
inline void TIMER0_ClearInt(int IntType) { T0IR = (IntType & 0xFF); }
inline void TIMER1_ClearInt(int IntType) { T1IR = (IntType & 0xFF); }

// TIMER Chanel def
typedef enum {
  TIMER0 = 0,
  TIMER1
} LPC_TimerChanel_t;

//unsigned long SystemZeit();
//void TIMER0_ClearInt(int IntType);
//__arm void TIMER1_ClearInt(int IntType);

unsigned long dTime(unsigned long *);
unsigned long dTimeSet(unsigned long *Stamp);  // liest die Differenz und setzt eine neue Zeitmarke
void setTimeStamp(unsigned long *Stamp);
extern volatile unsigned long BETRIEBSZEIT;
void incbetriebszeit();

void TIMER1_Init( unsigned long prescale );
void TIMER1_SetMatch( unsigned int MRNum, int action, unsigned long TimeValue );
void TIMER1_AddMatch( unsigned int MRNum, unsigned long TimeValue );
void TIMER0_Init( unsigned long prescale );
void TIMER0_SetMatch( unsigned int MRNum, int action, unsigned long TimeValue );
void TIMER0_AddMatch( unsigned int MRNum, unsigned long TimeValue );
void TIMER0_Reset();
void TIMER1_Reset();
void TIMER0_Start();
void TIMER1_Start();
void TIMER0_Stop();
void TIMER1_Stop();

#endif
