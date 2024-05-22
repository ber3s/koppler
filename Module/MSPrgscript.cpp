/************************************************************************************
 Multisignal Prgscript für Mehrphasen und KN+ Programme
 20.05.2010
 Horizont Gerätewerk GmbH
 Dr.Ing. Bernd Petzold
 ***********************************************************************************/

#include <msprgscript.h>
#include <cstring>
#include <tools.h>
#include <cstdlib>
#include <stdio.h>
#include <cmath>

#define byte unsigned char
#define word unsigned short

#define MAXGRP 14

const char SgnChars[21][5]
      = {"X#/-","-#--","-#--",".#..",".#..",
         "X#/-","../-",".#..",".B..","X#/-",
         "X#/-","XB/-","X./-","-R--","XB/-",
         "XB/-","X#/-","X#/-",
         "X#/-","X#/-","X#/-"};

/*
 LSGTypCaption: array[TLSGTyp] of string[12]
             = ('Kfz Nebenr.','Fußg.','Fußg. Dop.','Fußg. dunk.','Fußg. d. D.',
                'Kfz Hauptr.','Kfz dunkel','Räumpfeil','Blinker','ÖPNV',
                'ÖPNV Blink','Flash yell.','Lamps off','Flash red',
                'Kfz Blinker','Kfz Blink.3',
                'Turn','Controller',
                'ÖPNV Blink','ÖPNV Blink','ÖPNV Blink');

*/

typedef enum { sgnError=-1,sgnRot,sgnGelb,sgnFrei,sgnRotGelb,sgnAus,sgnblink,
          sgnRotBl,sgnRotGbBl,sgnfreiGbBl,sgnFreiRotBl,sgnFreiAus} TSgnNr;

typedef enum {hupe_horn, hupe_error, hupe_mode} THupe;

// Position der Signale im Signalstring beginnt in C mit 0
typedef enum { sgn_fehler=0, sgn_aus, sgn_ein, sgn_blink, sgn_rot, sgn_rtbl,
                     sgn_gelb, sgn_rtgb, sgn_off, sgn_qrz, sgn_yebl, sgn_grbl,
               sgn_MAX} TMSSignalBilder;
typedef enum { mssgnneben, mssgnhaupt, mssgnfuss, mssgnyellbl, mssgnyellbl3,mssgnend1,
               mssgnstandardD, mssgnstandard, mssgnturn, mssgnpedesD, mssgnpedes,
               mssgnoffD, mssgnoff, mssgnrotD, mssgnrot, mssgnflashD, mssgnflash,
               mssgnend2} TMSSignalStrings;


typedef unsigned char string13[13];
const string13 LSGTypCaptionUSA[21]
             = {"Standard","Pedestrian","Fußg. Dop.","Fußg. dunk.","Fußg. d. D.",
                "Kfz Hauptr.","Kfz dunkel","Räumpfeil","Flash","ÖPNV","ÖPNV Blink",
                "Flash yell.","Lamps off","Flash red",
                "Kfz Blink.","Kfz Blink.3",
                "Turn","Controller",
                "SRB","SRB","SRB"};

const string13 USAModeTxt[16]
     ={"[OFF]","[FLASH]","[REDFL]","[RED]"
      ,"[YELL]","[QUAR]","[AUTO]","[MAN]"
      ,"[YELFL]","[ 9]","[10]","[11]"
      ,"[13]","[14]","[sync]","[error]"};


const char LSGTypChar[22] = "KFFFFKKRBSSKKKKKK*SSS";
const char LSGTypCharUSA[22] = "CPPPPCCAFSSCCCCCC*SSS";

typedef enum {lsgNeben,  lsgFuss,      lsgFuss2K,    lsgFussOff, lsgFuss2KOff,
           lsgHaupt,  lsgDunkel,    lsgRPfeil,    lsgBlinker, lsgX,
           lsgYY,     lsgYellblink, lsgOff,       lsgRed,
           lsgYellbl, lsgYellbl3,
           lsgTurn,   lsgController,
           lsgSRBhaupt, lsgSRBvor, lsgSRBab} TLSGTyp;

typedef char string15[17];
const string15 SgnNrCaption[12]
             ={"---","Red","Yellow","Green","Red/Yellow","Off","Flash yell.",
               "Flash red","Red/Fl. yell.","Green/Fl. yell.","Green/Fl. red","Off"};

const string13 HornCaption[3] = {"Horn","Error","Mode"};


#pragma pack(1)
typedef union {
 struct {
   unsigned char Tagesbit;
   unsigned short Zeit;
 };
 unsigned char B[3];
} TUPkt;

#pragma pack()

class TPrgTab {
  protected:
		int charcount;   // Zählt die erzeugten Zeichen
    bool FAvail;
    int FPrgCnt;
    int FGrpCnt;
    char FKNLine[10]; // string;
    char Sint[50];
    virtual char *wrGruppe(char *S,int Nr,int &Um)=0;
		void zeile(char *Memo, char *S);
   public:
    volatile unsigned char *Tab;
	  int offset;      // erstes Zeichen, dass ausgegeben wird
		int laenge;      // Länge eines Blocks
    bool SgnPlan;    // wenn true werden die Signalzeitenpläne ausgegeben
//    constructor create; virtual;
    TPrgTab() { SgnPlan=false; };
//    void Assign(TPrgTab &Prg);
    virtual int Script(char *Memo)=0;
//    virtual void Compile(char *S, int &E, int &EAnf, int &EEnd,
//                      char *ErrorLine, bool Warnings, char *ImgName)=0;
//    procedure Compile(Mem: TMemo; var E,EAnf,EEnd: word;
//                   var ErrorLine: string; Warnings: boolean; ImgName: string); overload; virtual; abstract;
//    procedure CheckMatrix(var E: word); virtual; abstract;
//    virtual int DeCompile(char *Memo)=0;
//    virtual void CompErrorMsg(int Control, char * ErrorLine)=0;

//    virtual bool Avail()=0;
    virtual TLSGTyp LSGTyp( int Nr)=0;
    virtual int ZwZtMax( int R)=0;
//    virtual int PrgCnt()=0;
//    virtual int GrpCnt()=0;
//    virtual char *KNLine()=0;
    virtual char *Title()=0;
    virtual char *KopfBez(int N)=0;
    virtual int ZwZt(int R, int E)=0;
    virtual int Umlauf(int Nr)=0;
    virtual int RotGelb(int Nr)=0;
    virtual int Gelb(int Nr)=0;
    virtual int EinZt(int BlNr, int GrNr)=0;
    virtual int AusZt(int BlNr, int GrNr)=0;
    virtual char *TageszeitBlk(int Dest)=0;
    virtual int GrNr(int Nr)=0;
    virtual char *GrBez(int Nr)=0;
    virtual float Timeout();
//    virtual int Tag(int Nr)=0;
    virtual int GrpBits(int Nr)=0;
//    virtual int AnzPh()=0;
};

class TKNplusPrg : public TPrgTab {
 protected:
 public:
  int PrgArt();
  char *Title()=0;
  int ZwZt(int R, int E);
  int GrpBits(int Nr);
  int ZwZtMax(int R);
  TLSGTyp LSGTyp(int Nr);
  int Umlauf(int Nr);
  int RotGelb(int Nr);
  int Gelb(int Nr);
  int EinZt( int BlNr, int GrNr);
  int AusZt( int BlNr, int GrNr);
  int SgnTime(int BlNr, int KpfNr, int Nr);
  int MaxZt(int BlNr, int GrNr);
  int GrNr(int Nr);
  int PrNr(int BlNr);
  int PlanNr(int BlNr);
  bool DoppelRot();
  int SQSP1();
  int SQSP();
  int Luecke();
  int MaxZ();
  bool Vorrang(int Nr);
  bool Anforderung(int Nr);
  int Dimmer();
  int UPrgNr(int Nr);
  int UZeit(int Nr);
  int UTage(int Nr);
  bool Ulast(int Nr);
  int RueckRTyp();
  unsigned long RueckRZeit();
  int Versatz();
  int GSP(int Nr);
  int PrgTyp(int Nr);
  int Umschalten(int Nr);
  bool Ueberspringen(int Nr);
  int MaxRot();
  int MinRot();
  int MinGruen();
  int SignalNr(int TeilnSt,int TeilnNr,int Bits);
  char SgnCh(int GNr,int SgNr);
  int Sgn37(int Nr);
  char *GrBez(int Nr);
  int AnfVonGr(int Nr);
  int AmpproGr(int Nr);
  int AnzAmp();
  TUPkt UPkt(int Nr);
  int Version();
  char *TageszeitBlk(int Dest);
  char *KopfBez(int N);
};

class TMultiKNPrg: public TKNplusPrg  {
 public:
  char *Title();
  char *GrBez(int Nr);
  float Luecke(int Nr);
  float Timeout();
  int dimmin();
  int dimmax();
  char SgnCh(int GNr,int SgNr);
  int MaxRot();
  int MinRot();
  int AnzAmp();
  int GrnBl();
  int ErrorSgn(int GNr);
  int Fehler();
  virtual int Hupe();
};

typedef TMultiKNPrg *PMultiKNPrg;

class TMSUSA37Prg: public TMultiKNPrg {
//   private
//    Compiler: TCompiler;
 protected:
  char *wrGruppe(char *S,int Nr,int &Um);
 public:
  char SgnCh(int GNr,int SgNr);
  char *GrBez(int Nr);
  TLSGTyp LSGTyp(int Nr);
  int Hupe();
  int Script(char *Memo);
//   void Compile(char *S, int &E, int &EAnf, int &EEnd,
//                      char * ErrorLine, bool Warnings, char * ImgName);
//    procedure Compile(Mem: TMemo; var E,EAnf,EEnd: word;
//                   var ErrorLine: string; Warnings: boolean; ImgName: string); overload; virtual; abstract;
//    procedure CheckMatrix(var E: word); virtual; abstract;
//   int DeCompile(char *Memo);
//   void CompErrorMsg(int Control, char * ErrorLine);
};


//-------------------------------------------------------------------------------------------------------------
typedef unsigned char TSignalStr[12];

class TBaustelle : public TPrgTab {
   public:
    int PrgLng();
    int SgnIdx(int Grpr);
    int AnzGrp();
    virtual TLSGTyp SgnTyp(int Grp);
    TSignalStr *Signal(int Grp); // TSignalStr;

    int GrNr(int Nr);  //override;
    char *GrBez(int Nr); //override;
    int GrpBits(int Nr);
    char *Title(); //override;
    int ZwZt(int R,int E); //reintroduce;
    int Umlauf(int Nr); //override;
    int RotGelb(int Nr); //override;
    int Gelb(int Nr); //override;
    int EinZt(int BlNr,int GrNr); //override;
    int AusZt(int BlNr,int GrNr); //override;
    TLSGTyp LSGTyp(int Nr); //override;
    int ZwZtMax(int R); //override;
    char *KopfBez(int Nr); //override;
    char *TageszeitBlk(int Dest); //override;
    float Timeout();
//    TTageszeitBlk Tag(int Nr); override;
    float TimeOut();
    int AnzAmp();
    int AmpproGr(int Nr);
    float Luecke(int Nr);
    int AnzPrg();
    int Gruen(int Prg,int Grp);
    int Dehn(int Prg,int Grp);
    int Sensor(int Prg,int Grp);
    int ErrIdx(int Grp);
    int MaxDimm();
    int MinDimm();
    int MaxRot();
    int MinRot();
    bool QAllowed();
    int Gruenmin();
    bool Manuell();
    virtual int GrnBl();
    int AnzUPkte();
    TUPkt UPkt(int Nr);
    int UTage(int Nr);
    int UZeit(int Nr);
    int UPrgNr(int Nr);
    int Version();
    virtual bool OPNV();
};

class TMSUSABauPrg : public TBaustelle {
   protected:
    char *wrGruppe(char *S,int Nr,int &Um);
   public:
    char *GrBez(int Nr);
    int GrnBl();
    TSgnNr ErrorSgn(int Grp);
    TSgnNr FlashSgn(int Grp);
    TSgnNr QuarzSgn(int Grp);
    TSgnNr GlbErrSgn();
    TSgnNr GlbFlashSgn();
    THupe Hupe();
    bool OPNV();
    int Script(char *Memo);
};

class TMSUSAneuPrg : public TMSUSABauPrg {
	protected:
		uint8_t Ablauf(int Nr);
		uint8_t GSP(int Nr);
		int JmpPrg(int Prg,int Grp);
		int EinZt(int Prg,int Grp);
		int AusZt(int Prg,int Grp);
		int RueckRTyp();
		uint32_t RueckRZeit();
		int Versatz();
		char SgnCh(int GNr,int SgNr);
		char *wrGruppe(char *S,int Nr,int &Um);
		bool Anforderung(int Prg,int Grp);
		bool Vorrang(int Prg,int Grp);
    TLSGTyp SgnTyp(int Grp);
    char * SignalName(int Grp, TMSSignalBilder Sgn);
	public:
		int Script(char *Memo);
};


//-------------------------------------------------------------------------------------------------------------

typedef char Tstr12[13];

int SubUml(int Uml,int N1,int N2)
{ if (N2>N1) return (N1+Uml-N2); else return (N1-N2); }

int AddUml(int Uml,int N1,int N2)
{ if (N2+N1>=Uml) return (N1+N2-Uml); return (N1+N2); }

void ConvertName(char *s)
{ int i=0;
  while (s[i]) {
   switch (s[i]) {
    case 0xE1: s[i]='ä'; break;
    case 0xEF: s[i]='ö'; break;
    case 0xF5: s[i]='ü'; break;
    case 0xE2: s[i]='ß'; break;
   };
   ++i;
  };
}


const char MSSignalNr_Tab[12] ={ 0x44,0x22,0x11,0x66,0x00,0x20,0x40,0x64,0x31,0x51,0x00 };

TSgnNr MSSignalNr(int x)
{ int idx=0;
  x&=0x77;
  while ((MSSignalNr_Tab[idx]!=x) && (idx<sizeof(MSSignalNr_Tab))) ++idx;
  if (idx<sizeof(MSSignalNr_Tab)) return (TSgnNr)idx; else return (TSgnNr)-1;
}

char *trim(char *s)
{ int i,k,n;
  k=0; while (s[k]==' ') ++k;
  n=strlen(s)-1; while ((s[n]==' ') && (n>=0)) --n;
  if (n<0) s[0]=0;
  else {
   i=0; for (k=k;k<=n;++k,++i) s[i]=s[k];
   s[i]=0;
  };
  return s;
}

char *Mdoppel(char *S,int X,char *M)
{ int i;
  S[0]=0;
  for (i=0;i<8;++i) {
   if (1 & X) { strncat(S,&M[i*2],2); strncat(S," ",2); } else strcat(S,"-- ");
   X=X >> 1;
  };
  return S;
}

//----------------------TPrgTab-------------------------------------------

void TPrgTab::zeile(char *Memo, char *S)
{ int n;
  char xx[3];
  n=strlen(S);
	if ((charcount+n)<offset) {
	} else if (charcount>=(offset+laenge)) {
	} else {
		if (charcount<offset) S+=(offset-charcount);
		strcat(Memo,S);
		if (strlen(Memo)>laenge) Memo[laenge]=0;
	};
	charcount+=n;

	strcpy(xx,"\r\n");
	n=2;
	if ((charcount+n)<offset) {
	} else if (charcount>=(offset+laenge)) {
	} else {
		if (charcount<offset) S+=(offset-charcount);
		strcat(Memo,xx);
		if (strlen(Memo)>laenge) Memo[laenge]=0;
	};
	charcount+=n;
}


// -------------------- TSQ37Prg ---------------------------------
const unsigned char UebTab[9] = { 0,0x10,0x21,0x32,0x43,0x54,0x65,0x76,0x87 };
const char KNPlus_Title[14]="INTERSECTION+";

char *TKNplusPrg::Title()
{ return (char *)&KNPlus_Title; }

int TKNplusPrg::Version()
{ return Tab[0]; }

char *TKNplusPrg::TageszeitBlk(int Dest)
{ sprintf(Sint,"Program %d",Dest); return Sint; }

char *TKNplusPrg::KopfBez(int Nr)
{ char sx[20];
  strcpy(sx,GrBez(GrNr(Nr)));
  sprintf(Sint,"%s:%d",sx,Nr);
  return Sint;
}

char *TKNplusPrg::GrBez(int Nr)
{ sprintf(Sint,"?%1d",Nr); return Sint; }

int TKNplusPrg::AmpproGr(int Nr)
{ int n,c;
  c=0;
  for (n=1;n<=AnzAmp();++n) { if (Nr==GrNr(n)) ++c; };
  return c;
}


int TKNplusPrg::AnzAmp()
{ int i;
  i=1;
  while ((i<=17) && (Tab[0xDF+i]!=0)) ++i;
  --i;
  return i;
}

int TKNplusPrg::Sgn37(int Nr)
{ static const unsigned char SgnT[8] = {3,3,2,2,1,1,0,3};
  if ((Nr>=0) && (Nr<=7)) return SgnT[Nr]; else return 0;
}

char TKNplusPrg::SgnCh(int GNr,int SgNr)
{ int i;
  i=1;
  while ((i<=17) && (GrNr(i)!=GNr)) ++i;
  if (i<=16) return SgnChars[LSGTyp(i)][SgNr];
  else return SgnChars[lsgNeben][SgNr];
}

int TKNplusPrg::SignalNr(int TeilnSt,int TeilnNr,int Bits)
{ int N;
  N=(TeilnSt & 1);
  if ((1 << (GrNr(TeilnNr)-1)) & Bits) N=N | 2;
  switch (N) {
   case 1: return 6;
   case 2: return 2;
   case 3: return 5;
   default: return 0;
  };
}

int TKNplusPrg::RueckRTyp()
{ unsigned short P;
  P=(Tab[0xFA]<<8); P+=Tab[0xFB];
  if (P==(unsigned short)0xFFFF) { P=Tab[0xFC]; P+=(Tab[0xFD]<<8); return P; }
  else return 0;
}

unsigned long TKNplusPrg::RueckRZeit()
{ unsigned long result;
  result=Tab[0xFD];
  result+=Tab[0xFC]<<8;
  result+=Tab[0xFB]<<16;
  result+=Tab[0xFA]<<24;
  return result;
}

int TKNplusPrg::Dimmer()
{ return Tab[0xF9]; }

int TKNplusPrg::Versatz()
{ return Tab[0x2F]; }

bool TKNplusPrg::Vorrang(int Nr)
{ TUPkt U;
  U=UPkt(Nr); return ((U.B[1] & 0x40)!=0);
}

bool TKNplusPrg::Anforderung(int Nr)
{ TUPkt U;
  U=UPkt(Nr); return ((U.B[1] & 0x80)!=0);
}

int TKNplusPrg::Umschalten(int Nr)
{ int P;
  P=0; --Nr;
  if (Tab[1] & (1 << Nr)) {
   while ((Nr>=0) && (Nr<=7)) {
    if (Tab[1] & (1 << Nr)) ++P;
    --Nr;
   };
  };
  return P;
}


int TKNplusPrg::AnfVonGr(int Nr)
{ int P;
  int A;
  P=0; A=Tab[1];
  while ((A>0) && (Nr>0)) {
   if (A & 1) --Nr;
   ++P; A=A >> 1;
  };
  if (Nr==0) return P; else return 0;
}

bool TKNplusPrg::Ueberspringen(int Nr)
{ return ((Tab[2] & (1 << (Nr-1)))!=0); }

int TKNplusPrg::MaxZ()
{ return Tab[0xF0]; }

int TKNplusPrg::MaxRot()
{ return Tab[0xF8]; }

int TKNplusPrg::MinRot()
{ return Tab[0xF5]; }

int TKNplusPrg::MinGruen()
{ return Tab[0xF4]; }

bool TKNplusPrg::DoppelRot()
{ return ((Tab[0xF6] & 0x20)!=0); }

int TKNplusPrg::Luecke()
{ unsigned char x;
  x=Tab[0xF2];
  return x+(Tab[0xF3]<<8);
}

int TKNplusPrg::SQSP1()
{ return Tab[0xF1]; }

int TKNplusPrg::SQSP()
{ return Tab[0xF6]; }

int TKNplusPrg::PlanNr(int BlNr)
{ TUPkt U;
  int N;
  U=UPkt(BlNr); N=(U.Zeit >> 11) & 7;
  if (N>2) return N-2; else return 1;
}

int TKNplusPrg::PrNr(int BlNr)
{ TUPkt U;
  int N;
  U=UPkt(BlNr); N=(U.Zeit >> 11) & 7;
  if (N>2) return N-2; else return 0xA00;
}

int TKNplusPrg::SgnTime(int BlNr,int KpfNr,int Nr)
//type TAmpel = (KfzN,Fuss,Fuss2k,FussOff,Fuss2KOff,KfzH,KfzOff,RPfeil,Blinker,Ampx,Ampy);
{ int Gruppe,U,Progr;
  Gruppe=GrNr(KpfNr);
  Progr=PlanNr(BlNr); U=Umlauf(BlNr);
  switch (Nr) {
   case 0: case 3: case 4: case 7:   return 0;
   case 1: return ZwZtMax(Gruppe);
   case 2: return Gelb(Gruppe);
   case 5: return SubUml(U,AusZt(Progr,Gruppe),EinZt(Progr,Gruppe));
   case 6: return RotGelb(Gruppe);
   default: return 0;
  };
}

int TKNplusPrg::GrNr(int Nr)
{ return Tab[0xDF+Nr] >> 4; }

int TKNplusPrg::GrpBits(int Nr)
{ int i;
  int Result=0;
  Nr=GrNr(Nr);
  for (i=1;i<=AnzAmp();++i) if ((Tab[0xDF+i] >> 4)==Nr) Result|=(1 << (i-1));
  return Result;
}

TLSGTyp TKNplusPrg::LSGTyp(int Nr)
{ TLSGTyp result;
  result=(TLSGTyp)(Tab[0xDF+Nr] & 0xF);
  if ((result==lsgYY)
      && ((TLSGTyp)(Tab[0xDF+1] & 0xF)==lsgYY)
      && ((TLSGTyp)(Tab[0xDF+2] & 0xF)==lsgYY)) {
   switch (Nr) {
    case 1: case 4: case 7: case 10: case 13: result=lsgSRBhaupt; break;
    case 2: case 5: case 8: case 11: case 14: result=lsgSRBvor; break;
    case 3: case 6: case 9: case 12: case 15: result=lsgSRBab; break;
   };
  };
  return result;
}

int TKNplusPrg::Gelb(int Nr)
{ return (Tab[UebTab[Nr]] & 0xF); }

int TKNplusPrg::RotGelb(int Nr)
{ return ((Tab[UebTab[Nr]] >> 4) & 0xF); }

int TKNplusPrg::PrgTyp(int Nr)
{ short P;
  P=(Tab[0x3E]<<8); P+=Tab[0x3F];
  return ( (P >> ((Nr-1)*3))  & 7);
}

const unsigned char Tabx_Umlauf[6] = {0xE,0xE,0xF,0x1E,0x1F,0x2E};

int TKNplusPrg::Umlauf(int Nr)
{ return Tab[Tabx_Umlauf[Nr]];
}

int TKNplusPrg::GSP(int Nr)
{ static const unsigned char Tabx[6] = {3,3,4,5,6,7};
  return Tab[Tabx[Nr]];
}

const unsigned char Tabx_ZwZt[8][8]
           =  {0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
               0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
               0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
               0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
               0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,
               0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,
               0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,
               0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87} ;
int TKNplusPrg::ZwZt(int R,int E)
{
 if (R > 8) R=1; if (E > 8) E=1;
 if (E==R) return 0x80; else return Tab[Tabx_ZwZt[R-1][E-1]];
}


const unsigned char Tabx_EinZt[5][8]
           = {0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,
              0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
              0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,
              0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,
              0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7} ;
int TKNplusPrg::EinZt(int BlNr,int GrNr)
{
  if ((BlNr > 8) || (GrNr > 8)) return 0; else return Tab[Tabx_EinZt[BlNr-1][GrNr-1]];
}

const unsigned char Tabx_AusZt[5][8]
           = {0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
              0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
              0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
              0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
              0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF} ;
int TKNplusPrg::AusZt(int BlNr,int GrNr)
{
  if ((BlNr > 8) || (GrNr > 8)) return 0; else return Tab[Tabx_AusZt[BlNr-1][GrNr-1]];
}

const unsigned char Tabx_MaxZt[5][8]
           = {0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
              0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
              0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
              0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
              0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F} ;
int TKNplusPrg::MaxZt(int BlNr,int GrNr)
{
  if ((BlNr > 8) || (GrNr > 8)) return 0; else return Tab[Tabx_MaxZt[BlNr-1][GrNr-1]];
}

int TKNplusPrg::ZwZtMax(int R)
{ int E,X,Z;
  Z=-128;
  for (E=1;E<=8;++E) {
   if (E!=R) {
    X=ZwZt(R,E); if (X>Z) Z=X;
   };
  };
  return Z;
}

unsigned char const Tabx_UPkt[39]
          = {8,0xB,0x18,0x1B,0x28,0x2B,0x38,0x3B,
             0x8D,0x7D,0x6D,0x5D,0x4D,0x95,0xA5,0xB5,0xC5,0xD5,0x9D,0xAD,0xBD,0xCD,0xDD,
             0x8A,0x7A,0x6A,0x5A,0x4A,0x92,0xA2,0xB2,0xC2,0xD2,0x9A,0xAA,0xBA,0xCA,0xDA,0};

TUPkt TKNplusPrg::UPkt(int Nr)
{
  TUPkt *P;
  if (Nr>sizeof(Tabx_UPkt)) Nr=1;
  P=(TUPkt *)&Tab[Tabx_UPkt[Nr-1]];
  return *P;
}

bool TKNplusPrg::Ulast(int Nr)
{ TUPkt UP;
  UP=UPkt(Nr); return ((UP.Tagesbit & 0x80)==0);
}

int TKNplusPrg::UTage(int Nr)
{ TUPkt UP;
  UP=UPkt(Nr); return (UP.Tagesbit & 0x7F);
}

int TKNplusPrg::UZeit(int Nr)
{ TUPkt UP;
  UP=UPkt(Nr);
  return swap(UP.Zeit) & 0x7FF;
}

int TKNplusPrg::UPrgNr(int Nr)
{ TUPkt UP;
  UP=UPkt(Nr); return ( (swap(UP.Zeit) >> 11) & 0x7);
}

int TKNplusPrg::PrgArt()
{ return Tab[0]; }

//--------------------------------------------------------------------------

float TMultiKNPrg::Luecke(int Nr)
{ return Tab[0xE7+Nr]/10.0; }

char *TMultiKNPrg::Title()
{ int i;
  char ch;
 Sint[0]=0;
 i=1;
 while (i<=5) {
  ch=Tab[0x87+i*0x10];
  if ((ch!=0) && (ch!=0xFF)) { Sint[i-1]=ch; Sint[i]=0; } else i=100;
  ++i;
 };
 while (i<=10) {
  ch=Tab[0x8F+(i-5)*0x10];
  if ((ch!=0) && (ch!=0xFF)) { Sint[i-1]=ch; Sint[i]=0; } else i=100;
  ++i;
 };
 ConvertName(Sint);
 return Sint;
}

float TMultiKNPrg::Timeout()
{ return (Tab[0xF5]*0.03125); }

char *TMultiKNPrg::GrBez(int Nr)
{ int i,n,c,Gruppe;
  TLSGTyp LSG;
  i=1;
  while ((i<=8) && (GrNr(i)!=Nr)) ++i;
  if (i<=8) {
   c=0; Gruppe=0; LSG=LSGTyp(i);
   for (n=1;n<=i;++i) {
    if ( (LSGTypChar[LSG]==LSGTypChar[LSGTyp(n)]) && (Gruppe!=GrNr(n)) ) {
     ++c; Gruppe=GrNr(n);
    };
   };
   Sint[0]=LSGTypChar[LSG];
   sprintf(&Sint[1],"%1d",c);
  } else strcpy(Sint,"  ");
  return Sint;
}

int TMultiKNPrg::dimmin()
{ return Tab[0xF2]; }

int TMultiKNPrg::dimmax()
{ return Tab[0xF3]; }

int TMultiKNPrg::AnzAmp()
{ int i;
  i=1;
  while ((i<=8) && Tab[0xDF+i]) ++i;
  --i;
  return i;
}

char TMultiKNPrg::SgnCh(int GNr,int SgNr)
{ int i;
  i=1;
  while ((i<=8) && (GrNr(i)!=GNr)) ++i;
  if (i<=8) return SgnChars[LSGTyp(i)][SgNr]; else return SgnChars[lsgNeben][SgNr];
}

int TMultiKNPrg::MaxRot()
{ int result;
  result=Tab[0xF7] << 8;
  result+=Tab[0xF8];
  return result;
}

int TMultiKNPrg::MinRot()
{ return Tab[0xF9]; }

int TMultiKNPrg::GrnBl()
{ if (Tab[0]<0xAF) return 0; else return (Tab[0xF0] & 7); }

int TMultiKNPrg::ErrorSgn(int GNr)
{ return Tab[0x7F+GNr]; }

int TMultiKNPrg::Fehler()
{ return (Tab[0x87]); }

int TMultiKNPrg::Hupe()
{ return 0; }

//--------------------------------------------------------------------------
TLSGTyp TMSUSA37Prg::LSGTyp(int Nr)
{ switch (TMultiKNPrg::LSGTyp(Nr)) {
   case 0: return lsgNeben;
   case 1: return lsgTurn;
   case 2: return lsgFuss;
   case 3: return lsgOff;
   case 4: return lsgRed;
   case 5: return lsgYellblink;
   case 6: return lsgBlinker;
   case 7: return lsgController;
  default: return lsgNeben;
 };
}


int TMSUSA37Prg::Hupe()
{ switch (Tab[0xF0] & 0x18) {
   case 0: return 1;
   case 8: return 2;
   case 0x10: return 3;
   default: return 4;
  };
}

char TMSUSA37Prg::SgnCh(int GNr,int SgNr)
{ int i;
  i=1;
  while ((i<=17) && (GrNr(i)!=GNr)) ++i;
  if (i<=16) return SgnChars[LSGTyp(i)][SgNr]; else return SgnChars[lsgNeben][SgNr];
}

char *TMSUSA37Prg::GrBez(int Nr)
{ int i,n,c,Gruppe;
  TLSGTyp LSG;
  i=1;
  while ((i<=8) && (GrNr(i)!=Nr)) ++i;
  if (i<=8) {
   c=0; Gruppe=0; LSG=LSGTyp(i);
   for (n=1;n<=i;++n) {
    if ( (LSGTypCharUSA[LSG]==LSGTypCharUSA[LSGTyp(n)]) && (Gruppe!=GrNr(n)) ) {
     ++c; Gruppe=GrNr(n);
    };
   };
   Sint[0]=LSGTypCharUSA[LSG];
   sprintf(&Sint[1],"%1d",c);
  } else strcpy(Sint,"  ");
  return Sint;
}

/************************ TKNPrg ********************************/
char const *getFlashSgn(int Sgn)
{ switch ((Sgn >> 3) & 3) {
   case 0:  return "flash yel";
   case 1:  return "flash red";
   case 2:  return "lamps off";
   default: return "???";
 };
}

char const *getErrorSgn(int Sgn)
{ switch (Sgn & 7) {
   case 0:  return "flash yel";
   case 1:  return "flash red";
   case 2:  return "red'";
   case 3:  return "lamps off";
   default: return "???";
  };
}

char const *getHornOutput(int H)
{ switch (H) {
   case 1: return "horn";
   case 2: return "error";
   case 3: return "mode";
   default: return "???";
  };
}

char  *PrgStr(char *S,int Nr)
{ switch (Nr) {
   case 0x1C:  strcpy(S,"Flash yel "); break;
   case 0x1D:  strcpy(S,"Flash red "); break;
   case 0x1E:  strcpy(S,"Flash     "); break;
   case 0x1F:  strcpy(S,"Lamps off "); break;
   default: sprintf(S,"Program%3d",Nr); break;
  };
  return S;
}

char *SecDatum(char *S,int T)
{ static const unsigned char MonTab[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
  static const unsigned char Schalt[] = {0,31,29,31,30,31,30,31,31,30,31,30,31};
  int Jahr,Monat,Tag,Std,Min,i,E;
  unsigned char *PT;
  T=(T >> 1) & 0x7FFFFFFF;
  T=T / 30;
  E=T/60; Min=T-E*60; T=E;
  E=T/24; Std=T-E*24; T=E;
  Jahr=-1;
  do {
   ++Jahr;
   if ((Jahr/4)*4==Jahr) T=T-366; else T=T-365;
  } while (T>=0);
  if ((Jahr/4)*4==Jahr) T=T+366; else T=T+365;
  i=1; if ((Jahr/4)*4==Jahr) PT=(unsigned char *)&Schalt; else PT=(unsigned char *)&MonTab;
  while ((T-PT[i])>=0) { T=T-PT[i]; ++i; };
  Monat=i;
  Tag=T+1;
  sprintf(S,"%02d.%02d.%02d %02d:%02d",Tag,Monat,Jahr,Std,Min);
  return S;
}

char *TMSUSA37Prg::wrGruppe(char *S,int Nr,int &Um)
{ int i,U,n;
  i=1; if (Nr==1) Um=1;
  S[0]=0;
  while ((i<=8) && (GrNr(i)!=Nr)) ++i;
  if (i<=8) {
   n=sprintf(S,"%1d %-5s%-12s%5d%4d%5s%5s",
     Nr,GrBez(Nr),LSGTypCaptionUSA[LSGTyp(i)],AmpproGr(Nr),Gelb(Nr),
     Vorrang(Nr)?"yes":"no",Anforderung(Nr)?"yes":"no");
   U=Umschalten(Nr);
   if (U>0) {
    n+=sprintf(&S[n],"%3d  yes",U);
    Um=U+1;
   } else n+=sprintf(&S[n],"%3d   no",Um);
   n+=sprintf(&S[n],"%6.1f",Luecke(Nr));
   n+=sprintf(&S[n],"%10s",getErrorSgn(ErrorSgn(Nr)));
   n+=sprintf(&S[n],"%10s ",getFlashSgn(ErrorSgn(Nr)));
   do {
    n+=sprintf(&S[n],"%2d,",i); ++i;
   } while ((GrNr(i)==Nr) || (i>8));
   S[n-1]=0;  // letztes Komma löschen
  };
  return S;
}

int TMSUSA37Prg::Script(char *Memo)
{ char *S;
  char *Sx;
  int i,k,j,U,Um,n,Ez,Az,RtGb,Gb;
  int Tx,Ux;
  int Gr;
  int Prg,Sch;
  float G,Mod;

 Memo[0]=0; charcount=0;
 if (Tab[0]<0xAB) zeile(Memo,"not available !");
 else {
  S=(char *)malloc(300); if (S==0) return charcount;
  Sx=(char *)malloc(300); if (Sx==0) return charcount;
  sprintf(S,"Title        %s",Title());    zeile(Memo,S);
  sprintf(S,"Version      %X",Version());  zeile(Memo,S);
  zeile(Memo," ");
  zeile(Memo,"OPTIONS");
  sprintf(S,"Timeout      %.1fs",Timeout()); zeile(Memo,S);
  sprintf(S,"Error signal %s",getErrorSgn(Fehler())); zeile(Memo,S);
  sprintf(S,"Flash signal %s",getFlashSgn(Fehler())); zeile(Memo,S);
  sprintf(S,"Horn output  %s",getHornOutput(Hupe())); zeile(Memo,S);
  zeile(Memo," ");

  zeile(Memo,"INTERSECTION+");
  i=0;
  do {
   ++i; wrGruppe(S,i,Um);
  } while ((strlen(S)>0) && (i<=7));
  --i;
  sprintf(S,"Number of groups%3d",i);  zeile(Memo,S);
  zeile(Memo,"Group  Type         Lgt. Yel Pri. Red   Sw.T.  Gap/s Error     Flash      Light");
  i=1;
  do {
   wrGruppe(S,i,Um); ++i;
   zeile(Memo,S);
  } while ((strlen(S)>0) && (i<=7));
  if (strlen(S)>0) zeile(Memo," ");

  zeile(Memo,"Intergreen matrix");
  strcpy(S,"   "); i=1; Gr=0; n=strlen(S);
  do {
   sprintf(Sx,"%3s ",GrBez(i));
   strcat(S,Sx); ++i; ++Gr;
  } while (strlen(trim(Sx)) && (Gr<=7));
  zeile(Memo,S);
  --Gr;

  for (i=1;i<8;++i) {
   sprintf(S,"%-3s",GrBez(i));
   for (k=1;k<8;++k) {
    if (k==i) {
     if (i>Gr) strcat(S,"    "); else strcat(S,"xxx ");
    } else if (ZwZt(i,k)==128) {
     if ((i>Gr) || (k>Gr)) strcat(S,"    "); else strcat(S," -- ");
    } else if (ZwZt(i,k)>128) {
     sprintf(Sx,"%3d ",ZwZt(i,k)-256); strcat(S,Sx);
    } else {
     sprintf(Sx,"%3d ",ZwZt(i,k)); strcat(S,Sx);
    };
   };
   trim(S);
   if (strlen(S)) zeile(Memo,S);
  };

  zeile(Memo," "); Prg=0;
  while ((Prg<5) && Umlauf(Prg+1)) ++Prg;
  sprintf(S,"Number of programs%3d",Prg); zeile(Memo,S);
  zeile(Memo," ");
  strcpy(S,"     ");
  for (i=1;i<=Prg;++i) { sprintf(Sx,"Program%2d     ",i); strcat(S,Sx); }
  zeile(Memo,S);
  strcpy(S,"Type ");
  for (i=1;i<=Prg;++i) {
   switch (PrgTyp(i)) {
    case 0: strcat(S,"fixed cycle   "); break;
    case 1: strcat(S,"ext. cycle    "); break;
    case 2: strcat(S,"Demand        "); break;
    case 3: strcat(S,"Special       "); break;
    case 4: strcat(S,"ext. fixed    "); break;
    case 5: strcat(S,"ext. ext.     "); break;
    default:strcat(S,"???           "); break;
   };
  };
  zeile(Memo,S);
  strcpy(S,"Cyc. ");
  for (i=1;i<=Prg;++i) {
   sprintf(Sx,"%3d s         ",Umlauf(i));
   strcat(S,Sx);
  };
  zeile(Memo,S); strcpy(S,"GSP  ");
  for (i=1;i<=Prg;++i) {
   sprintf(Sx,"%3d           ",GSP(i)+1);
   strcat(S,Sx);
  };
  zeile(Memo,S);
  zeile(Memo," ");
  strcpy(S,"     ");
  for (i=1;i<=Prg;++i) strcat(S,"frm to  ext.  ");
  zeile(Memo,S);

  for (k=1;k<=Gr;++k) {
   sprintf(S,"%-5s",GrBez(k));
   i=1;
   while (Umlauf(i) && (i<=5)) {
    Tx=EinZt(i,k); Ux=Umlauf(i);
    if (Tx>250) strcpy(Sx," -- ");
    else {
     Tx=AddUml(Ux,RotGelb(k),Tx)+1;
     sprintf(Sx,"%3d ",Tx);
    };
    strcat(S,Sx);
    Tx=AusZt(i,k);
    if (Tx>250) strcpy(Sx," -- ");
    else {
     Tx=SubUml(Ux,Tx,1)+1;
     sprintf(Sx,"%3d ",Tx);
    };
    strcat(S,Sx);
    Tx=MaxZt(i,k);
    if ((Tx>=0xF1) && (Tx<=0xFF)) sprintf(Sx,"%3s ",GrBez(0x100-Tx));
    else if (Tx==0xF0) strcpy(Sx,"fixed");
    else sprintf(Sx,"%3d   ",Tx);
    strcat(S,Sx);
    ++i;
   };
   zeile(Memo,S);
  };

  zeile(Memo," ");
  Sch=0;
  do { ++Sch; } while (!(Ulast(Sch) || (Sch>38)));
  sprintf(S,"Number of switch times%2d",Sch);
  zeile(Memo,S);
  for (i=1;i<=Sch;++i) {
   sprintf(S,"%2d  ",i);
   if (((AnfVonGr(i)>=1) && (AnfVonGr(i)<=7))) {
    strcpy(Sx,GrBez(AnfVonGr(i))); strcat(S,Sx);
    strcat(S," demands          ");
    strcat(S,"       ");
   } else {
    strcat(S,Mdoppel(Sx,UTage(i),"MoTuWeThFrSaSu"));
    S[strlen(S)-4]=0; strcat(S,"  ");
    k=UZeit(i);
    Mod=modf(k/60.0,&G);
    sprintf(Sx,"%02.0f:%02.0f",G,Mod); strcat(S,Sx);
   };
   strcat(S,"  ");
   strcat(S,PrgStr(Sx,UPrgNr(i)));
   zeile(Memo,S);
  };

  zeile(Memo," ");
  zeile(Memo,"PARAMETER");
  if (MaxRot()==0) strcpy(Sx,"unlimited"); else sprintf(Sx,"%ds",MaxRot());
  strcpy(S,"max. red         "); strcat(S,Sx);
  zeile(Memo,S);
//  zeile('Dimmen auf    '+conv(dimmin,1)+'%'+' (...'+conv(dimmax,1)+'%)');
  sprintf(S,"min. green       %ds",MinGruen()); zeile(Memo,S);
  sprintf(S,"min. red         %ds",MinRot());   zeile(Memo,S);
  n=GrnBl();
  if (n) {
   sprintf(S,"Green flash      %ds",n); zeile(Memo,S);
  };
  strcpy(S,"Back calculation ");
  switch (RueckRTyp()) {
   case 0xFFFF: strcat(S,"Sw  switch point"); break;
   case 0xFFFE: strcat(S,"S2  01.01.-- 00:00"); break;
   case 0xFFFD: strcat(S,"S3  01.01.80 00:00"); break;
   case 0xFFFC: strcat(S,"WI  Wait for impulse"); break;
   case 0xFFFB: strcat(S,"Sy  Synchron impulse"); break;
   default:     strcat(S,"Ti  "); strcat(S,SecDatum(Sx,RueckRZeit())); break;
  };
  zeile(Memo,S);
  strcpy(S,"Delay            ");
  if (Versatz()==0xFF) strcat(S,"GSP"); else { sprintf(Sx,"%ds",Versatz()); strcat(S,Sx); };
  zeile(Memo,S);
  zeile(Memo," ");

  if (SgnPlan) {
   for (i=1;i<=Prg;++i) {
    sprintf(S,"Program%2d",i); zeile(Memo,S); strcpy(S,"s      "); Sx[0]=0;
    n=strlen(S); j=0;
    for (k=1;k<=Umlauf(i);++k) {
     if ((k/10)*10==k) {
      ++j; if (j==10) j=0;
      sprintf(Sx,"%1d",j);
     } else if (modf(k/5.0,&G)==0) strcpy(Sx,":");
     else strcpy(Sx,".");
     strcat(S,Sx);
    };
    S[GSP(i)+n]='G';
    zeile(Memo,S);
    U=Umlauf(i);

    for (k=1;k<=Gr;++k) {
     RtGb=RotGelb(k);
     Gb=Gelb(k);
     strcpy(S,GrBez(k));
     Ez=EinZt(i,k);
     Az=AusZt(i,k);
     if ((Ez<253) && (Az<253) && (Ez!=Az)) {
      sprintf(Sx,"%4d ",SubUml(U,Az,Ez)-RtGb);
      strcat(S,Sx);
     } else strcat(S,"     ");
     if ((U>0) && (U<255) && (Ez!=Az)) {
      for (n=0;n<U;++n) Sx[n]=' '; Sx[U]=0;
      if ((Az>=0) && (Az<=U) && (Ez>=0) && (Ez<=U)) {
       j=Ez+1; if (j>U) j=1;
       for (n=1;n<=RtGb;++n) { Sx[j-1]=SgnCh(k,0); ++j; if (j>U) j=1; };
       for (n=1;n<=(SubUml(U,Az,Ez)-RtGb);++n)  { Sx[j-1]=SgnCh(k,1); ++j; if (j>U) j=1; };
       for (n=1;n<=Gb;++n) { Sx[j-1]=SgnCh(k,2); ++j; if (j>U) j=1; };
       n=Ez+1;
       if (n>U) n=1; if (j>U) j=1;
       while (j!=n) { Sx[j-1]=SgnCh(k,3); ++j; if (j>U) j=1; };
      } else if ((Ez>=0) && (Ez<U)) {
       if (RtGb>0) {
        for (n=1;n<=U;++n) Sx[n-1]='#';
        j=Ez+1; if (j>U) j=1;
        for (n=1;n<=RtGb;++n) { Sx[j-1]=SgnCh(k,0); ++j; if (j>U) j=1; };
       } else Sx[Ez]='*';
      } else if ( ((Az+1)>=0) && (Az<U)) {
       if (Gb>0) {
        for (n=1;n<=U;++n) Sx[n-1]='-';
        j=Ez+1; if (j>U) j=1;
        for (n=1;n<=Gb;++n) { Sx[j-1]=SgnCh(k,2); ++j; if (j>U) j=1; };
       } else Sx[Az]='O';
      };
     };
     strcat(S,Sx); zeile(Memo,S);
    };
    zeile(Memo," ");
   };
  };
 };
 free(Sx);
 free(S);
 return charcount;
}
/**************************************************************************************
                                 Baustelle
 *************************************************************************************/

const int MinManuellVersion = 2;  // ab dieser Version ist die Umschaltung "keine Beeinflussung der Räumzeit" möglich )
const int MinNameVersion = 2;     // ab dieser Version ist der Name im Programm vorhanden                             )
const int MinUSAVersion = 3;      // ab da ist das Auslesen von eu.usaerr möglich
const int MinGruenBlVersion = 4;  // ab da wird GrnBl angeboten und hupe
const int MinOPNVVersion =5;      // ab da ist OPNV möglich

#pragma pack(1)
typedef
 struct {
  unsigned short Lng;      // Gesmtlänge des Programms
  unsigned short Version;  // Version
  unsigned short AmpPnt;   // ->amp Anz Ampeln   Zuordnung zu den Gruppen
  unsigned short GrpPnt;   // ->mtx Anz Gruppen  Zwischenzeitenmatrix
  unsigned short TypPnt;   // ->sgn TypDef Size  Definition der Signalgebertypen
  unsigned short SZtPnt;   // ->szt Schaltzeiten
  unsigned short PrgPnt;   // ->prg Anz Prg
  unsigned short NamePnt;  // zeigt auf den Namen  ab Version 3
	unsigned short SyncPnt;  // zeigt auf die Rückrechnung
 } TMultiPnt;
typedef TMultiPnt * PMultiPnt;

typedef struct {
  unsigned char Typ;
  unsigned short Umlauf;
	unsigned short GSP;
} TMultiPrgParam;
typedef TMultiPrgParam * PMultiPrgParam;

typedef struct {
  unsigned char Sens;
  unsigned short Gruen;
  unsigned short Dehn;
	unsigned short EinZt;
	unsigned short AusZt;
	unsigned char Prg;
} TMultiPrgDef;
typedef TMultiPrgDef * PMultiPrgDef;

typedef struct {
  unsigned char Timeout;  // in 31ms Schritten )
  unsigned char maxdim;  // MaxDimm )
  unsigned char nachtab;  // MinDim  )
  unsigned char rotmin;  //
  unsigned char gruenmin;  //
  unsigned short rotmax;  //
  unsigned short Qallowed;  //  IF FLAG ELSE [0] THEN 2M!
  unsigned short manuell;  //  IF FLAG ELSE [0] THEN 2M!
  union {
   struct {
    unsigned char grnbl;
    unsigned char OPNV;
   };  //
   struct {
    unsigned char globerr;
    unsigned char usagrnbl;
    unsigned char hupe;
    unsigned char usaOPNV;
   };
  };
} TMultiparam;
typedef TMultiparam * PMultiParam;

#pragma pack()

#pragma pack(1)
const TSignalStr mssignaltypen[18] =
  { 0x20,0x44,0x99,0x20,0x44,0x40,0x22,0xEE,0x00,0x10,0xFF,0xFF,
    0x00,0x44,0x99,0x00,0x44,0x40,0x22,0xEE,0x00,0x10,0xFF,0xFF,
    0x00,0x44,0x99,0x00,0x44,0x40,0x00,0x00,0x00,0x00,0xFF,0xFF,
    0x20,0x44,0xA8,0x20,0x44,0x40,0x22,0xEE,0x00,0x00,0xFF,0xFF,
    0x20,0x44,0x98,0x20,0x44,0x40,0x22,0xEE,0x00,0x00,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x20,0x44,0x99,0x20,0x44,0x40,0x22,0xEE,0x00,0x20,0x20,0x10,
    0x40,0x44,0x99,0x40,0x44,0x40,0x22,0x00,0x00,0x20,0x20,0x10,
    0x00,0x00,0x99,0x00,0x00,0x00,0x22,0x00,0x00,0x20,0x20,0x10,
    0x00,0x44,0x99,0x00,0x44,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x44,0x99,0x40,0x44,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
    0x20,0x44,0x88,0x00,0x44,0x40,0x22,0x00,0x00,0x00,0x20,0x00,
    0x40,0x44,0x88,0x00,0x44,0x40,0x22,0x00,0x00,0x00,0x20,0x00,
    0x00,0x44,0xC8,0x00,0x44,0x40,0x00,0x00,0x00,0x40,0x00,0x00,
    0x40,0x44,0xC8,0x40,0x44,0x40,0x00,0x00,0x00,0x40,0x00,0x00,
    0x20,0x44,0xA8,0x20,0x44,0x40,0x22,0x00,0x00,0x20,0x20,0x00,
    0x40,0x44,0xA8,0x40,0x44,0x40,0x22,0x00,0x00,0x20,0x20,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };
#pragma pack()

int TBaustelle::PrgLng()
{ unsigned char x;
  x=Tab[1];
  return (Tab[0]<<8)+x; }

int TBaustelle::AnzGrp()
{ int idx;
  idx=swap(((PMultiPnt)Tab)->GrpPnt);
  return Tab[idx];
}

int TBaustelle::SgnIdx(int Grp)
{ int idx,n;
  n=AnzGrp();
  idx=swap(((PMultiPnt)Tab)->GrpPnt)+1+(n*n+Grp-1)*2;
  return Tab[idx];
};

TSignalStr *TBaustelle::Signal(int Grp)
{ int idx,n,Sz;
  n=SgnIdx(Grp);
  idx=swap(((PMultiPnt)Tab)->TypPnt);
  Sz=Tab[idx];
  idx=idx+1+n*Sz;
  memcpy(&Sint,(void *)&Tab[idx],sizeof(TSignalStr));
  return (TSignalStr *)Sint;
}

TLSGTyp TBaustelle::SgnTyp(int Grp)
{  TSignalStr s;
   TSignalStr sm;
   int n;
 memcpy(&s,Signal(Grp),sizeof(s));
 n=swap(((PMultiPnt)Tab)->TypPnt);
 n=Tab[n];
 if (n<=10) {
  for (n=mssgnneben;n<=mssgnend1;++n) {
   if (memcmp(&s,&mssignaltypen[n],9)==0) break;
  };
 } else {
  s[sgn_fehler]=0; s[sgn_blink]=0; s[sgn_qrz]=0; //s:=copy(s,1,11);
  for (n=mssgnstandardD;n<=mssgnend2;++n) {
   memcpy(&sm,&mssignaltypen[n],sizeof(TSignalStr));
   sm[sgn_fehler]=0; sm[sgn_blink]=0; sm[sgn_qrz]=0;
   if (memcmp(&s,&sm,11)==0) break;
  };
 };
 switch (n) {
  case mssgnneben: case mssgnstandardD: case mssgnstandard:  return lsgNeben;
  case mssgnhaupt:                                           return lsgHaupt;
  case mssgnfuss: case mssgnpedesD: case mssgnpedes:         return lsgFuss;
  case mssgnyellbl: case mssgnflashD:                        return lsgYellbl;
  case mssgnyellbl3:                                         return lsgYellbl3;
  case mssgnturn:                                            return lsgRPfeil;
  case mssgnoffD: case mssgnoff:                             return lsgOff;
  case mssgnrotD: case mssgnrot:                             return lsgRed;
  case mssgnflash:                                           return lsgYellblink;
  default:                                                   return lsgNeben;
 };
}

int TBaustelle::Version()
{ return swap(((PMultiPnt)Tab)->Version); }

int TBaustelle::GrNr(int Nr)
{ int idx;
  idx=swap(((PMultiPnt)Tab)->AmpPnt)+Nr;
  return Tab[idx];
}

char *TBaustelle::GrBez(int Nr)
{ if (SgnTyp(Nr)==lsgFuss) sprintf(Sint,"F%1d",Nr);
  else sprintf(Sint,"K%1d",Nr);
  return Sint;
}

char *TBaustelle::Title()
{ int idx,i,b;
  if (Version()<MinNameVersion) strcpy(Sint,"Multi Signal");
  else {
   idx=swap(((PMultiPnt)Tab)->NamePnt);
   b=0;
   for (i=idx+1;i<=idx+Tab[idx];++i,++b) Sint[b]=Tab[i];
   Sint[b]=0;
   ConvertName(Sint);
  };
  return Sint;
}

int TBaustelle::ZwZt(int R,int E)
{ int idx,n;
  unsigned char x;
  if (R==E) return -1;
  else {
   n=AnzGrp();
   idx=swap(((PMultiPnt)Tab)->GrpPnt)+1+((R-1)*n+(E-1))*2;
   x=Tab[idx+1];
   return (Tab[idx]<<8)+x;
  };
}

int TBaustelle::Umlauf(int Nr)
{ int idx,x,Sz;
  idx=swap(((PMultiPnt)Tab)->PrgPnt);
  Sz=Tab[idx+1];
  idx=idx+2+(Nr-1)*Sz;
//  x=swap(*((unsigned short *)&Tab[idx]));     // holt den Parameterpointer des Programms
  x=(Tab[idx]<<8); x+=Tab[idx+1];
  idx=x+idx;
  return swap( ( (PMultiPrgParam)&Tab[idx])->Umlauf);
}

int TBaustelle::Gruen(int Prg,int Grp)
{ int idx,x,Sz;
  idx=swap(((PMultiPnt)Tab)->PrgPnt);
  Sz=Tab[idx+1];
  idx=idx+2+(Prg-1)*Sz;
//  x=swap(*((unsigned short *)&Tab[idx+2]));   // holt den def pointer des Programms
  x=(Tab[idx+2]<<8); x+=Tab[idx+3];
  idx=x+idx;
  Sz=Tab[idx];
  idx=idx+1+(Grp-1)*Sz;
  return swap( ((PMultiPrgDef)&Tab[idx])->Gruen);
}

int TBaustelle::Dehn(int Prg,int Grp)
{ int idx,x,Sz;
  idx=swap(((PMultiPnt)Tab)->PrgPnt);
  Sz=Tab[idx+1];
  idx=idx+2+(Prg-1)*Sz;
  x=Tab[idx+2]<<8; x=x+Tab[idx+3];
  idx=x+idx;
  Sz=Tab[idx];
  idx=idx+1+(Grp-1)*Sz;
  return swap( ((PMultiPrgDef)&Tab[idx])->Dehn);
}

int TBaustelle::Sensor(int Prg,int Grp)
{ int idx,x,Sz;
  idx=swap(((PMultiPnt)Tab)->PrgPnt);
  Sz=Tab[idx+1];
  idx=idx+2+(Prg-1)*Sz;
//  x=swap(*((unsigned short *)&Tab[idx+2]));   // holt den def pointer des Programms
  x=(Tab[idx+2]<<8); x+=Tab[idx+3];
  idx=x+idx;
  Sz=Tab[idx];
  idx=idx+1+(Grp-1)*Sz;
  return ((PMultiPrgDef)&Tab[idx])->Sens;
}

int TBaustelle::ErrIdx(int Grp)
{ int idx,n;
  n=AnzGrp();
  idx=swap(((PMultiPnt)Tab)->GrpPnt)+2+(n*n+Grp-1)*2;
  return Tab[idx];
}

int TBaustelle::AnzPrg()
{ int idx;
  idx=swap(((PMultiPnt)Tab)->PrgPnt);
  return Tab[idx];
}

int TBaustelle::RotGelb(int Nr)
{ int idx,n;
  n=AnzGrp();
  idx=swap(((PMultiPnt)Tab)->GrpPnt)+1+(n*(Nr-1)+(Nr-1))*2;
  return Tab[idx];
}

int TBaustelle::Gelb(int Nr)
{ int idx,n;
  n=AnzGrp();
  idx=swap(((PMultiPnt)Tab)->GrpPnt)+1+(n*(Nr-1)+(Nr-1))*2;
  return Tab[idx+1];
}

int TBaustelle::EinZt(int BlNr,int GrNr)
{ return 0; }

int TBaustelle::AusZt(int BlNr,int GrNr)
{ return 0; }

TLSGTyp TBaustelle::LSGTyp(int Nr)
{ return SgnTyp(GrNr(Nr)); }


int TBaustelle::ZwZtMax(int R)
{ return 0; }

char *TBaustelle::KopfBez(int Nr)
{ GrBez(GrNr(Nr));
  strcat(Sint,":");
  sprintf(&Sint[strlen(Sint)],"%1d",Nr);
  return Sint;
}

char *TBaustelle::TageszeitBlk(int Dest)
{ sprintf(Sint,"Program %d",Dest);
  return Sint;
}

/*function TBaustelle.rdTag( Nr: byte): TTageszeitBlk;
begin
 result.ModeW:=0;
end;
*/

int TBaustelle::GrpBits(int Nr)
{ return 0; }

int TBaustelle::AnzAmp()
{ int idx;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  return Tab[idx];                        // Anzahl der Ampeln
}

int TBaustelle::AmpproGr(int Nr)
{ int idx,n,result;
  result=0;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  for (n=1;n<=Tab[idx];++n) if (Tab[idx+n]==Nr) ++result;
  return result;
}

float TBaustelle::Timeout()
{ int idx,n;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  n=Tab[idx];                        // Anzahl der Ampeln
  idx=idx+n+1;
  return ((PMultiParam)&Tab[idx])->Timeout*0.03125;
}

float TBaustelle::Luecke(int Nr)
{ int idx,n;
  n=AnzGrp();
  idx=swap(((PMultiPnt)Tab)->GrpPnt)+1+(n*(n+1)+(Nr-1))*2;
  return Tab[idx]*0.1;
}

int TBaustelle::MaxDimm()
{ int idx;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return ((PMultiParam)&Tab[idx])->maxdim;
}

int TBaustelle::MinDimm()
{ int idx;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return ((PMultiParam)&Tab[idx])->nachtab;
}

int TBaustelle::MaxRot()
{ int idx;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return ((PMultiParam)&Tab[idx])->rotmax;
}

int TBaustelle::MinRot()
{ int idx;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return ((PMultiParam)&Tab[idx])->rotmin;
}

bool TBaustelle::QAllowed()
{ int idx;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return (((PMultiParam)&Tab[idx])->Qallowed==0xA55A);
}

int TBaustelle::Gruenmin()
{ int idx;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return ((PMultiParam)&Tab[idx])->gruenmin;
}

bool TBaustelle::Manuell()
{ int idx;
  if (Version()<MinManuellVersion) return false;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return (((PMultiParam)&Tab[idx])->manuell==0xA55A);
}

int TBaustelle::GrnBl()
{ int idx;
  if (Version()<MinGruenBlVersion) return 0;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return ((PMultiParam)&Tab[idx])->grnbl;
}

bool TBaustelle::OPNV()
{ int idx;
  if (Version()<MinOPNVVersion) return false;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return (((PMultiParam)&Tab[idx])->OPNV==0xFF);
}

int TBaustelle::AnzUPkte()
{ int idx;
  idx=swap(((PMultiPnt)Tab)->SZtPnt);
  return Tab[idx];
}

TUPkt TBaustelle::UPkt(int Nr)
{ int idx;
  idx=swap(((PMultiPnt)Tab)->SZtPnt)+1+(Nr-1)*sizeof(TUPkt);
  return *((TUPkt *)&Tab[idx]);
}

int TBaustelle::UTage(int Nr)
{ return UPkt(Nr).Tagesbit & 0x7F; }

int TBaustelle::UZeit(int Nr)
{ return swap(UPkt(Nr).Zeit) & 0x7FF; }

int TBaustelle::UPrgNr(int Nr)
{ return (swap(UPkt(Nr).Zeit) >> 11) & 0x1F; }

//--------------------------------------------------------------------------------------
char *TMSUSABauPrg::GrBez(int Nr)
{ if (SgnTyp(Nr)==lsgFuss) sprintf(Sint,"P%1d",Nr); else sprintf(Sint,"C%1d",Nr);
  return Sint;
}

TSgnNr TMSUSABauPrg::ErrorSgn(int Grp)
{ Signal(Grp);
  return MSSignalNr(Sint[sgn_fehler]);
}

TSgnNr TMSUSABauPrg::FlashSgn(int Grp)
{ Signal(Grp);
  return MSSignalNr(Sint[sgn_blink]);
}

TSgnNr TMSUSABauPrg::QuarzSgn(int Grp)
{ Signal(Grp);
  return MSSignalNr(Sint[sgn_qrz]);
}

TSgnNr TMSUSABauPrg::GlbErrSgn()
{ int idx;
  if (Version()<MinUSAVersion) return sgnRotBl;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  idx=((PMultiParam)&Tab[idx])->globerr & 7;
  switch (idx) {
   case 1: return sgnRotBl;
   case 2: return sgnRot;
   case 3: return sgnAus;
   default: return sgnblink;
  };
}

TSgnNr TMSUSABauPrg::GlbFlashSgn()
{ int idx;
  if (Version()<MinUSAVersion) return sgnRotBl;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  idx=(((PMultiParam)&Tab[idx])->globerr & 0x18) >> 3;
  switch (idx) {
   case 1: return sgnRotBl;
   case 2: return sgnAus;
   default: return sgnblink;
  };
}

THupe TMSUSABauPrg::Hupe()
{ int idx;
  if (Version()<MinGruenBlVersion) return hupe_horn;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  idx=(((PMultiParam)&Tab[idx])->hupe);
  switch (idx) {
   case 0xA5: return hupe_error;
   case 0xBE: return hupe_mode;
   default: return hupe_horn;
  };
}

int TMSUSABauPrg::GrnBl()
{ int idx;
  if (Version()<MinGruenBlVersion) return 0;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return ((PMultiParam)&Tab[idx])->usagrnbl;
}

bool TMSUSABauPrg::OPNV()
{ int idx;
  if (Version()<MinOPNVVersion) return false;
  idx=swap(((PMultiPnt)Tab)->AmpPnt);
  idx=idx+Tab[idx]+1;
  return (((PMultiParam)&Tab[idx])->usaOPNV==0xFF);
}

char *TMSUSABauPrg::wrGruppe(char *S,int Nr,int &Um)
{ char sx[30];
  if (Um==1) sprintf(S,"%-6s%-13s%-7s%-6s%-5s%-16s%-16s\r\n","Group","Type","Lights","Yell."," Gap","Error Sgn.","Flash Sgn.");
  else S[0]=0;
  sprintf(sx,"%-6s",GrBez(Nr)); strcat(S,sx);
  sprintf(sx,"%-13s",LSGTypCaptionUSA[SgnTyp(Nr)]); strcat(S,sx);
  sprintf(sx,"%6d ",AmpproGr(Nr)); strcat(S,sx);
  sprintf(sx,"%5d ",Gelb(Nr)); strcat(S,sx);
  sprintf(sx,"%4.1f ",Luecke(Nr)); strcat(S,sx);
  sprintf(sx,"%-16s",SgnNrCaption[ErrorSgn(Nr)+1]); strcat(S,sx);
  sprintf(sx,"%-16s",SgnNrCaption[FlashSgn(Nr)+1]); strcat(S,sx);
  return S;
}

int TMSUSABauPrg::Script(char *Memo)
{ int n,e,r;
  char *s;
  char *sx;
  charcount=0; Memo[0]=0;
  if (Tab[0]>=0xAB) { zeile(Memo,"not available !"); return charcount; };
  s=(char *)malloc(300); if (s==0) return charcount;
  sx=(char *)malloc(50); if (sx==0) return charcount;
  n=Version();
  if (n>=MinNameVersion) { sprintf(s,"%-9s%s","Title:",Title()); zeile(Memo,s); };
  sprintf(s,"%-9s%x.%.2x","Version:",(n>>8) & 0xFF,n & 0xFF);    zeile(Memo,s);
  zeile(Memo," ");
  sprintf(s,"%-8s%d","Groups:",AnzGrp());  zeile(Memo,s);
  zeile(Memo," ");
  for (n=1;n<=AnzGrp();++n) zeile(Memo,wrGruppe(s,n,n));
  zeile(Memo," ");
  sprintf(s,"Number of programs: %d",AnzPrg()); zeile(Memo,s);

  for (n=1;n<=AnzPrg();++n) {
   zeile(Memo," ");
   sprintf(s,"Program %d",n); zeile(Memo,s);
   sprintf(s,"%-6s%-5s%-6s%-8s%-12s%-9s","Group"," Red","Green","Extens.","Rest in red","Priority"); zeile(Memo,s);
   for (r=1;r<=AnzGrp();++r) {
    if (r==AnzGrp()) e=1; else e=r+1;
    e=ZwZt(r,e)-Gelb(r)-RotGelb(e);
    sprintf(s,"%-6s%4d %5d %+7d ",GrBez(r),e,Gruen(n,r),Dehn(n,r));
   //    80	EQU ^ST.Anf	  40	EQU ^ST.Vor
    e=Sensor(n,r);
    if (e & 0x80) sprintf(sx,"%12s","yes "); else sprintf(sx,"%12s","no "); strcat(s,sx);
    if (e & 0x40) sprintf(sx,"%9s","yes "); else sprintf(sx,"%9s","no "); strcat(s,sx);
    zeile(Memo,s);
   };
  };

  zeile(Memo," ");
  zeile(Memo,"Intergreen matrix");
  for (r=0;r<=AnzGrp();++r) {
   if (r==0) strcpy(s,"//  "); else sprintf(s,"%-4s",GrBez(r));
   for (e=1;e<=AnzGrp();++e) {
    if (r==0) {
     sprintf(sx,"%4s",GrBez(e)); strcat(s,sx);
    } else if (r==e) strcat(s," xxx");
    else { sprintf(sx,"%4d",ZwZt(r,e)); strcat(s,sx);
    };
   };
   zeile(Memo,s);
  };

  zeile(Memo," ");
  sprintf(s,"Switch times%3d",AnzUPkte()); zeile(Memo,s);
  for (n=1;n<=AnzUPkte();++n) {
   sprintf(s,"%2d  ",n);
   strcat(s,Mdoppel(sx,UTage(n),"MoTuWeThFrSaSu"));
   s[strlen(s)-4]=0; strcat(s,"  ");
   e=UZeit(n);
   sprintf(sx,"%02d:%02d  ",e/60,e-(e/60)*60); strcat(s,sx);
   strcat(s,PrgStr(sx,UPrgNr(n)));
   zeile(Memo,s);
  };

  zeile(Memo," ");
// zeile(alignleft('Dimmen:',16)+format('%d%% ... %d%%',[MinDimm,MaxDimm]));
  if (MaxRot()==0) sprintf(s,"%-17s%s","Max. red:","---"); else sprintf(s,"%-17s%ds","Max. red:",MaxRot()); zeile(Memo,s);
  sprintf(s,"%-17s%ds","Min. red:",MinRot()); zeile(Memo,s);
  sprintf(s,"%-17s%ds","Min. green:",Gruenmin()); zeile(Memo,s);
  if (Version()>=MinGruenBlVersion) {
   if (GrnBl()==0) sprintf(s,"%-17s%s","Flash green:","---"); else sprintf(s,"%-17s%ds","Flash green:",GrnBl()); zeile(Memo,s);
  };


  zeile(Memo," ");
  zeile(Memo,"Options");
  if (Version()>=MinManuellVersion) {
   if (Manuell()) sprintf(s,"%-17s%s","Change cl. time:","yes"); else sprintf(s,"%-17s%s","Change cl. time:","no"); zeile(Memo,s);
  };
  sprintf(s,"%-17s%.1f","Timeout:",Timeout()); zeile(Memo,s);
  if (QAllowed()) sprintf(s,"%-17s%s","Quartz mode:",SgnNrCaption[QuarzSgn(1)+1]); else sprintf(s,"%-17s%s","Quartz mode:","---"); zeile(Memo,s);
  if (Version()>=MinUSAVersion) {
   sprintf(s,"%-17s%s","Error signal:",SgnNrCaption[GlbErrSgn()+1]); zeile(Memo,s);
   sprintf(s,"%-17s%s","Flash signal:",SgnNrCaption[GlbFlashSgn()+1]); zeile(Memo,s);
  };
  sprintf(s,"%-17s%s","Horn:",HornCaption[Hupe()]); zeile(Memo,s);
  if (Version()>=MinOPNVVersion) {
   if (OPNV()) sprintf(s,"%-17s%s","Public transp.:","yes"); else sprintf(s,"%-17s%s","Public transp.:","no"); zeile(Memo,s);
  };
  zeile(Memo," ");
  free(sx);
  free(s);
  return charcount;
}
//---------------------------- TMSUSAneuPrg --------------------------
#pragma pack(1)
typedef uint32_t TSignalTypNeu[sgn_MAX];
#pragma pack()

char * TMSUSAneuPrg::SignalName(int Grp, TMSSignalBilder Sgn)
{ int idx,n,Sz;
  uint32_t xx;
  TSignalTypNeu * P;
  n=SgnIdx(Grp);
  idx=swap(((PMultiPnt)Tab)->TypPnt);
  Sz=Tab[idx];
  idx=idx+1+n*Sz;
	memcpy(&xx,(void *)&Tab[idx+Sgn*sizeof(xx)],sizeof(xx));
	switch (xx) {
		case 0x00110011: return "Red";
		case 0x00110000: return "Flash red";
 		case 0x08110800: return "Flash red";
  	case 0x00220022: return "Yellow";
 		case 0x00220000: return "Flash yel";
 		case 0x08220800: return "Flash yel";
 		case 0x08440844: return "Green";
 		case 0x00440000: return "Flash grn";
 		case 0x00000000: return "Off";
 		case 0x08000800: return "Off";
 		case 0x00330033: return "Red/Yel.";
 		case 0x08330833: return "Red/Yel.";
 		case 0x08880800: return "Fl.Ye.Ar.";
		default:         return "???";
	}
}

char TMSUSAneuPrg::SgnCh(int GNr,int SgNr)
{ int i;
  i=1;
  while ((i<=8) && (GrNr(i)!=GNr)) ++i;
  if (i<=8) return SgnChars[LSGTyp(i)][SgNr]; else return SgnChars[lsgNeben][SgNr];
}

int TMSUSAneuPrg::RueckRTyp()
{ int idx;
  uint16_t P;
  idx=swap(((PMultiPnt)Tab)->SyncPnt);
	P=(Tab[idx]<<8)+Tab[idx+1];
  if (P==(unsigned short)0xFFFF) { P=Tab[idx+2]; P+=(Tab[idx+3]<<8); return P; }
  else return 0;
}

int TMSUSAneuPrg::Versatz()
{ int idx;
  int16_t V;
  idx=swap(((PMultiPnt)Tab)->SyncPnt)+4;
	V=(Tab[idx]<<8) | Tab[idx+1];
  return V;
}

uint32_t TMSUSAneuPrg::RueckRZeit()
{ int idx;
	uint32_t result;
  idx=swap(((PMultiPnt)Tab)->SyncPnt);
  result=Tab[idx+3];
  result+=Tab[idx+2]<<8;
  result+=Tab[idx+1]<<16;
  result+=Tab[idx]<<24;
  return result;
}

int TMSUSAneuPrg::EinZt(int Prg,int Grp)
{ int idx,x,Sz;
  idx=swap(((PMultiPnt)Tab)->PrgPnt);
  Sz=Tab[idx+1];
  idx=idx+2+(Prg-1)*Sz;
  x=Tab[idx+2]<<8; x=x+Tab[idx+3];
  idx=x+idx;
  Sz=Tab[idx];
  idx=idx+1+(Grp-1)*Sz;
  return swap( ((PMultiPrgDef)&Tab[idx])->EinZt);
}

int  TMSUSAneuPrg::AusZt(int Prg,int Grp)
{ int idx,x,Sz;
  idx=swap(((PMultiPnt)Tab)->PrgPnt);
  Sz=Tab[idx+1];
  idx=idx+2+(Prg-1)*Sz;
  x=Tab[idx+2]<<8; x=x+Tab[idx+3];
  idx=x+idx;
  Sz=Tab[idx];
  idx=idx+1+(Grp-1)*Sz;
  return swap( ((PMultiPrgDef)&Tab[idx])->AusZt);
}

int  TMSUSAneuPrg::JmpPrg(int Prg,int Grp)
{ int idx,x,Sz;
  idx=swap(((PMultiPnt)Tab)->PrgPnt);
  Sz=Tab[idx+1];
  idx=idx+2+(Prg-1)*Sz;
  x=Tab[idx+2]<<8; x=x+Tab[idx+3];
  idx=x+idx;
  Sz=Tab[idx];
  idx=idx+1+(Grp-1)*Sz;
  return ((PMultiPrgDef)&Tab[idx])->Prg;
}

bool TMSUSAneuPrg::Vorrang(int Prg,int Grp)
{ int idx,x,Sz;
  idx=swap(((PMultiPnt)Tab)->PrgPnt);
  Sz=Tab[idx+1];
  idx=idx+2+(Prg-1)*Sz;
  x=Tab[idx+2]<<8; x=x+Tab[idx+3];
  idx=x+idx;
  Sz=Tab[idx];
  idx=idx+1+(Grp-1)*Sz;
  return (((((PMultiPrgDef)&Tab[idx])->Sens) & 0x40) != 0);
}

bool TMSUSAneuPrg::Anforderung(int Prg,int Grp)
{ int idx,x,Sz;
  idx=swap(((PMultiPnt)Tab)->PrgPnt);
  Sz=Tab[idx+1];
  idx=idx+2+(Prg-1)*Sz;
  x=Tab[idx+2]<<8; x=x+Tab[idx+3];
  idx=x+idx;
  Sz=Tab[idx];
  idx=idx+1+(Grp-1)*Sz;
  return (((((PMultiPrgDef)&Tab[idx])->Sens) & 0x80) != 0);
}

uint8_t TMSUSAneuPrg::Ablauf(int Nr)
{ int idx,x,Sz;
	idx=swap(((PMultiPnt)Tab)->PrgPnt);
	Sz=Tab[idx+1];         // Länge eines Programms
  idx=idx+2+(Nr-1)*Sz;
  x=Tab[idx]<<8; x+=Tab[idx+1];
  idx=x+idx;
  return ((PMultiPrgParam)&Tab[idx])->Typ;
}

uint8_t TMSUSAneuPrg::GSP(int Nr)
{ int idx,x,Sz;
	idx=swap(((PMultiPnt)Tab)->PrgPnt);
	Sz=Tab[idx+1];         // Länge eines Programms
  idx=idx+2+(Nr-1)*Sz;
  x=Tab[idx]<<8; x+=Tab[idx+1];
  idx=x+idx;
  return ((PMultiPrgParam)&Tab[idx])->GSP;
}

TLSGTyp TMSUSAneuPrg::SgnTyp(int Grp)
{ int idx,n;
  n=AnzGrp();
  idx=swap(((PMultiPnt)Tab)->GrpPnt)+1+(n*(n+2)+Grp-1)*2;
	switch (Tab[idx]) {
   case 0: return lsgNeben;
   case 1: return lsgTurn;
   case 2: return lsgFuss;
   case 3: return lsgOff;
   case 4: return lsgRed;
   case 5: return lsgYellblink;
   case 6: return lsgBlinker;
   case 7: return lsgController;
   default: return lsgNeben;
	};
};

char *TMSUSAneuPrg::wrGruppe(char *S,int Nr,int &Um)
{ int U,n;
  if (Nr==1) Um=1;
  S[0]=0;
	if (Nr<=MAXGRP) {
   n=sprintf(S,"%1d %-5s%-12s%5d%5.1f%5s%4s",
     Nr,GrBez(Nr),LSGTypCaptionUSA[SgnTyp(Nr)],AmpproGr(Nr),Gelb(Nr)/10.0,
     Vorrang(1,Nr)?"yes":"no",Anforderung(1,Nr)?"yes":"no");
   U=JmpPrg(1,Nr);
   if (U>0) {
    n+=sprintf(&S[n],"%5d",U);
   } else n+=sprintf(&S[n],"   --");
   n+=sprintf(&S[n],"%6.1f ",Luecke(Nr));
   n+=sprintf(&S[n],"%-10s",SignalName(Nr,sgn_fehler));
   n+=sprintf(&S[n],"%-10s",SignalName(Nr,sgn_blink));
  };
  return S;
}


int TMSUSAneuPrg::Script(char *Memo)
{ char *S;
  char *Sx;
  int i,k,j,U,Um,n,Ez,Az,RtGb,Gb;
  int Tx,Ux;
  int Gr;
  int Prg,Sch;
  float G,Mod;

  charcount=0; Memo[0]=0;
  S=(char *)malloc(300); if (S==0) return charcount;
  Sx=(char *)malloc(300); if (Sx==0) return charcount;
  sprintf(S,"Title        %s",Title());    zeile(Memo,S);
  sprintf(S,"Version      %X",Version());  zeile(Memo,S);
  zeile(Memo," ");
  zeile(Memo,"OPTIONS");
  sprintf(S,"Timeout      %.1fs",Timeout()); zeile(Memo,S);
//  sprintf(S,"Horn output  %s",getHornOutput(Hupe())); zeile(Memo,S);
  zeile(Memo," ");

  zeile(Memo,"MS+");
  sprintf(S,"Number of groups%3d",AnzGrp());  zeile(Memo,S);
  zeile(Memo,"Group  Type         Lgt. Yel. Pri. Red Dem. Gap/s Error     Flash");
	for (i=1;i<=AnzGrp();++i) {
		wrGruppe(S,i,Um);
   	zeile(Memo,S);
  };
  if (strlen(S)>0) zeile(Memo," ");

  zeile(Memo,"Intergreen matrix");
  strcpy(S,"   "); i=1; Gr=AnzGrp(); n=strlen(S);
	for (i=1;i<=Gr;++i) {
   sprintf(Sx,"%3s ",GrBez(i));
   strcat(S,Sx);
  };
  zeile(Memo,S);

  for (i=1;i<=Gr;++i) {
   sprintf(S,"%-3s",GrBez(i));
   for (k=1;k<=Gr;++k) {
    if (k==i) {
     if (i>Gr) strcat(S,"    "); else strcat(S,"xxx ");
    } else if (ZwZt(i,k)==128) {
     if ((i>Gr) || (k>Gr)) strcat(S,"    "); else strcat(S," -- ");
    } else if (ZwZt(i,k)>128) {
     sprintf(Sx,"%3d ",ZwZt(i,k)-256); strcat(S,Sx);
    } else {
     sprintf(Sx,"%3d ",ZwZt(i,k)); strcat(S,Sx);
    };
   };
   trim(S);
   if (strlen(S)) zeile(Memo,S);
  };

  zeile(Memo," ");
	Prg=AnzPrg();
  sprintf(S,"Number of programs%3d",Prg); zeile(Memo,S);
  zeile(Memo," ");
  strcpy(S,"     ");
  for (i=1;i<=Prg;++i) { sprintf(Sx,"Program%2d     ",i); strcat(S,Sx); }
  zeile(Memo,S);
  strcpy(S,"Type ");
  for (i=1;i<=Prg;++i) {
   switch (Ablauf(i)) {
    case 0: strcat(S,"fixed cycle   "); break;
    case 1: strcat(S,"ext. cycle    "); break;
    case 2: strcat(S,"Demand        "); break;
    case 3: strcat(S,"Special       "); break;
    case 4: strcat(S,"ext. fixed    "); break;
    case 5: strcat(S,"ext. ext.     "); break;
    default:strcat(S,"???           "); break;
   };
  };
  zeile(Memo,S);
  strcpy(S,"Cyc. ");
  for (i=1;i<=Prg;++i) {
   sprintf(Sx,"%3d s         ",Umlauf(i));
   strcat(S,Sx);
  };
  zeile(Memo,S); strcpy(S,"GSP  ");
  for (i=1;i<=Prg;++i) {
   sprintf(Sx,"%3d           ",GSP(i)+1);
   strcat(S,Sx);
  };
  zeile(Memo,S);
  zeile(Memo," ");
  strcpy(S,"     ");
  for (i=1;i<=Prg;++i) strcat(S,"frm to  ext.  ");
  zeile(Memo,S);

  for (k=1;k<=Gr;++k) {
   sprintf(S,"%-5s",GrBez(k));
   i=1;
   while (i<=Prg) {
    Tx=EinZt(i,k); Ux=Umlauf(i);
    if (Tx>250) strcpy(Sx," -- ");
    else {
     Tx=AddUml(Ux,RotGelb(k),Tx)+1;
     sprintf(Sx,"%3d ",Tx);
    };
    strcat(S,Sx);
    Tx=AusZt(i,k);
    if (Tx>250) strcpy(Sx," -- ");
    else {
     Tx=SubUml(Ux,Tx,1)+1;
     sprintf(Sx,"%3d ",Tx);
    };
    strcat(S,Sx);
    Tx=Dehn(i,k);
    if ((Tx>=0xF1) && (Tx<=0xFF)) sprintf(Sx,"%3s ",GrBez(0x100-Tx));
    else if (Tx==0xF0) strcpy(Sx,"fixed");
    else sprintf(Sx,"%3d   ",Tx);
    strcat(S,Sx);
    ++i;
   };
   zeile(Memo,S);
  };

  zeile(Memo," ");
  Sch=AnzUPkte();
  sprintf(S,"Number of switch times%2d",Sch);
  zeile(Memo,S);
  for (i=1;i<=Sch;++i) {
   sprintf(S,"%2d  ",i);
   strcat(S,Mdoppel(Sx,UTage(i),"MoTuWeThFrSaSu"));
   S[strlen(S)-4]=0; strcat(S,"  ");
   k=UZeit(i);
   Mod=modf(k/60.0,&G);
   sprintf(Sx,"%02.0f:%02.0f",G,Mod); strcat(S,Sx);
   strcat(S,"  ");
   strcat(S,PrgStr(Sx,UPrgNr(i)));
   zeile(Memo,S);
  };

  zeile(Memo," ");
  zeile(Memo,"PARAMETER");
  if (MaxRot()==0) strcpy(Sx,"unlimited"); else sprintf(Sx,"%ds",MaxRot());
  strcpy(S,"max. red         "); strcat(S,Sx);
  zeile(Memo,S);
  sprintf(S,"min. green       %ds",Gruenmin()); zeile(Memo,S);
  sprintf(S,"min. red         %ds",MinRot());   zeile(Memo,S);
  n=GrnBl();
  if (n) {
   sprintf(S,"Green flash      %ds",n); zeile(Memo,S);
  };
  strcpy(S,"Back calculation ");
  switch (RueckRTyp()) {
   case 0xFFFF: strcat(S,"Sw  switch point"); break;
   case 0xFFFE: strcat(S,"S2  01.01.-- 00:00"); break;
   case 0xFFFD: strcat(S,"S3  01.01.80 00:00"); break;
   case 0xFFFC: strcat(S,"WI  Wait for impulse"); break;
   case 0xFFFB: strcat(S,"Sy  Synchron impulse"); break;
   default:     strcat(S,"Ti  "); strcat(S,SecDatum(Sx,RueckRZeit())); break;
  };
  zeile(Memo,S);
  strcpy(S,"Delay            ");
  if (Versatz()==0xFF) strcat(S,"GSP"); else { sprintf(Sx,"%ds",Versatz()); strcat(S,Sx); };
  zeile(Memo,S);
  zeile(Memo," ");

  if (SgnPlan) {
   for (i=1;i<=Prg;++i) {
    sprintf(S,"Program%2d",i); zeile(Memo,S); strcpy(S,"s      "); Sx[0]=0;
    n=strlen(S); j=0;
    for (k=1;k<=Umlauf(i);++k) {
     if ((k/10)*10==k) {
      ++j; if (j==10) j=0;
      sprintf(Sx,"%1d",j);
     } else if (modf(k/5.0,&G)==0) strcpy(Sx,":");
     else strcpy(Sx,".");
     strcat(S,Sx);
    };
    S[GSP(i)+n]='G';
    zeile(Memo,S);
    U=Umlauf(i);

    for (k=1;k<=Gr;++k) {
     RtGb=RotGelb(k);
     Gb=Gelb(k);
     strcpy(S,GrBez(k));
     Ez=EinZt(i,k);
     Az=AusZt(i,k);
     if ((Ez<253) && (Az<253) && (Ez!=Az)) {
      sprintf(Sx,"%4d ",SubUml(U,Az,Ez)-RtGb);
      strcat(S,Sx);
     } else strcat(S,"     ");
     if ((U>0) && (U<255) && (Ez!=Az)) {
      for (n=0;n<U;++n) Sx[n]=' '; Sx[U]=0;
      if ((Az>=0) && (Az<=U) && (Ez>=0) && (Ez<=U)) {
       j=Ez+1; if (j>U) j=1;
       for (n=1;n<=RtGb;++n) { Sx[j-1]=SgnCh(k,0); ++j; if (j>U) j=1; };
       for (n=1;n<=(SubUml(U,Az,Ez)-RtGb);++n)  { Sx[j-1]=SgnCh(k,1); ++j; if (j>U) j=1; };
       for (n=1;n<=Gb;++n) { Sx[j-1]=SgnCh(k,2); ++j; if (j>U) j=1; };
       n=Ez+1;
       if (n>U) n=1; if (j>U) j=1;
       while (j!=n) { Sx[j-1]=SgnCh(k,3); ++j; if (j>U) j=1; };
      } else if ((Ez>=0) && (Ez<U)) {
       if (RtGb>0) {
        for (n=1;n<=U;++n) Sx[n-1]='#';
        j=Ez+1; if (j>U) j=1;
        for (n=1;n<=RtGb;++n) { Sx[j-1]=SgnCh(k,0); ++j; if (j>U) j=1; };
       } else Sx[Ez]='*';
      } else if ( ((Az+1)>=0) && (Az<U)) {
       if (Gb>0) {
        for (n=1;n<=U;++n) Sx[n-1]='-';
        j=Ez+1; if (j>U) j=1;
        for (n=1;n<=Gb;++n) { Sx[j-1]=SgnCh(k,2); ++j; if (j>U) j=1; };
       } else Sx[Az]='O';
      };
     };
     strcat(S,Sx); zeile(Memo,S);
    };
    zeile(Memo," ");
   };
  };
  free(Sx);
  free(S);
  return charcount;
}

/******************************************************************************/

char *MSUSAConvertLogEntry(char *Field, char *result)
{ int i,f;
  float x;
  char s[30];
  result[0]=0;
  switch (Field[0]) {
   case 0xF0:
    switch (Field[7]) {
     case 0: strcpy(s,"Mo"); break;
     case 1: strcpy(s,"Tu"); break;
     case 2: strcpy(s,"We"); break;
     case 3: strcpy(s,"Th"); break;
     case 4: strcpy(s,"Fr"); break;
     case 5: strcpy(s,"Sa"); break;
     case 6: strcpy(s,"Su"); break;
     default: strcpy(s,"??");  break;
    };
    sprintf(result,"Program %s %02x/%02x/%02x %02x:%02x:%02x   ",s,Field[5],Field[4],Field[6],
                        Field[3],Field[2],Field[1]);
    switch (Field[8]) {
     case 0: sprintf(s,"Q%d ",Field[9]); break;
     case 1: sprintf(s,"C%d ",Field[9]); break;
     case 2: sprintf(s,"R%d ",Field[9]); break;
     default: sprintf(s,"?%d ",Field[9]); break;
    };
    strcat(result,s);
    switch (Field[10]) {
     case '+': sprintf(s,"IN+ %02X%02X  ",Field[11],Field[12]); break;
     case 'E': sprintf(s,"MS %x.%.2x  ",Field[11],Field[12]); break;
     default: strcpy(s,"??? "); break;
    };
    strcat(result,s);
    sprintf(&result[strlen(result)],"  Grp[%d] Lgt[%d] Prg[%d]",Field[13],Field[14],Field[15]);
   break;

   case 0xF1: strcpy(result,"Stop-Command"); break;
   case 0xF2:
    strcpy(result,"Name: "); i=1;
    while ((i<=14) && (Field[i]!=0)) { f=strlen(result); result[f]=Field[i]; result[f+1]=0; ++i; };
   break;
   case 0xF3: strcpy(result,"Initiation"); break;
   case 0xFF: strcpy(result,"Power On"); break;
   case 0xFE: strcpy(result,"Reset"); break;
   case 0xFD: strcpy(result,"Synchronization error"); break;
   default:
    if (Field[3] & 0x80) {
     sprintf(result,"Power off %02x/%02x/%02x %02x:%02x:%02x",Field[4],Field[3] & 0x7F,Field[5],Field[2],Field[1],Field[0]);
    } else {
     sprintf(result,"%02X %02X:%02X:%02X ",Field[3],Field[2],Field[1],Field[0]);
     if (Field[6]<16) {
      sprintf(s,"%-8s ",USAModeTxt[Field[6]]); } else sprintf(s,"[%02X]    ",Field[6]);
      strcat(result,s);
//    if Field[8]<16 then result:=result+MSModeTxt(Field[8],USA) else result:=result+'['+hex(Field[8])+']   ';
     f=(Field[4]<<8)+Field[5];
     if (f & 0x8000) strcat(result,"&"); else strcat(result,"-");
     if (f & 0x4000) strcat(result,"C"); else strcat(result,"-");
     if (f & 0x2000) strcat(result,"P"); else strcat(result,"-");
     if (f & 0x1000) strcat(result,"S"); else strcat(result,"-");
     if (f &  0x800) strcat(result,"!"); else strcat(result,"-");
     if (f &  0x400) strcat(result,"N"); else strcat(result,"-");
     if (f &  0x200) strcat(result,"*"); else strcat(result,"-");
     if (f &  0x100) strcat(result,"Q"); else strcat(result,"-");
     if (f &   0x80) strcat(result,"T"); else strcat(result,"-");
     if (f &   0x40) strcat(result,"R"); else strcat(result,"-");
     if (f &   0x20) strcat(result,"Y"); else strcat(result,"-");
     if (f &   0x10) strcat(result,"G"); else strcat(result,"-");
     if (f &    0x8) strcat(result,"V"); else strcat(result,"-");
     if (f &    0x4) strcat(result,"r"); else strcat(result,"-");
     if (f &    0x2) strcat(result,"y"); else strcat(result,"-");
     if (f &    0x1) strcat(result,"g"); else strcat(result,"-");
     x=Field[8]/10.0;
//     __disable_interrupt();
      sprintf(s," %4.1fV",x);
//     __enable_interrupt();
     strcat(result,s);
     sprintf(s," Prg%2d",Field[9]); strcat(result,s);
     sprintf(s," [%02X %02X %02X",Field[7],Field[10],Field[11]); strcat(result,s);
     sprintf(s," %02X %02X %02X %02X]",Field[12],Field[13],Field[14],Field[15]); strcat(result,s);
    };
   break;
  };
//  strcat(result,"\r\n");
  return result;
}
//--------------------------------------------------------------------------------------

TMSUSA37Prg USAMulti;
TMSUSABauPrg Baustelle;
TMSUSAneuPrg BaustelleNeu;

int MSprgscript(unsigned char *Prg, char *Memo, int Aoffset, int Alaenge)
{ int Vers;
  TPrgTab *P;
	Memo[0]=0;
  if (!MSprgavail(Prg)) {
   strcat(Memo,"No program available !"); strcat(Memo,"\r\n");
	 return 0;
  } else {
	 Vers=(*(Prg+2)<<8)+(*(Prg+3));
   if (*Prg>=0xA0) {
		P=&USAMulti;
   } else if (Vers>=0x100) {
    P=&BaustelleNeu;
   } else {
		 P=&Baustelle;
   };
	 P->offset=Aoffset;
	 P->laenge=Alaenge;
   P->Tab=(unsigned char *)Prg;
   return P->Script(Memo);
  };
}

bool MSprgavail(unsigned char *Prg)
{ unsigned short crc;
  int Lng;
  if ((*Prg)>=0xA0) Lng=0x100; else Lng=(Prg[0]<<8)+Prg[1];
  if ((Lng<0x40) || (Lng>0x1000)) return false;
  crc=calc_memcrc(Prg,(unsigned char *)&Prg[Lng-3],0);
  crc=calc_crc(Prg[Lng-2],crc);
  crc=calc_crc(Prg[Lng-1],crc);
  return (crc==0);
}


