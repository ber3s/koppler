/********************************************************************/
/*  Tools            ************************************************/
/********************************************************************/

#ifndef TASK_H
#define TASK_H

/******************************************************************/
/* TASK-Verwaltung    *********************************************/
/******************************************************************/

//! mögliche Zustände eines Tasks
typedef enum { 
  				ts_idle = 0, 		//!< 
				ts_active, 			//!< Task wird ausgeführt
				ts_wait, 			//!< Task befindet sich im Wartezustand
				ts_clrfkt 			//!< 
} TTaskstates;

//! TSemaphore regelt den Zugriff auf bestimmte Programmabschnitte, die nur von je einer Task gleichzeitig verarbeitet werden dürfen
typedef volatile unsigned char TSemaphore;

//! Zeiger auf eine tasknode-Struktur
typedef struct tasknode* TTaskptr;

//! Task als Element einer verketteten Liste
typedef struct tasknode {
 	void*			stack;					//!< Zeiger auf den aktuellen Stack
 	void (*start)(void);					//!< Startadresse des Task
 	TTaskstates 	state;					//!< Taskzustand
 	unsigned char 	priority;				//!< Priorität
 	TTaskptr 		prev;					//!< Zeiger auf den vorherigen Task in der Kette
	TTaskptr		next;					//!< Zeiger auf den nächsten Task in der Kette
 	TTaskstates (* fptr)(TTaskptr fptr);	//!< Wartefunktion
 	void*			mem;  					//!< zeigt auf das untere Ende des Stack und kann für Variable genutzt werden
} TTask;

#define TASKVAR(TASK,NR) (((unsigned long *)TASK ## ->mem)+NR)

// Vereinbarung der Hauptroutine eines Tasks
// inline ist notwendig, damit der Optimierer nicht die Umschaltung zwischen __arm und __thump entfernt
#define TASK(NAME, STACKSIZE)     										\
  	typedef struct {               	 									\
  						unsigned long long mem[STACKSIZE/2];   			\
  						TTask task;                     				\
 	} T__ ## NAME;                   									\
__no_init T__ ## NAME NAME;       										\
inline void NAME ## _body();      										\
 __arm void NAME ## _start() { NAME.task.mem=&NAME; NAME ## _body(); } 	\
inline void NAME ## _body()


//

void taskinit(TTask* task,unsigned char Prio, void(*start)(void),TTaskstates (*Fptr)(TTaskptr fptr));	// Einrichten eines neuen Tasks
void taskreset(TTask* task,unsigned char Prio);				// Neustart eines Tasks
void tasklink(TTask* task,unsigned char Prio,TTaskstates state,TTaskstates (*Fptr)(TTaskptr fptr));
void taskremove(TTask* task);  // entfernt die Task aus der Linktabelle und gibt die Steuerung an eine andere Task ab
void idle();
//TTaskstates taskwaitfkt(TTaskptr Ptr);
void taskwait(unsigned long Tx, unsigned char Prio);
void settaskwait(unsigned long Tx, unsigned char Prio);
void taskwait();
bool istasklinked(TTask* task);


extern "C" { void iidle(); }		// gibt die Steuerung am Ende eines Interrupts an eine andere Task ab, nur IAR-Compiler
extern TTask *__taskptr;		// Zeiger auf die aktuelle Taskvariable
extern "C" { __arm TTaskptr __tasklink(TTask* ptr); }
extern volatile unsigned char __taskcnt;
extern const unsigned char TASK_TIME;

// Reserviert ein Gerät für die aktuelle Task
// device ist die Adresse der Semaphore
// Rückgabe 0x00: Gerät ist reserviert und kann benutzt werden
// Rückgabe 0x01: Gerät wird schon benutzt
unsigned char locksemaphore(TSemaphore* device);

//! Wartet bis das Gerät reserviert ist
void waitlocksemaphore(TSemaphore* device);
//! Gibt das Gerät wieder frei
void unlocksemaphore(TSemaphore* device);







/******************************************************************************/
/*!
 * \class		TLock
 * \brief   	Verwaltet einen Semaphoren
 *			
 *          	Ausführliche Beschreibung der Klasse
 *
 ******************************************************************************/
class TLock {
 
private:
/*----- Objekt-Attribute -----------------------------------------------------*/
  	volatile TSemaphore 	Flag;
 
public:
/*----- Konstruktoren und Destruktoren ---------------------------------------*/  
  	TLock() 	{ Flag = 0; };
	
public:
/*----- Objekt-Methoden ------------------------------------------------------*/
  	bool 	lock		(void);
  	void 	unlock		(void);
  	void 	waitlock	(void);
};

#endif // TASK_H
