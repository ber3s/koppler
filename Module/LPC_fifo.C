/******************************************************************************/
/* (C)horizont group gmbh - Homberger Weg 4-6 - 34497 Korbach - 05631/565-0 ***/
/******************************************************************************/
/*!
 * \file    	LPC_fifo.c
 * \brief   	Verwaltung von Fifo-Puffern
 *
 *          	Unterstützt das Einfügen und Auslesen von Elementen aus Fifo-
 *				Puffern. Zusätzlich werden Hilfsfunktionen zum Bestimmen der
 *				schon vorhandenen und noch freien Elemente unterstützt. Die
 *				Puffer unterstützen Überläufe: Erreicht ein Zeiger das Ende des
 *				zugewiesenen Speicherbereichs, wird er auf den Anfang 
 *				zurückgesetzt. Die Speicherbereiche werden nicht verwaltet, 
 *				sondern müssen von der aufrufenden Funktion angelegt werden. 
 *				Es findet keine Prüfung statt, ob die Speicherbereiche gültig 
 *				sind.
 *
 * \author  	Bernd Petzold (BPG)
 *
 * \sa
 *
 * \ingroup		
 *
 ****************************************************************************//*
 * Version:			$Rev$
 * Geändert am:		$Date$
 * Geändert von:	$Author$
 ******************************************************************************/


#include "LPC_fifo.h"


//! Speicherstelle, an der die maximale Elementanzahl hinterlegt wird
#define max 	0
//! Speicherstelle, an der der Lesezeiger hinterlegt wird
#define wr  	1
//! Speicherstelle, an der der Schreibzeiger hinterlegt wird
#define rd  	2
//! Speicherstelle, an der der eigentliche Puffer anfängt
#define buff 	3



/******************************************************************************/
/*!
 * \brief	Initialisieren eines Fifo-Puffers
 *			
 *			Füllt die ersten Elemente des übergebenen Speicherbereiches mit 
 *			der maximalen Größe sowie den Schreib- und Lesezeigern
 * 
 * \remark 	initfifo ist nicht threadfest und kann fehlschlagen wenn ein rd 
 *			oder wr erfolgt
 *
 * \todo 	Um initfifo threadfest zu machen, müsste noch ein Flag eingeführt 
 *			werden, das initfifo() verhindert während rdfifo() oder wrfifo() läuft
 *          Das kann auch außerhalb durch eine Semaphore für den FIFO erfolgen.
 *
 *
 * \param	Buf		: Speicherbereich, der als Puffer dient
 * \param	n		: Größe des Puffers in Bytes
 *
 * \return	%
 *
 ******************************************************************************/
void initfifo(short* Buf, short n) { 
  	Buf[wr] = buff; 	// Schreibzeiger
	Buf[rd] = buff; 	// Lesezeiger
	
	Buf[max] = n / sizeof(short) - buff; // Anzahl der Elemente im Puffer
}




/******************************************************************************/
/*!
 * \brief	Bestimmt die Anzahl der im Puffer vorhandenen Elemente (Füllstand)
 *			
 *			Die Anzahl der Elemente wird aus der Differenz von Schreib- und
 *			Lesezeiger errechnet. 
 * 
 *
 * \param	Buf		: Speicherbereich, der als Puffer dient
 *
 * \return	Anzahl der Elemente im Puffer
 *
 ******************************************************************************/
int fifocnt(short* Buf) { 
  	
  	// Anzahl der aktuell im FIFO vorhandenen Elemente
  	int n = Buf[wr] - Buf[rd]; 
	
	// möglichen Wraparound behandeln
	if (n < 0) {
	  	n += Buf[max];
	}
  	
	return n;
}




/******************************************************************************/
/*!
 * \brief	Bestimmt die Anzahl der freien Speicherstellen im Puffer
 *			
 *			Die Anzahl der Elemente wird aus der Differenz von Lese- und 
 *			Schreibzeiger errechnet. 
 * 
 *
 * \param	Buf		: Speicherbereich, der als Puffer dient
 *
 * \return	Anzahl der freien Speicherstellen im Puffer
 *
 ******************************************************************************/
int fiforest(short* Buf) { 
  
  	// Differenz zwischen Lese- und Schreibzeiger bestimmen
  	int n = Buf[rd] - Buf[wr] - 1; 
	
	// möglichen Wraparound behandeln
	if (n < 0) {
	  	n += Buf[max];
	}
	
  	return n;
}




/******************************************************************************/
/*!
 * \brief	Schreibt ein Zeichen in den Puffer
 *			
 *			Schreibt das übergebene Zeichen an die Stelle, auf die der 
 *			Schreibzeiger aktuell zeigt. Der Schreibzeiger zeigt anschließend 
 *			auf die nächste freie Speicherstelle.
 *			Rückgabewert wird nie abgefragt, wird also eigentlich nicht benötigt.
 * 
 *
 * \param	Buf		: Speicherbereich, der als Puffer dient
 * \param	Ch		: Zu schreibendes Zeichen
 *
 * \return	Anzahl der freien Speicherstellen im Puffer oder -1 bei Fehler
 *
 ******************************************************************************/
int wrfifo(short* Buf, short Ch) { 
  
  	// aktuellen Schreibzeiger sichern
  	int temp = Buf[wr]; 
  	
	// Schrebeibzeiger am Ende angekommen: Am Anfang wieder anfangen
	if (++temp == (Buf[max]+buff)) {
	  	temp = buff;
	}
  
	// Schreib- und Lesezeiger zeigen auf dieselbe Adresse: Fehler melden
	if (temp == Buf[rd]) {
	  	return -1;
		
	} else {
	  	// Datum schreiben und Zeiger aktualisieren
   		Buf[Buf[wr]] = Ch; 
		Buf[wr] = temp;
		
		// Wraparound behandeln (?)
		// Rückgabewert wird nicht behandelt, könnte entfallen
   		temp = Buf[rd] - temp - 1;
   		
		if (temp < 0) {
		  	temp += Buf[max];
		}
   		
		return temp;
  	};
}





/******************************************************************************/
/*!
 * \brief	Liest ein Zeichen aus dem Puffer
 *			
 *			Liest das Zeichen an der Stelle, auf die der Lesezeiger zeigt, aus
 *			dem Puffer. Der Lesezeiger wird anschließend weitergezählt und zeigt
 *			auf die nächste Speicherstelle.
 * 
 *
 * \param	Buf		: Speicherbereich, der als Puffer dient
 *
 * \return	Ausgelesenes Zeichen oder -1 bei Fehler
 *
 ******************************************************************************/
short rdfifo(short* Buf) { 
  
  	short temp;
	
  	if (Buf[rd] != Buf[wr]) {
   		// Zeichen auslesen
	  	temp = Buf[Buf[rd]];
   		
		// Puffer voll: Wraparound
		// Lesezeiger weitersetzen
		if (Buf[rd]+1 == Buf[max]+buff) {
		  	Buf[rd] = buff;
		} else { 
		  	Buf[rd] += 1;
		}
   		
		return temp;
	
	// Lese- und Schreibzeiger zeigen auf die selbe Stelle	
  	} else {
	 	return -1;
	}
}


//! liest das Zeichen an der Stelle n des Buffers ausgehend vom Anfang
/******************************************************************************/
/*!
 * \brief	Liest das Zeichen an der n-ten Stelle aus dem Puffer
 *			
 *			Das gelesene Zeichen wird nicht auf Gültigkeit geprüft
 * 
 *
 * \param	Buf		: Speicherbereich, der als Puffer dient
 * \param	n		: Nummer des Zeichens, das ausgelesen werden soll
 *
 * \return	Ausgelesenes Zeichen
 *
 ******************************************************************************/
short fifoch(short* Buf, int n) { 
  	
  	n = Buf[rd] + n; 
	if (n >= Buf[max] + buff) {
	  	n -= Buf[max]; 
	}
	
	return Buf[n]; 
}




/******************************************************************************/
/*!
 * \brief	Liest ein Zeichen aus dem Puffer
 *			
 *			Liest das Zeichen an der Stelle, auf die der Lesezeiger zeigt, aus
 *			dem Puffer. Der Lesezeiger wird anschließend NICHT weitergezählt.
 * 
 *
 * \param	Buf		: Speicherbereich, der als Puffer dient
 *
 * \return	Ausgelesenes Zeichen oder -1 bei Fehler
 *
 ******************************************************************************/
short getfifo(short* Buf) { 
	
  	if (Buf[rd] != Buf[wr]) {
	 	return Buf[Buf[rd]];
	} else { 
	  	return -1; 
	}
}



/******************************************************************************/
/*!
 * \file LPC_fifo.c
 * \section log_lpc_fifo_c  
 * \subsection sub Änderungshistorie
 * 
 * \changelog		\b xx.xx.xxxx:	Bernd Petzold
 *							- Erstellung
 *
 ******************************************************************************/
