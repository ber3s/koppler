/******************************************************************************
*  IAP Programming routines
*  !!! Achtung die IAP-Routinen nutzen die oberen 32 Byte des RAM !!!!
*
*  17.03.2006
*  Bernd Petzold
******************************************************************************/

#ifndef _LPC_IAP
#define _LPC_IAP

/******************************************************************************
* Rückgabewerte
* [0]  Ergebniscode
         0 = OK
         1 = Invalid command
	 2 = Source address is not word boundary
	 3 = Destination address is incorrect boundary
	 4 = Source address is not mapped
	 5 = Destination address is not mapped
	 6 = Count byte not permitted
	 7 = Invalid sector
	 8 = Sector not blank
	 9 = Sector not prepared for write
	10 = Compare Error
	11 = BUSY
  [1]	Offset of the first non blank
	Offset of the first mismatch
	Chip ID
	Boot Version
  [2]   Content of the first non blank
*/
extern unsigned int iap_result[3];

// Chip ID lesen
void rdIC_identification();
// Version des Bootloaders lesen
void rdBoot_identification();
// Blankcheck der Sectoren Anf bis End
void iap_blankcheck(unsigned int Anf, unsigned int End);
// Vorbereiten der Sectoren Anf bis End zum Löschen oder Schreiben
void iap_prepare(unsigned int Anf, unsigned int End);
// Kopieren von n Bytes (256/512/1024/4096) von RAM-Adresse (Src) auf die Flash-Adresse (Dst)
void iap_write(unsigned int Dst, unsigned int Src, unsigned int n);
// Löschen der Sectoren Anf bis End
void iap_erase(unsigned int Anf, unsigned int End);
// Vergleichen von n Byte auf Adr1 mit Adr2
void iap_compare(unsigned int Adr1, unsigned int Adr2, unsigned int n);
// Löschen und schreiben (Interuptvektoren möglich)
void iap_mem(unsigned int Anf, unsigned int End, unsigned int Dst, unsigned int Src, unsigned int n);

#endif

