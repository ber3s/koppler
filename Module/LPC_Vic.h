#ifndef __LPC_VIC_H
#define __LPC_VIC_H

/*************************************************************************
 *
 *    Used with ICCARM and AARM.
 *
 *    (c) Copyright IAR Systems 2003
 *
 *    File name   : LPC_Vic.h
 *    Description :
 *
 *    History :
 *    1. Data: July 8th, 2004
 *       Author: Wilson Liu
 *       Description: Create the basic function
 *
 *    2. Data: August 4th, 2004
 *       Author: Shawn Zhang
 *       Description: Clean up the functions. Support nonvector interrupt at first.
 *
 *    3. Data        : Oct 11, 2004
 *       Author      : Stanimir Bonev
 *       Description : Modify some function and interface
 *
 *    $Revision: 1.1 $
 **************************************************************************/

#include <iolpc2138.h>
#include <intrinsics.h>

#define IRQ_FLAG      0x80
//#define INT_MAXNUM   32

/* Interrupt Source */
/*
#define INT_WDT		0x1
#define INT_TIMER0	0x10
#define INT_TIMER1	0x20
#define INT_UART0	0x40
#define INT_UART1	0x80
#define INT_PWM0	0x100
#define INT_I2C		0x200
#define INT_SPI0	0x400
#define INT_SPI1	0x800
#define INT_PLL		0x1000
#define INT_RTC		0x2000
#define INT_EINT0	0x4000
#define INT_EINT1	0x8000
#define INT_EINT2	0x10000
#define INT_EINT3	0x20000
#define INT_AD     	0x40000
*/

#define INT_ALL     	0xFFFFFFFF

// Interrupt protection type
typedef enum {
  UserandPrivilegedMode=0,
  PrivilegedMode
}LPC_Vic_ProtectionMode_t;

typedef enum {
  VIC_Slot0 = 0,  // high priority
  VIC_Slot1,VIC_Slot2,VIC_Slot3,VIC_Slot4,VIC_Slot5,VIC_Slot6,VIC_Slot7,VIC_Slot8,
  VIC_Slot9,VIC_Slot10,VIC_Slot11,VIC_Slot12,VIC_Slot13,VIC_Slot14,VIC_Slot15
}LPC_VicIrqSlots_t;

/* Declare API functions */
void VIC_SetProtectionMode(LPC_Vic_ProtectionMode_t ProtectionType);
LPC_Vic_ProtectionMode_t VIC_GetProtectionMode(void);

void VIC_Init(void);

void VIC_EnableInt(unsigned int IntType);
void VIC_DisableInt(unsigned int IntType);

unsigned int VIC_GetIRQStatus(void);
unsigned int VIC_GetFIQStatus(void);

void VIC_EnableNonVectoredIRQ(void(*pIRQSub)());
void VIC_DisableNonVectoredIRQ(void);

void VIC_SetVectoredIRQ(void(*pIRQSub)(), LPC_VicIrqSlots_t VicIrqSlot, unsigned int VicIntSouce);

 /*************************************************************************
 * Function Name: restore_IRQ
 * Parameters: unsigned long IFlag
 * Return: void
 * Description: Restore I flag state
 *
 *************************************************************************/
#pragma inline
__arm void restore_IRQ(unsigned long IFlag)
{
unsigned long tmp;
  tmp=__get_CPSR();
  __set_CPSR(tmp & (IFlag | ~(unsigned long)IRQ_FLAG));
}

 /*************************************************************************
 * Function Name: disable_IRQ
 * Parameters:
 * Return: unsigned long
 * Description: Disable IRQ and return previous state state of flgas I
 *
 *************************************************************************/
#pragma inline
__arm unsigned long disable_IRQ(void)
{
unsigned long tmp;
  tmp=__get_CPSR();
  __set_CPSR(tmp | IRQ_FLAG);
  return tmp & IRQ_FLAG;
}

#pragma inline
__arm void enable_IRQ(void)
{
unsigned long tmp;
  tmp=__get_CPSR();
  __set_CPSR(tmp & ~(unsigned long)IRQ_FLAG);
}

#define IRQENABLE 			/* Nur im Arm Mode verwendbar, sonst enable_IRQ()erwenden              */ \
  __asm ( "MSR	  CPSR_c, #0x1F" )  	/* Sys-Mode setzen, Interrupt ein                                      */

#define IRQSETSYS                       /* Interrupt bleibt aus  */                                               \
  __asm ( "MRS	  R0, SPSR" );       	/* SPSR Register speichern                                             */ \
  __asm ( "MRS	  R1, CPSR" );       	/* CPSR Register speichern, Interrupt ist aus                          */ \
  __asm ( "MSR	  CPSR_c, #0x9F" );  	/* Sys-Mode setzen, Interrupt aus                                      */ \
  __asm ( "STMDB  SP!,{R0,R1,LR}" )    	/* LRsys retten, weil dass die Interruptroutine nicht unbedingt tut    */

#define IRQDISABLE                                                                                                \
  __asm ( "LDMFD  SP!, {R0,R1,LR}" );	/* LRsys wiederherstellen                                               */ \
  __asm ( "MSR    CPSR_cxsf,R1" );	/* XX-Mode setzen, Interrupts aus                                       */ \
  __asm ( "MSR    SPSR_cxsf,R0" )	/* SPSR wiederherstellen */

#endif // __LPC_VIC_H
