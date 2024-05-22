/******************************************************************************/
/* (C)horizont group gmbh - Homberger Weg 4-6 - 34497 Korbach - 05631/565-0 ***/
/******************************************************************************/
/*!
 * \file    	LPC_fifo.h
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

#ifndef LPC_FIFO_H
#define LPC_FIFO_H


void 	initfifo	(short* Buf,	short n		);
int 	fifocnt		(short* Buf					);
int 	wrfifo		(short* Buf, 	short Ch	);
short 	rdfifo		(short* Buf					);	
short 	fifoch		(short* Buf, 	int n		);
short 	getfifo		(short* Buf					);
int 	fiforest	(short* Buf					);



#endif // LPC_FIFO_H



/******************************************************************************/
/*!
 * \file LPC_fifo.h
 * \section log_lpc_fifo_h  
 * \subsection sub Änderungshistorie
 * 
 * \changelog		\b xx.xx.xxxx:	Bernd Petzold
 *							- Erstellung
 *
 ******************************************************************************/
