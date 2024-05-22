/******************************************************************************/
/* (C)horizont group gmbh - Homberger Weg 4-6 - 34497 Korbach - 05631/565-0 ***/
/******************************************************************************/
/*!
 * \file    	task.c
 * \brief   	Task-Routinen
 *
 *          	Ausf�hrliche Beschreibung der Datei
 *
 * \author  	Bernd Petzold (BPG)
 *
 * \sa  		task.h
 *
 ****************************************************************************//*
 * Version:			$Rev$
 * Ge�ndert am:		$Date$
 * Ge�ndert von:	$Author$
 ******************************************************************************/

/*----- Abh�ngigkeiten:  -----------------------------------------------------*/
#include <task.h>
#include <timer.h>

extern "C" { void __idle(); }
extern "C" { void __taskreset(void *); }
extern "C" { unsigned char __lock(TSemaphore* device); }
extern "C" { unsigned char __unlock(TSemaphore* device); }
extern int __mem1[1]; //wird in taskasm.s79 definiert, ist der Stack des MainTask



volatile TSemaphore 	__taskflag 	= 0;		//!< Tasksemaphore 1 = gesperrt
volatile unsigned char 	__taskcnt 	= 0;		//!< Zum Ausz�hlen der Taskumschaltungen


TTask __maintask = { 
  0,					//!< Stackpointer
  0,					//!< Startadresse
  ts_active,			//!< Status
  0,					//!< Priorit�t
  0, 0, 				//!< Vorheriger, N�chster
  0,					//!< Wartefunktion
  &__mem1,				//!< 
};		


TTask* __taskroot = &__maintask;	//! Zeiger auf den ersten Task in der Liste
TTask* __taskptr = &__maintask;		//! Zeiger auf die aktuelle Taskvariable

#pragma optimize=none          // ist n�tig wegen Compilerfehler bei Mischung thumb und arm
__arm void idle() { __idle(); }
#pragma optimize=none
__arm unsigned char locksemaphore(TSemaphore* device) { return __lock(device); }
#pragma optimize=none
__arm void unlocksemaphore(TSemaphore* device) { __unlock(device); __idle(); }
#pragma optimize=none
__arm void waitlocksemaphore(TSemaphore* flag) { while (__lock(flag) != 0) __idle(); }


bool TLock::lock() 		{ return locksemaphore(&Flag)	; }
void TLock::unlock() 	{ unlocksemaphore(&Flag)		; }
void TLock::waitlock() 	{ waitlocksemaphore(&Flag)		; }




/******************************************************************************/
/*!
 * \brief	Einf�gen eines Tasks in die Verarbeitungskette
 *			
 *			Tasks werden nach Priorit�ten sortiert und in eine verkettete Liste
 *			eingef�gt
 * 
 *
 * \param	task	: Einzuf�gender Task
 *
 * \return	
 *
 ******************************************************************************/
void tsk_insert(TTask* task) { 
  
  	// Taskzeiger auf erste Task setzen
  	TTask* ptr = __taskroot;	// Die Wartefunktionen abarbeiten und den Task neu einordnen
  
	// Zeiger ung�ltig: noch kein Task definiert
	// -> �bergebene Task als erste Task in der Liste festlegen
	if (!ptr) {
   		__taskroot = task; 
		task->prev = 0; 
		task->next = 0;
		
  	} else {
	  	// ptr wird verwendet, um die Liste durchzugehen
	  	while (ptr) {
   			// aktuelle Priorit�t ist kleiner als Priorit�t des �bergebenen Tasks
		  	// -> �bergebene Task wird vor der aktuellen einsortiert
		  	if ((ptr->priority) < (task->priority)) {
    			if (ptr->prev) {
     				ptr->prev->next = task; 
					task->prev = ptr->prev; 
					ptr->prev = task; 
					task->next = ptr;
				
    			// -> gibt es vor der aktuellen keine weitere Task, wird die �bergebene am Anfang der Liste einsortiert
				} else {
     				__taskroot = task; 
					task->prev = 0; 
					task->next = ptr; 
					ptr->prev = task;
    			};
    		
				break;
   
			// Task-Priorit�t kleiner als die aktuelle und letzte Task in der Liste erreicht
			// -> Task wird am Ende einsortiert
			} else if (!ptr->next) {
    			ptr->next = task; 
				task->prev = ptr; 
				task->next = 0;
    			break;
	   
			// Listenende noch nicht erreicht und Task-Priorit�t kleiner als die aktuelle
			// -> weiter durch die Liste wandern
			} else {
			  	ptr = ptr->next;
			}
  
		};
		
	}
		
}




/******************************************************************************/
/*!
 * \brief	Entfernen eines Tasks aus der Verarbeitungskette
 *			
 *			Der �bergebene Task wird aus der verketteten Liste entfernt
 * 
 *
 * \param	task	: Zu entfernender Task
 *
 * \return	
 *
 ******************************************************************************/
void tsk_remove(TTask* task) { 
  	
  	
  	if (task->next) {
	 
		// Task hat Nachfolger und Vorg�nger  
	  	if (task->prev) {
			task->prev->next = task->next;
		// Task hat Nachfolger, aber keinen Vorg�nger: erste Task in der Liste
		} else {
		  __taskroot = task->next;
		}
   		task->next->prev = task->prev;
	
	// Task hat Vorg�nger, aber keinen Nachfolger: letzte Task in der Liste	
  	} else if (task->prev) {
    	task->prev->next = 0;
	
	// Task hat keinen Vorg�nger und keinen Nachfolger: erste und einzige Task in der Liste	
  	} else {
	  	__taskroot = 0;
	}
  
	// Vorg�nger- und Nachfolger-Felder des entfernten Tasks auf Standardwerte setzen
	task->next = (TTask *)0x1; 
	task->prev = 0;  // damit ein Task sich selbst entfernen kann
}




/******************************************************************************/
/*!
 * \brief	Pr�ft, ob ein Task in der Liste vorhanden ist
 *			
 *			
 * 
 *
 * \param	task	: Zu pr�fender Task
 *
 * \return	
 *
 ******************************************************************************/
bool tsk_linked(TTask* task) { 
  	
  	TTask* ptr = __taskroot;
  	
	// Sicherheitsabfrage, falls task NULL ist
	if (!task) {
	  	task = __taskptr;
	}
  	
	// Liste durchgehen, bis der durchlaufende Zeiger auf die angegebene Task 
	// oder hinter das Listenende zeigt
	while (ptr) { 
	  	if (ptr == task) {
			break;
	  	} 
	  	ptr = ptr->next; 
	};
  	
	// Ist der Zeiger NULL, wurde der Task nicht in der Liste gefunden
	return (ptr != 0);
}




/******************************************************************************/
/*!
 * \brief	
 *			
 *			
 * 
 *
 * \param	ptr	: 
 *
 * \return	
 *
 ******************************************************************************/
__arm TTaskptr __tasklink(TTask* ptr) { 
  
  	TTaskstates state;
  	
	// Task entfernen und neu einordnen (Priorit�ten werden dabei sortiert)
	if (ptr->next != (TTask *)0x1) {
   		tsk_remove(ptr);	// Task entsprechend seiner Priorit�t einordnen
   		tsk_insert(ptr);
  	};

  
	// Endlosschleife
	while (1) {		// solange die Wartefunktionen ausf�hren, bis ein Task gestartet wird
   		ptr = __taskroot;      // Wartefunktionen abarbeiten
   		while (ptr) {
    		if (ptr->fptr) {
     			state = ptr->fptr(ptr);
     			if (state == ts_clrfkt) {
      				ptr->fptr = 0; 
					ptr->state = ts_active;
     			} else if (ptr->state != ts_active) {
				  	ptr->state = state;
				}
    		};
    
			ptr = ptr->next;
   		};
		
   		ptr = __taskroot;
   
		while (ptr) {           // Neuen Task starten
			// if (!ptr) ptr=__taskroot;
    		if (ptr->state == ts_active) {
     			__taskptr = ptr; 
				__taskcnt = TASK_TIME; 
				return (TTaskptr)ptr->stack;
    		};
    
			ptr = ptr->next;
   
		};
  
	};
}



//! wird nicht genutzt
bool istasklinked(TTask* task) { 
  
  	bool result;
  	while (locksemaphore(&__taskflag)); // idle();
  	
	result = tsk_linked(task);
  	unlocksemaphore(&__taskflag);
  	
	return result;
}





/******************************************************************************/
/*!
 * \brief	Task wird initialisiert
 *			
 *			Startet den Task, indem NAME_start() aufgerufen wird. Staus wird
 *			auf ts_active gesetzt. Alle Tasks erhalten die gleiche Rechenzeit.
 * 
 *
 * \param	task	: Adresse der Task, der gestartet werden soll NAME.task
 * \param	Prio	: 0: niedrigste Priorit�t, 255: h�chste Priorit�t
 * \param	start	: Startadresse des Tasks (?)
 * \param	Fptr	: Zeiger auf eine Wartefunktion
 *
 * \return	%
 *
 ******************************************************************************/
__arm void taskinit(TTask* task, unsigned char Prio, void(* start)(void), TTaskstates (* Fptr)(TTaskptr FPtr)) { 
  
  	while (locksemaphore(&__taskflag)); // idle();
  	
	task->priority = Prio;
  	task->start = start;
  	task->state = ts_active;
  	task->fptr = Fptr;
  	
	__taskreset(task);	
  	tsk_insert(task);
  	unlocksemaphore(&__taskflag);
}


/******************************************************************************/
/*!
 * \brief	Task wird neu gestartet
 *			
 *			Tasks erhalten gleiche Rechenzeit, Tasks h�herer Priorit�t 
 *			unterbrechen Tasks niedrigerer Priorit�t
 * 
 *
 * \param	task	: Adresse der Task, der gestartet werden soll NAME.task
 * \param	Prio	: 0: niedrigste Priorit�t, 255: h�chste Priorit�t
 * \param	state	: Status, mit dem der Task gestartet wird
 * \param	Fptr	: Zeiger auf eine Wartefunktion
 *
 * \return	%
 *
 ******************************************************************************/
void tasklink(TTask *task, unsigned char Prio, TTaskstates state, TTaskstates (* Fptr)(TTaskptr FPtr)) { 
  
  	if (!task) {
		task = __taskptr;
  	}
  	
	while (locksemaphore(&__taskflag)) ; //idle();
  
	task->priority = Prio;
  	task->state = state;
  	task->fptr = Fptr;
  
	// Task in die Liste einf�gen (war er schon in der Liste, wird er zun�chst entfernt)
	if (tsk_linked(task)) {
	  	tsk_remove(task);
	}
  	tsk_insert(task);
  	idle();
}




/******************************************************************************/
/*!
 * \brief	Entfernt den angegebenen Task aus der Ausf�hrungsliste
 *			
 *			Task wird entfernt, Kontrolle wird an einen anderen Task �bergeben
 * 
 *
 * \param	task	: Task, der aus der Liste entfernt werden soll
 *
 * \return	%
 *
 ******************************************************************************/
void taskremove(TTask* task) { 
  
  	if (!task) {
		task=__taskptr;
	}
  	
	while (locksemaphore(&__taskflag)) ; //idle();
  	
	if (tsk_linked(task)) {
	  	tsk_remove(task);
	}
	
  	idle();
}




/******************************************************************************/
/*!
 * \brief	Startet den angebenen Task mit anderer Priorit�t neu
 *			
 *			Task wird mit der angegebenen Priotit�t neu gestartet. Status wird
 *			ts_active. Eine vorhandene Wartefunktion wird entfernt
 * 
 *
 * \param	task	: Neu zu startender Task
 * \param	Prio	: Priorit�t, mit der der Task neu gestartet wird
 *
 * \return	%
 *
 ******************************************************************************/
void taskreset(TTask* task,unsigned char Prio) {

  	taskremove(task);
  	taskinit(task,Prio,task->start,0);
}





/******************************************************************************/
/*!
 * \brief	Warteroutine f�r alle Tasks  
 *			
 *			task.mem[0] = Z�hler
 * 			task.mem[1] = Wartezeit in �s                                         
 * 
 *
 * \param	Ptr	: Neu zu startender Task
 *
 * \return	Zustand des Tasks
 *
 ******************************************************************************/
TTaskstates taskwaitfkt(TTaskptr Ptr) { 
  	
  	if (dTime(TASKVAR(Ptr, 0)) > *TASKVAR(Ptr, 1)) {
   		*TASKVAR(Ptr, 0) += *TASKVAR(Ptr, 1);
   		return ts_clrfkt;
		
  	} else {
	  	return ts_wait;
	}
}




/******************************************************************************/
/*!
 * \brief	Bereitet die Funktion taskwait() vor
 *			
 *			Schreibt einen Zeitstempel, eine Wartezeit und eine Priorit�t auf 
 *			den Stack
 * 
 *
 * \param	Tx		: Wartezeit in �s, die auf den Stack geschrieben wird
 * \param	Prio	: Priorit�t, die auf den Stack geschrieben wird
 *
 * \return	%
 *
 ******************************************************************************/
void settaskwait(unsigned long Tx, unsigned char Prio) {
  	setTimeStamp(TASKVAR(__taskptr, 0));
  	*TASKVAR(__taskptr, 1) = Tx;
  	*TASKVAR(__taskptr, 2) = Prio;
}




/******************************************************************************/
/*!
 * \brief	Startet einen Task nach einer Wartezeit
 *			
 *			Benutzt die von settaskwait() auf dem Stack abgelegten Werte zum 
 *			Warten. Die Wartefunktion erh�ht den Zeitstempel um die Wartezeit, 
 *			sobald die Task wieder gestartet wird. Damit kann eine Task in 
 *			festen Intervallen gestartet werden, unabh�ngig von der Rechenzeit.
 * 
 *
 * \param	%
 *
 * \return	%
 *
 ******************************************************************************/
void taskwait() { 
  	tasklink(__taskptr, *TASKVAR(__taskptr, 2), ts_wait, taskwaitfkt); 
}




/******************************************************************************/
/*!
 * \brief	Startet einen Task nach einer Wartezeit
 *			
 *			Wartet Tx �s und startet die Task danach mit der Priorit�t Prio
 *			Ben�tigt die Timerfunktionen zur Bestimmung der Wartezeit. 
 *			Am Stackende werden drei unsigned long Variable benutzt:
 *			0	Zeitstempel beim Start
 *			1	Wartezeit in �s
 *			2	Priorit�t mit der die Task gestartet wird.
 * 
 *
 * \param	Tx		: Wartezeit in �s
 * \param	Prio	: Priorit�t des Tasks
 *
 * \return	%
 *
 ******************************************************************************/
void taskwait(unsigned long Tx, unsigned char Prio) { 
  	settaskwait(Tx, Prio); 
	taskwait(); 
}






/*
TTaskstates taskwaitfkt(TTaskptr Ptr)
{ if ( dTime((unsigned long *)Ptr->mem)>*(((unsigned long *)Ptr->mem)+1) ) return ts_clrfkt; else return ts_wait; }

void taskwait(unsigned long Tx, unsigned char Prio)
{ setTimeStamp((unsigned long *)__taskptr->mem); *(((unsigned long *)__taskptr->mem)+1)=Tx; tasklink(__taskptr,Prio,ts_wait,taskwaitfkt); }
*/

/*******************************************************
TTaskstates taskwaitfkt(TTaskptr Ptr)
{ if ( dTime((unsigned long *)Ptr->mem) > *(((unsigned long *)Ptr->mem)+1) ) {
   *(((unsigned long *)Ptr->mem))+=*(((unsigned long *)Ptr->mem)+1);
   return ts_clrfkt;
  } else return ts_wait;
};

void settaskwait(unsigned long Tx, unsigned char Prio)
{
  setTimeStamp((unsigned long *)__taskptr->mem);
  *(((unsigned long *)__taskptr->mem)+1)=Tx;
  *(((unsigned long *)__taskptr->mem)+2)=Prio;
}

void taskwait()
{
 tasklink(__taskptr,*(((unsigned long *)__taskptr->mem)+2),ts_wait,taskwaitfkt);
}

void taskwait(unsigned long Tx, unsigned char Prio)
{
  tasklink(__taskptr,*(((unsigned long *)__taskptr->mem)+2),ts_wait,taskwaitfkt);
  tasklink(__taskptr,*TASKVAR(__taskptr,2),ts_wait,taskwaitfkt);
}

*/


/******************************************************************************/
/*!
 * \file task.c
 * \section log_task_c �
 * \subsection sub �nderungshistorie
 * 
 * \changelog		\b xx.xx.xxxx:	Bernd Petzold
 *							- Erstellung
 *
 ******************************************************************************/
