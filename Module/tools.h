/*! \file tools.h */
/********************************************************************/
/*  Tools            ************************************************/
/********************************************************************/

#ifndef _TOOLS
#define _TOOLS
#include <cmath>

#include <intrinsics.h>
#include <stdint.h>

typedef uint32_t lampenbits_t;
typedef uint32_t lampenpins_t;

/*!
  Größte Ordnungszahl darf maximal 15 sein
*/
typedef enum {
  lamp_rot1=0,
  lamp_gelb1,
  lamp_gruen1,
  lamp_pfeil1,
  lamp_rot2,
  lamp_gelb2,
  lamp_gruen2,
  lamp_pfeil2,
  lamp_ackn1,
  lamp_ackn2,
  lamp_error,
  lamp_frei,
  lamp_max
} lampen_t;

#define lampenbit(NR) (1 << NR)
#define rotlampen (lampenbit(lamp_rot1) | lampenbit(lamp_rot2))
#define gelblampen (lampenbit(lamp_gelb1) | lampenbit(lamp_gelb2))
#define gruenlampen (lampenbit(lamp_gruen1) | lampenbit(lamp_gruen2))
#define pfeillampen (lampenbit(lamp_pfeil1) | lampenbit(lamp_pfeil2))
#define alllampen (rotlampen | gruenlampen | gelblampen | pfeillampen)
#define grp2_lampen (lampenbit(lamp_rot2) | lampenbit(lamp_gelb2) | lampenbit(lamp_gruen2) | lampenbit(lamp_pfeil2))


#define ERR_RAM        0x0000000000000001  ///< Speicherfehler
#define ERR_QUELNG     0x0000000000000002  ///< Quelänge ist >4
#define ERR_LAMP_OFF   0x0000000000000004  ///< Es sind Lampen ausgefallen
#define ERR_LAMP_ON    0x0000000000000008  ///< Falsche Lampen an
#define ERR_POWER_LOW  0x0000000000000010  ///< \todo ERR_POWER_LOW implementieren
#define ERR_NOTOK      0x0000000000000020  ///< Nicht alle Steuerungen senden OK
#define ERR_LINE       0x0000000000000040  ///< Nicht alle Steuerungen werden empfange
#define ERR_AKKU_LOW   0x0000000000000080  ///< Spannung < AKKU_LOW
#define ERR_AKKU_TOT   0x0000000000000100  ///< Spannung < AKKU_TOT
#define ERR_NOPRG      0x0000000000000200  ///< Kein Programm
#define ERR_ZWZT       0x0000000000000400  ///< Zwischenzeitkonflikt
#define ERR_SIGNAL     0x0000000000000800  ///< Signalfehler
#define ERR_OFF        0x0000000000001000  ///< Ampel wurde ausgeschaltet U<9V
#define ERR_KOPPL			 0x0000000000002000  ///< kein Signal cpu2 <-> cpu1
#define ERR_ZEIT       0x0000000000004000  ///< Zeitdifferenz cpu1 zu cpu2 > 1s
#define ERR_FLASH      0x0000000000008000  ///< Prüfsummenfehler im Flash
#define ERR_IMAX       0x0000000000010000  ///< Imax überschritten

#define olderr_Sample  0x8000
#define olderr_CErr    0x4000
#define olderr_NoPrg   0x2000
#define olderr_SgnErr  0x1000
#define olderr_NotOK   0x800
#define olderr_NetzErr 0x400
#define olderr_Feind   0x200
#define olderr_Qmode   0x100
#define olderr_UhrErr  0x80
#define olderr_ORot    0x40
#define olderr_OGelb   0x20
#define olderr_OGruen  0x10
#define olderr_UErr    0x8
#define olderr_LRot    0x4
#define olderr_LGelb   0x2
#define olderr_LGruen  0x1

typedef uint64_t errorbits_t;  ///< Pro Bit wird ein Fehler zugeordnet

uint16_t ConvertError(errorbits_t ERR,lampenbits_t Loff,lampenbits_t Lon);

char* ftoa(double x, int d, int p, char* Buff);
char* right(char* buff, int n);



// Tab[0] enthält die Anzahl der Werte
// Die Tabellenwerte sind die Funktionswerte mit gleichem x-Abstand im Bereich von min bis max
// x ist der x-Wert für den der Funktionswert gesucht wird
/*! \brief Interpoliert in einer Tabelle.
    @tparam X Datentyp der Tabellenwerte
    @param[in] x Eingangswert
    @param[in] Tab Pointer zur Tabelle der Funktionswerte
    @param[in] max Maximaler Eingangswert
    @param[in] min Minimaler Eingangswert
    \return Interpolierter Funktionswert
*/
template <typename X> X interpol(int x, X* Tab, int max, int min) {

  	float idx;
  	int i;
  	if (x > max) {
	  	return Tab[Tab[0]];

	} else if (x >= min) {
   		idx = 1+(((float)x-min) * (Tab[0]-1))/(max-min);
   		i = (int)floor(idx);

		if ((i+1) <= Tab[0]) {
		  	return (X)floor(Tab[i]+(Tab[i+1]-Tab[i])*(idx-i))+1;
		} else {
		  	return Tab[Tab[0]];
		}

	} else return Tab[1];
}

/*******************************************************************************//**
 *  \brief Parametervariable

 *  Der Parameterwert wird mit abgeschaltem Interrupt gelesen und geschrieben.
 **********************************************************************************/

template <class X> class TParameter {

private:
  	volatile X fparameter;

public:
  	TParameter() { fparameter = 0; }
  	X get();
  	void set(X Aparameter);
};

/// Liest den Parameter
template <class X> X TParameter<X>::get() {
  	volatile X tmp;
  	__disable_interrupt();
	tmp = fparameter;
	__enable_interrupt();
  	return tmp;
}

//! Schreibt den Parameter
template <class X> void TParameter<X>::set(X Aparameter) {
  	__disable_interrupt();
	fparameter = Aparameter;
	__enable_interrupt();
}
/*********************************************************************************/


int 			countbits		(unsigned long x);
//short 		round			(float x);
unsigned short 	swap			(unsigned short x);
unsigned short 	calc_crc		(unsigned char val, unsigned short crc);
unsigned short 	calc_crc_slow	(unsigned char val, unsigned short crc);
unsigned short 	calc_memcrc		(unsigned char*  anf, unsigned char* end, unsigned short crc);

/* andere Tabelle -> anderes Polynom */
unsigned short fast_crc16(unsigned short sum, unsigned char* p, unsigned int len);
unsigned short slow_crc16(unsigned short sum, unsigned char* p, unsigned int len);



extern const unsigned int 	checksumstart;     	//!< Startadresse für die Flash-Checksum
extern const unsigned int 	checksumend;       	//!< Endadresse für die Flash-Checksum
extern unsigned short 		__checksum;  		//!< Adresse für die Flash-Checksum
extern unsigned int const 	__checksum_begin;  	//!< Startadresse für die Flash-Checksum
extern unsigned int const 	__checksum_end;    	//!< Endadresse für die Flash-Checksum


bool CheckFlashCRC(void (*watchdog)());

typedef unsigned char (*PBuff)[1];



enum  Tbits {
 	bit0  = 0x00000001, 	bit1  = 0x00000002, 	bit2  = 0x00000004, 	bit3  = 0x00000008,
 	bit4  = 0x00000010, 	bit5  = 0x00000020, 	bit6  = 0x00000040, 	bit7  = 0x00000080,
 	bit8  = 0x00000100, 	bit9  = 0x00000200, 	bit10 = 0x00000400, 	bit11 = 0x00000800,
 	bit12 = 0x00001000, 	bit13 = 0x00002000, 	bit14 = 0x00004000, 	bit15 = 0x00008000,
 	bit16 = 0x00010000, 	bit17 = 0x00020000, 	bit18 = 0x00040000, 	bit19 = 0x00080000,
 	bit20 = 0x00100000, 	bit21 = 0x00200000, 	bit22 = 0x00400000, 	bit23 = 0x00800000,
 	bit24 = 0x01000000, 	bit25 = 0x02000000, 	bit26 = 0x04000000, 	bit27 = 0x08000000,
 	bit28 = 0x10000000, 	bit29 = 0x20000000, 	bit30 = 0x40000000, 	bit31 = 0x80000000
};


#endif
