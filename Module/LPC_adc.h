/**********************************************************************
* Analog Digitalumsetzer
**********************************************************************/
#ifndef _LPC_ADC
#define _LPC_ADC

#define MAX_ADC_FREQ      4500000 /* Max Adc frequence */

void ADC_Init (char ch);	// Vereinbart das zugehörige Pin als Analogeingang, initialisiert das Pin und startet eine Umsetzung
void ADC_Off(char ch);		// Schaltet den ADU ab, gibt das Pin frei
void ADC_Start (char ch);	// Startet eine neue Umsetzung
int ADC_Wait();
int ADC_Read ();			// Liest den vorher umgesetzten Wert
int ADC_Measure (char );	// Startet eine Umsetzung und wartet auf das Ergebnis

#endif
