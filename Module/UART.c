/*****************************************************************************
*  Routinen für die serielle Schnittstelle
*****************************************************************************/
#include <uart.h>
#include <iolpc2138.h>
#include <LPC_SysControl.h>

/*****************************************************************************
*  Einen Datenblock zum Senden fertig machen
*****************************************************************************/
void makeblock(	PBlock channel, 
			   	unsigned char Adr,
                unsigned char Abs,
                unsigned char Lng,
                unsigned char Nr,
                unsigned char Typ, 
				void* Daten)		{
 
	// nicht ausführen, wenn bereits ein sendefertiger Block vorliegt oder gerade ein Block gesendet wird
	if ((channel->State != BLK_TRM) && (channel->State != BLK_READY)) {
  		channel->cnt = -5;
  		// Übertragungen anhalten, bis der Block zusammengesetzt wurde
		channel->State = BLK_OFF;
  		
		// Übergebene Attribute übernehmen
		channel->Kopf.Adr = Adr; 
		channel->Kopf.Abs = Abs; 
		channel->Kopf.Lng = Lng; 
		channel->Kopf.Nr = Nr; 
		channel->Kopf.Typ = Typ;
  		
		channel->Buff = (unsigned char(*)[1])Daten;
  		// Block als "Bereit zum Senden" markieren
		channel->State = BLK_READY;
		
 	};
}

/*****************************************************************************
*  Einen Datenkanal auf Empfang stellen
*****************************************************************************/

void setreceive(PBlock channel, unsigned short Msk, unsigned short Adr, unsigned Lng, void* Buff) { 
  	
  	channel->State = BLK_OFF;
  	channel->Msk = Msk;
  	channel->AdrX = Adr;
  	
	if (Buff) {
	  	channel->LngX = Lng;
	} else {
	  	channel->LngX = 0;
	}
  	
	channel->Buff = (unsigned char(*)[1])Buff;
  	channel->State = BLK_WAIT;
}




/*************************************************************************
 * Function Name: UART_Init
 * Parameters: LPC_UartChanel_t DevNum
 * 			
 * Return: int
 *             	0: sucess
 *		1: fail
 * Description: Initialize Uart, configure baut rate, frame format and FIFO
 *
 *************************************************************************/
int UART_Init (TUARTChannel DevNum, const TDCB_UART* pConfig) { 
  	
  	unsigned long Divisor;
	unsigned long Frame;
	unsigned long FIFO;

	// ungültige Baudrate eingestellt: Initialisierung abbrechen
	if ((pConfig->BaudRate == 0) || (pConfig->BaudRate > BDMAX)) {
	  	return 0;
	}

  	// Berechnung des Divisors für die Baudrate (Divisor = pclk / (16*Baudrate)
  	Divisor = (SYS_GetFpclk() >> 4) / pConfig->BaudRate;

  	// frame format
  	Frame = pConfig->WordLength;
  	if ( pConfig->Stopbits == TwoStopbits ) {
	  	Frame |= (1 << LCR_STOPBITSEL_BIT);
	}
  	if ( pConfig->Parity != NoParity ) {
   		Frame |= ((1 << LCR_PARITYENBALE_BIT) | ( (pConfig->Parity-1) << LCR_PARITYSEL_BIT ));
  	};

  	// FIFO
	if ( pConfig->FIFOLevel != FIFORXOFF ) {
    	FIFO = (((pConfig->FIFOLevel-1) & 0x3)<<6) | 0x1;
	}

  if (DevNum==UART0) {
   // Set baut rate
    U0LCR_bit.DLAB = true;	// DLAB = 1
    U0DLM = Divisor >> 8;
    U0DLL = Divisor;

    // Set frame format
    U0LCR = Frame;	        // DLAB = 0

    // Set FIFO
    U0FCR = FIFO;

    // Set Interrupt Enable Register
    U0IER = pConfig->InterruptEnable & 0x5;

    // Enable TxD0 and RxD0, bit 0~3=0101
    // PINSEL0 |= 0x05;
    PINSEL0_bit.P0_0 = 0x1;
    PINSEL0_bit.P0_1 = 0x1;
  }
  else if ( DevNum == UART1 )
  {
    // Set baut rate
    U1LCR_bit.DLAB = true;      // DLAB = 1
    U1DLM = Divisor>>8;
    U1DLL = Divisor & 0xff;

    // Set frame format
    U1LCR = Frame;              // DLAB =0

    /* Set FIFO */
    U1FCR = FIFO;

    // Set Interrupt Enable Register
    U1IER = pConfig->InterruptEnable & 0x5;

    // Enable TxD0 and RxD0, bit 16~19=0101
    // PINSEL0 |= 0x50000;
    PINSEL0_bit.P0_8 = 0x1;
    PINSEL0_bit.P0_9 = 0x1;
  }
  else return 1;
  return 0;
}

