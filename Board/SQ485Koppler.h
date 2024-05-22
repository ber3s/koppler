#ifndef _SQ485Koppler
#define _SQ485Koppler

#define	RS485senden bit6

class TSQNetzLPC : public TSQNetz {
 protected:
  void RS485on() { IO0SET=RS485senden; };
  void RS485off() { IO0CLR=RS485senden; };
  void TxStart(char Cx) { U1IER_bit.THREIE = true; U1THR=Cx; };
  bool TxEmpty() { return (U1LSR_bit.TEMT!=0); };
 public:
  TSQNetzLPC(PSQParam AParam,PChannels ARxCh,PChannels ATxCh,unsigned int ATakt,unsigned char Abs,unsigned char Verb, unsigned char Anz)
   : TSQNetz(AParam,ARxCh,ATxCh,ATakt,Abs,Verb,Anz) { };
  void TxD(char Cx) { U1THR=Cx; };
  void TxStop() { U1IER_bit.THREIE = false; IO0CLR=RS485senden; };
  bool RxEmpty() { return (U1LSR_bit.DR==0); };
  unsigned short RxD() { return U1RBR; };
  void initports() { IO0CLR=RS485senden; IO0DIR|=RS485senden; };
  unsigned short geterr() { return U1LSR; };	// wird aufgerufen wenn die Serielle Schnitstelle einen Fehler entdeckt
};

#endif
