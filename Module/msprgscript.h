/************************************************************************************
 Multisignal Prgscript für Mehrphasen und KN+ Programme
 20.05.2010
 Horizont Gerätewerk GmbH
 Dr.Ing. Bernd Petzold
 ***********************************************************************************/

#ifndef _MSPRGSCRIPT
#define _MSPRGSCRIPT

//void MSprgscript(unsigned char *Prg, char *Memo);      //erzeugt den Programmtext auf Memo
int MSprgscript(unsigned char *Prg, char *Memo, int Aoffset, int Alaenge);
bool MSprgavail(unsigned char *Prg);                   //Prüft die Checksumme von Prg
char *MSUSAConvertLogEntry(char *Field, char *result);  //erzeugt die Log-Textzeile vom Logeintrag Field

#endif