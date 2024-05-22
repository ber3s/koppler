
#ifndef _RS232FAT
#define _RS232FAT

#include <uart.h>
#include <iolpc2138.h>

class TRS232FAT {
 private:
  short cnt;
  int NetTyp;
  unsigned short CRCrcv,CRCtrm;
  unsigned long RcvTimeout;     // Timeout für Blockanfang im Empfänger
  unsigned long ConnectTime;
  void del(unsigned int n);
  short getch();		// holt das nächste Zeichen zum Senden  -1 Trm ist leer
  void putch(unsigned char cx);	// Schreibt das empfangene Zeichen nach Rcv
  virtual void TxStart(unsigned char cx)=0;
  virtual bool TxEmpty()=0;
  virtual void TxD(char Cx)=0;
  virtual void TxStop()=0;
  virtual bool RxEmpty()=0;
  virtual unsigned char RxD()=0;
  virtual bool isTxOn()=0;
  void wrblockanfang(unsigned char cx);
  void wrchar(unsigned char cx);
  void wrblockende();
 protected:
  int scann();			// 1 wenn ein vollständiger Block im FIFO steht sons 0, entfernt falsche Zeichen am Anfang
  short Rcv[100*sizeof(short)];
  short Trm[20*sizeof(short)];
 public:
  TRS232FAT(int Lng);
  int BlkLng;
  void Execute();			
  void rxd();				// Empfangsinterrupt
  void txd();				// Sendeinterrupt
  virtual short clrerror()=0;
  void reset();
  void delblock();			// löscht einen Block aus dem Rcv-Puffer
  void read(PBlock Dst);		// liest einen Block aus Rcv und schreibt ihn auf Dst
  void write(unsigned char *P, unsigned char Lng);
  void writekmd(unsigned char cx);	// schreibt den Block Src in den Trm FIFO
  bool BlkOK;				// true, wenn ein Datenblock vorhanden ist
  bool connected;
  unsigned int BlkCnt;
};

#endif
