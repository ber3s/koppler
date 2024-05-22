/***********************************************************************/
/*  This file is part of the C51 Compiler package                      */
/*  Copyright KEIL ELEKTRONIK GmbH 1993                                */
/***********************************************************************/
/*                                                                     */
/*  GETKEY.C:  This routine is the general character input of C51.     */
/*                                                                     */
/*  To translate this file use C51 with the following invocation:      */
/*                                                                     */
/*     C51 GETKEY.C  <memory model>                                    */
/*                                                                     */
/*  To link the modified GETKEY.OBJ file to your application use the   */
/*  following L51 invocation:                                          */
/*                                                                     */
/*     L51 <your object file list>, GETKEY.OBJ <controls>              */
/*                                                                     */
/***********************************************************************/
/*                                                                     */
/*  Die Eingabe benutzt einen Puffer und laeft interruptgesteuert      */
/*  ausgelegt fÅr den 80517 und 11.059 MHz                             */
/*                                                                     */
/*  putinit mu· von einem Pollinginterrupt aufgerufen werden           */
/*  damit nach einem CTS==Aus die Ausgabe weiterlÑuft.                 */
/*  Ist der Ausgabepuffer voll, wird gewartet bis Platz ist.           */
/***********************************************************************/


#pragma code large ot(6) symbols debug objectextend NOIP
#include <reg517.h>
#include <stdio.h>
#include <hardware.h>

#define S0aus ES0=0
#define S0ein ES0=1

/* wenn im Textmodus gearbeitet wird ist eine SMS max 200 Zeichen lang */
#if (defined(M20) || defined(TC35))
typedef struct {
  unsigned char mode;
  unsigned char rd,wr,lng;
  char buff[250]; } TFiFo, *PFiFo;
#else
typedef struct {
  unsigned char mode;
  unsigned int rd,wr,lng;
  char buff[500]; } TFiFo, *PFiFo;
#endif

TFiFo  xdata FiFoIn;
TFiFo  xdata FiFoOut;

bit keypressed() {
/*********************************************************************/

  if (sizeof(FiFoIn.buff)-FiFoIn.lng>10) RTS=Ein;
  return (FiFoIn.lng>0);
}

void iniIO(char baud)  /* Schnittstelle 0 RS232 */
/*********************************************************************/
{ ES0  =0;
  RTS  =Aus;
  switch (baud) {
   case  96: PCON&=~SMOD; break;
   case 192: PCON|=SMOD; break;
   default: PCON&=~SMOD; break;
  };
  S0CON=0x50;
  TMOD =0x20;
  TH1  =0xFD;                 /*  9,6 kbaud   */
  TR1  =1;                    /* TR1:  timer 1 run  */
  FiFoIn.rd=0; FiFoIn.wr=0; FiFoIn.mode=0; FiFoIn.lng=0;
  FiFoOut.rd=0; FiFoOut.wr=0; FiFoOut.mode=0; FiFoOut.lng=0;
  ES0  =1; RTS=Ein;           /* Empfangsint. ein */
/*  DTR=Ein;   */
}

#pragma DISABLE
void putinit()
/*********************************************************************/
{
  if (FiFoOut.mode==0) { if ((FiFoOut.lng>0) && (CTS==Ein)) TI0=1; };
}

void IOinterrupt() interrupt  4
/*********************************************************************/
{
  if (RI0)
   { RI0=0;
     if (FiFoIn.lng<sizeof(FiFoIn.buff))
      { FiFoIn.buff[FiFoIn.wr]=S0BUF;
        if (FiFoIn.wr<(sizeof(FiFoIn.buff)-1)) ++FiFoIn.wr;
        else FiFoIn.wr=0;
        ++FiFoIn.lng;
        if (FiFoIn.lng>(sizeof(FiFoIn.buff)-10)) RTS=Aus;
      }
     else FiFoIn.mode=1;
   };
  if (TI0)
   { TI0=0;
     if ((FiFoOut.lng>0) && (CTS==Ein))
      { FiFoOut.mode=1;
        S0BUF=FiFoOut.buff[FiFoOut.rd];
        if (FiFoOut.rd<sizeof(FiFoOut.buff)-1) ++FiFoOut.rd;
        else FiFoOut.rd=0;
        --FiFoOut.lng;
      }
     else FiFoOut.mode=0;
   };
}


#pragma DISABLE
void add(PFiFo FiFo, signed char X) {
/*********************************************************************/
  FiFo->lng+=X;
}

char _getkey ()  {
/*********************************************************************/
  char c;

  if (keypressed())
   { c=FiFoIn.buff[FiFoIn.rd];
   if (FiFoIn.rd<(sizeof(FiFoIn.buff)-1)) ++FiFoIn.rd;
   else FiFoIn.rd=0;
   add(&FiFoIn,-1);
   if (sizeof(FiFoIn.buff)-FiFoIn.lng>10) RTS=Ein;
   }
  else c=EOF;
  return (c);
}

char putchar (char c)  {
/*********************************************************************/
/*  if (FiFoOut.lng==sizeof(FiFoOut.buff)) putinit();  */

  while (FiFoOut.lng==sizeof(FiFoOut.buff)) { putinit(); };
  FiFoOut.buff[FiFoOut.wr]=c;
  if (FiFoOut.wr<(sizeof(FiFoOut.buff)-1)) ++FiFoOut.wr;
  else FiFoOut.wr=0;
  add(&FiFoOut,1);
  return c;
}