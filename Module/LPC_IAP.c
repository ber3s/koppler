/******************************************************************************
*  IAP Programming routines
*  !!! Achtung die IAP-Routinen nutzen die oberen 32 Byte des RAM
*  17.03.2006
*  Bernd Petzold
*
******************************************************************************/

#include <lpc_iap.h>
#include <intrinsics.h>
#include <lpc2138_sys_cnfg.h>

#define IAP_LOCATION 0x7FFFFFF1

typedef void (*Tiap)(unsigned int [], unsigned int []);
unsigned int iap_result[3];


/********* IC Identification  *****************/
void iap_prepare(unsigned int Anf, unsigned int End) { 
  
  	unsigned int iap_command[5];
  	iap_command[0] = 50;
  	iap_command[1] = Anf;
  	iap_command[2] = End;
  
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);
}


void iap_write(unsigned int Dst, unsigned int Src, unsigned int n) { 
  
  	unsigned int iap_command[5];
  	iap_command[0] = 51;
  	iap_command[1] = Dst;
  	iap_command[2] = Src;
  	iap_command[3] = n;
  	iap_command[4] = FCCLK/1000;
  
  	__disable_interrupt();
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);
  	__enable_interrupt();
}



void iap_erase(unsigned int Anf, unsigned int End) { 
  
  	unsigned int iap_command[5];
  	iap_command[0] = 52;
  	iap_command[1] = Anf;
  	iap_command[2] = End;
  	iap_command[3] = FCCLK/1000;
  
  	__disable_interrupt();
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);
  	__enable_interrupt();
}




void iap_blankcheck(unsigned int Anf, unsigned int End) { 
  
  	unsigned int iap_command[5];
  	iap_command[0] = 53;
  	iap_command[1] = Anf;
  	iap_command[2] = End;
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);
}



void rdIC_identification() { 
  
  	unsigned int iap_command[5];
  	iap_command[0] = 54;
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);
	// IAP(iap_command,iap_result);
}



void rdBoot_identification() { 
  
  	unsigned int iap_command[5];
  	iap_command[0] = 55;
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);
}



void iap_compare(unsigned int Adr1, unsigned int Adr2, unsigned int n) { 
  
  	unsigned int iap_command[5];
  	iap_command[0] = 56;
  	iap_command[1] = Adr1;
  	iap_command[2] = Adr2;
  	iap_command[3] = n;
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);
}



void iap_mem(unsigned int Anf, unsigned int End, unsigned int Dst, unsigned int Src, unsigned int n) { 
  
  	unsigned int iap_command[5];
  	iap_command[0] = 50;          // Prepare
  	iap_command[1] = Anf;
  	iap_command[2] = End;
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);

  	iap_command[0] = 52;          // Erase
  	iap_command[1] = Anf;
  	iap_command[2] = End;
  	iap_command[3] = FCCLK/1000;
  	__disable_interrupt();
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);

  	iap_command[0] = 50;          // Prepare
  	iap_command[1] = Anf;
  	iap_command[2] = End;
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);

  	iap_command[0] = 51;          // Write
  	iap_command[1] = Dst;
  	iap_command[2] = Src;
  	iap_command[3] = n;
  	iap_command[4] = FCCLK/1000;
  	((Tiap)IAP_LOCATION)(iap_command,iap_result);
  	__enable_interrupt();
}
