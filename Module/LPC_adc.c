/******************************************************************************/
/* (C)horizont group gmbh - Homberger Weg 4-6 - 34497 Korbach - 05631/565-0 ***/
/******************************************************************************/
/*!
 * \file    	LPC_adc.c
 * \brief   	Routinen für den A/D-Wandler
 *
 *          	Ausführliche Beschreibung der Datei
 *
 * \author  	Bernd Petzold (BPG)
 *
 * \sa  	
 *
 ****************************************************************************//*
 * Version:			$Rev$
 * Geändert am:		$Date$
 * Geändert von:	$Author$
 ******************************************************************************/


// Abhängigkeiten
#include <LPC_adc.h>
#include <iolpc2138.h>
#include <LPC_SysControl.h>


/******************************************************************************/
/*!
 * \brief	Initialisiert den A/D-Wandler
 *			
 *			Der erste Start wird zur Messung des ersten Wertes verwendet
 * 
 *
 * \param	ch		: Nummer des A/D-Kanals (0..7) 
 *
 * \return	%
 *
 ******************************************************************************/
void ADC_Init (char ch) { 
  
  	switch (ch) {
   		case 0: PINSEL1_bit.P0_27 = 1; break;
   		case 1: PINSEL1_bit.P0_28 = 1; break;
   		case 2: PINSEL1_bit.P0_29 = 1; break;
   		case 3: PINSEL1_bit.P0_30 = 1; break;
   		case 4: PINSEL1_bit.P0_25 = 1; break;
   		case 5: PINSEL1_bit.P0_26 = 1; break;
   		case 6: PINSEL0_bit.P0_4  = 3; break;
   		case 7: PINSEL0_bit.P0_5  = 3; break;
   		default: return;
  	};
	
  // ADC-Konfiguration
  	AD0CR_bit.PDN = 1;
  	AD0CR_bit.CLKDIV = SYS_GetFpclk()/MAX_ADC_FREQ+1;
  	AD0CR_bit.BURST = 0;
  	AD0CR_bit.TEST = 0;
  	AD0CR_bit.START = 0;
	// ADC_Start(ch);
}

void ADC_Off(char ch)
{ switch (ch) {
   case 0: PINSEL1_bit.P0_27 = 0; break;
   case 1: PINSEL1_bit.P0_28 = 0; break;
   case 2: PINSEL1_bit.P0_29 = 0; break;
   case 3: PINSEL1_bit.P0_30 = 0; break;
   case 4: PINSEL1_bit.P0_25 = 0; break;
   case 5: PINSEL1_bit.P0_26 = 0; break;
   case 6: PINSEL0_bit.P0_4  = 0; break;
   case 7: PINSEL0_bit.P0_5  = 0; break;
   default: return;
  };
  if ((PINSEL1_bit.P0_27!=1)
      && (PINSEL1_bit.P0_28!=1)
      && (PINSEL1_bit.P0_29!=1)
      && (PINSEL1_bit.P0_30!=1)
      && (PINSEL1_bit.P0_25!=1)
      && (PINSEL1_bit.P0_26!=1)
      && (PINSEL0_bit.P0_4!=3)
      && (PINSEL0_bit.P0_5!=3)) AD0CR_bit.PDN = 0;
}





/******************************************************************************/
/*!
 * \brief	Startet eine A/D-Wandlung
 *			
 *			
 * 
 *
 * \param	ch		: Nummer des A/D-Kanals (0..7) 
 *
 * \return	
 *
 ******************************************************************************/
__arm void ADC_Start (char ch) { 
  	
  	AD0CR &= ~ADUStartBit;
  	AD0CR = AD0CR & 0xFFFFFF00 | ch | ADUStartBit;
}





/******************************************************************************/
/*!
 * \brief	Wartet auf das Ende der Wandlung und liefert das Ergebnis zurück
 *			
 *			
 * 
 *
 * \param	
 *
 * \return	zuvor gewandelter A/D-Wert	
 *
 ******************************************************************************/
__arm int ADC_Wait() { 
  
  	unsigned int AdcResult;
	
  	do {
	  	AdcResult = AD0DR; 
	} while ((AdcResult & 0x80000000) == 0);
  	
	AD0CR &= ~ADUStartBit;
	
  	return (AdcResult >> 6) & 0x3FF;
}




/******************************************************************************/
/*!
 * \brief	Liest den zuvor gewandelten A/D-Wert ein
 *			
 *			
 * 
 *
 * \param	ch		: Nummer des A/D-Kanals (0..7) 
 *
 * \return	zuvor gewandelter A/D-Wert oder -1 bei Fehlern	
 *
 ******************************************************************************/
__arm int ADC_Read () { 
  
  	unsigned int AdcResult = AD0DR;
	
  	AD0CR &= ~ADUStartBit;
  	
	if (AdcResult & 0x80000000) {
	  	return (AdcResult >> 6) & 0x3FF; 
	} else {
	 	return -1;
	}
	
}




/******************************************************************************/
/*!
 * \brief	Startet eine neue A/D-Wandlung und wartet auf das Ergebnis
 *			
 *			
 * 
 *
 * \param	ch		: Nummer des A/D-Kanals (0..7) 
 *
 * \return	gewandelter A/D-Wert oder -1 bei Fehlern	
 *
 ******************************************************************************/
__arm int ADC_Measure (char ch) { 
  
  	unsigned int AdcResult;
  	
	AD0CR = AD0CR & 0xFFFFFF00 | ch | ADUStartBit;
  	
	do {
		AdcResult = AD0DR; 
	} while ((AdcResult & 0x80000000) == 0);
  	
	AD0CR &= ~ADUStartBit;
  	
	return (AdcResult >> 6) & 0x3FF;
}




/******************************************************************************/
/*!
 * \file LPC_adc.c
 * \section log_lpc_adc_c  
 * \subsection sub Änderungshistorie
 * 
 * \changelog		\b xx.xx.xxxx:	Bernd Petzold
 *							- Erstellung
 *
 ******************************************************************************/
