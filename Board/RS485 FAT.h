/*************************************************************************************
 * RS485 Netz für das Kickstartboard
 * 18.07.2006
 * Bernd Petzold
 *
 * UART1
 * Port0 Bit20 ist RS485 Senden
 ************************************************************************************/

#ifndef _RS485FAT
#define _RS485FAT

#include <RS485.h>

#define	RS485senden bit6

class TRS485Netz : public TMSNetz {
 protected:
  void RS485on() { IO0SET=RS485senden; };
  void RS485off() { IO0CLR=RS485senden; };
  void TxRS485start(char Cx) { U1THR=Cx; U1IER_bit.THREIE = true; };
  bool TxRS485empty() { return (U1LSR_bit.TEMT!=0); };
 public:
  void TxRS485(char Cx) { U1THR=Cx; };
  void TxRS485stop() { U1IER_bit.THREIE = false; };
  bool RxRS485empty() { return (U1LSR_bit.DR==0); };
  unsigned short RxRS485() { return U1RBR; };
  unsigned short geterr() { return U1LSR; };	// wird aufgerufen wenn die Serielle Schnitstelle einen Fehler entdeckt
  TRS485Netz(PNetParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,unsigned char Verb, unsigned char Anz)
   : TMSNetz(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { };
  void initports() { IO0CLR=RS485senden; IO0DIR|=RS485senden; };
};

#endif
