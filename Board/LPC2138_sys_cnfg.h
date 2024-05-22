/*************************************************************************
;    Copyright (c) 2004, BU MMS China, Philps Semiconductor
;    All rights reserved.
;
;    file name : LPC2000_LIB.H
;    description: give some basic definitions which will be used in the drivers
;
;    History     :

;
;*************************************************************************/
#ifndef __LPC2138_SYS_CNFG_H
#define __LPC2138_SYS_CNFG_H


#define FOSC 	          14745600UL               /* OSC [Hz] */
#define FCCLK	          FOSC*4                   /* Core clk [Hz] */
#define PRESCALER  1*FCCLK/1000000                 /* Timer Prescaler für 1 µs */
#define usec * ((FCCLK/1000)/(PRESCALER))/1000     /* Timer Ticks per µs */
#define msec * ((FCCLK/1000)/(PRESCALER))          /* Timer Ticks per ms */
#define sec * (FCCLK/1000)/(PRESCALER) * 1000      /* Timer Ticks per s */

#endif /* __LPC2138_SYS_CNFG_H */


