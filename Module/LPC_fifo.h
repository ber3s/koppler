/******************************************************************************/
/* (C)horizont group gmbh - Homberger Weg 4-6 - 34497 Korbach - 05631/565-0 ***/
/******************************************************************************/
/*!
 * \file    	LPC_fifo.h
 * \brief   	Verwaltung von Fifo-Puffern
 *
 *          	Unterst�tzt das Einf�gen und Auslesen von Elementen aus Fifo-
 *				Puffern. Zus�tzlich werden Hilfsfunktionen zum Bestimmen der
 *				schon vorhandenen und noch freien Elemente unterst�tzt. Die
 *				Puffer unterst�tzen �berl�ufe: Erreicht ein Zeiger das Ende des
 *				zugewiesenen Speicherbereichs, wird er auf den Anfang 
 *				zur�ckgesetzt. Die Speicherbereiche werden nicht verwaltet, 
 *				sondern m�ssen von der aufrufenden Funktion angelegt werden. 
 *				Es findet keine Pr�fung statt, ob die Speicherbereiche g�ltig 
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
 * Ge�ndert am:		$Date$
 * Ge�ndert von:	$Author$
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
 * \section log_lpc_fifo_h �
 * \subsection sub �nderungshistorie
 * 
 * \changelog		\b xx.xx.xxxx:	Bernd Petzold
 *							- Erstellung
 *
 ******************************************************************************/
