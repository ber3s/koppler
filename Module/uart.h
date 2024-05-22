/*****************************************************************************************
*  UART
*****************************************************************************************/

#ifndef UART_H
#define UART_H

#include <stdint.h>

//! \brief Zustände der Datenblöcke
typedef enum {
 		BLK_OFF		= 0,			//!<
 		BLK_WAIT	= 0x51,			//!< Block ist empfangsbereit
 		BLK_RCV,					//!< Block wird gerade empfangen
 		BLK_OK, 					//!< Block wurde vollständig empfangen
 		BLK_READY,     				//!< Block ist bereit zum Senden
 		BLK_TRM, 					//!< Block wird gesendet
 		BLK_TRMOK	= -10			//!< Block wurde gesendet
} TBlkState;


//! Aufbau eines Blocks
#pragma pack(1)
typedef struct {
 		uint8_t 		Adr;	//!< Adressat
 		uint8_t 		Abs;	//!< Absender
 		uint8_t 		Lng;	//!< Länge des Datensegments
 		uint8_t 		Nr;		//!<
 		uint8_t 		Typ;	//!< Art der Datenübertragung
 		uint16_t 		Ext;
} TBlockKopf;
#pragma pack()


//typedef struct TMSBlocknode *PMSBlock;
typedef struct {
 		unsigned char 		AdrX;   //!< Adresse des Blockpuffers
 		unsigned char 		LngX;   //!< max. Datenlänge des Blockpuffers
 		TBlockKopf 			Kopf;	//!<
 		unsigned short 		CRC;	//!<
 		unsigned char (*Buff)[1];	//!< Zeiger auf den Datenpuffer
 		unsigned char 		Flag;	//!< TSemaphore FLag;
 		volatile TBlkState 	State;	//!<
 		short 				cnt;	//!<
 		unsigned char 		Msk;	//!<
} TBlock;


//! Zeiger auf eine TBlock-Struktur
typedef TBlock* PBlock;
// Zeiger auf ein Array von PBlock-Zeigern
typedef PBlock (*PChannels)[1];

/* Uart line control register bit descriptions */
#define LCR_WORDLENTH_BIT         0
#define LCR_STOPBITSEL_BIT        2
#define LCR_PARITYENBALE_BIT      3
#define LCR_PARITYSEL_BIT         4
#define LCR_BREAKCONTROL_BIT      6
#define LCR_DLAB_BIT              7

/* Uart Interrupt Identification */
#define IIR_RSL                   0x3
#define IIR_RDA                   0x2
#define IIR_CTI                   0x6
#define IIR_THRE                  0x1

/* Uart Interrupt Enable Type */
#define IER_RBR                   0x1
#define IER_THRE                  0x2
#define IER_RLS                   0x4

/* Uart Receiver Errors */
#define RC_FIFO_OVERRUN_ERR       0x1
#define RC_OVERRUN_ERR            0x2
#define RC_PARITY_ERR             0x4
#define RC_FRAMING_ERR            0x8
#define RC_BREAK_IND              0x10


//! Baudratendefinitionen
typedef enum {
  		BDMAX			= 128000,
  		BD128000		= 128000,
  		BD115200		= 115200,
  		BD57600			= 57600,
  		BD38400			= 38400,
  		BD19200			= 19200,
  		BD9600			= 9600,
  		BD4800			= 4800,
  		BD2400			= 2400,
  		BD1200			= 1200
} TBaudrate;


//! UART-Kanäle
typedef enum {
  		UART0 			= 0,
  		UART1
} TUARTChannel;



 // Word Lenth type
typedef enum {
    	WordLength5 	= 0,
    	WordLength6,
    	WordLength7,
    	WordLength8
} TWordlength;

// Parity Select type
typedef enum {
    	NoParity 		= 0,
    	OddParity,
    	EvenParity,
    	StickParity,
    	EvenLowParity
} TParity;

typedef enum {
    	OneStopbit 		= 0,
    	OneHalfStopbits,
    	TwoStopbits
} TStopbits;



//! FIFO Rx Trigger Level type
typedef enum {
    	FIFORXOFF 		= 0,
    	FIFORX1,		// 0x1
    	FIFORX4,		// 0x4
    	FIFORX8,		// 0x8
    	FIFORX15		// 0xe
} TFIFORxTriggerLevel;



typedef struct {		// DeviceControlBlock
  		TBaudrate 			BaudRate;			//!< Baud Rate
  		TWordlength 		WordLength;			//!< Frame format
  		TStopbits 			Stopbits;			//!<
  		TParity 			Parity;				//!<
  		TFIFORxTriggerLevel FIFOLevel;			//!<
  		unsigned long 		InterruptEnable ;	//!< Interrupt Type: RBR, THRE, RLS
} TDCB_UART;



int UART_Init(TUARTChannel DevNum, const TDCB_UART* pConfig);
void makeblock(PBlock channel, unsigned char Adr, unsigned char Abs, unsigned char Lng, unsigned char Nr, unsigned char Typ, void* Daten);
void setreceive(PBlock channel, unsigned short Msk, unsigned short Adr, unsigned Lng, void* Buff);


#endif // UART_H
