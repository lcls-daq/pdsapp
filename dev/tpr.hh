#ifndef PDSTPR_HH
#define PDSTPR_HH

#include "evgr/evgr/evr/evr.hh"
#include <unistd.h>
#include <string>

typedef struct { 
  uint32_t  maxSize;
  uint32_t* data; } EvrRxDesc;

class AxiVersion {
public:
  std::string buildStamp() const;
public:
  volatile uint32_t FpgaVersion; 
  volatile uint32_t ScratchPad; 
  volatile uint32_t DeviceDnaHigh; 
  volatile uint32_t DeviceDnaLow; 
  volatile uint32_t FdSerialHigh; 
  volatile uint32_t FdSerialLow; 
  volatile uint32_t MasterReset; 
  volatile uint32_t FpgaReload; 
  volatile uint32_t FpgaReloadAddress; 
  volatile uint32_t Counter; 
  volatile uint32_t FpgaReloadHalt; 
  volatile uint32_t reserved_11[0x100-11];
  volatile uint32_t UserConstants[64];
  volatile uint32_t reserved_0x140[0x200-0x140];
  volatile uint32_t BuildStamp[64];
  volatile uint32_t reserved_0x240[0x4000-0x240];
};

class XBar {
public:
  enum InMode  { StraightIn , LoopIn };
  enum OutMode { StraightOut, LoopOut };
  void setEvr( InMode  m );
  void setEvr( OutMode m );
  void setTpr( InMode  m );
  void setTpr( OutMode m );
  void dump() const;
public:
  volatile uint32_t outMap[4];
};

// Memory map of EVR registers (EvrCardG2 BAR 0)
class EvrReg {
public:
  Pds::Evr evr;
  volatile uint32_t reserved_0x6000[(0x10000-sizeof(Pds::Evr))/4];
  AxiVersion version;
  volatile uint32_t reserved_axiversion[(0x20000-0x10000-sizeof(AxiVersion))/4];
  volatile uint32_t reserved_0x20000[(0x30000-0x20000)/4];
  volatile uint32_t reserved_0x30000[(0x40000-0x30000)/4];
  XBar xbar;
};

class TprBase {
public:
  enum { NCHANNELS=12 };
  enum { NTRIGGERS=12 };
  enum Destination { Any };
  enum FixedRate { _1M, _500K, _100K, _10K, _1K, _100H, _10H, _1H };
public:
  void dump() const;
  void setupDaq    (unsigned i,
                    unsigned partition);
  void setupChannel(unsigned i,
                    Destination d,
                    FixedRate   r,
                    unsigned    bsaPresample,
                    unsigned    bsaDelay,
                    unsigned    bsaWidth);
  void setupTrigger(unsigned i,
                    unsigned source,
                    unsigned polarity,
                    unsigned delay,
                    unsigned width);
public:
  volatile uint32_t status;
  volatile uint32_t countRxClk;
  volatile uint32_t countTxClk;
  volatile uint32_t gtxDebug;
  volatile uint32_t countReset;
  volatile uint32_t trigMaster;
  volatile uint32_t reserved_18[2];
  struct {  // 0x20
    volatile uint32_t control;
    volatile uint32_t evtSel;
    volatile uint32_t evtCount;
    volatile uint32_t bsaDelay;
    volatile uint32_t bsaWidth;
    volatile uint32_t bsaCount;
    volatile uint32_t bsaData;
    volatile uint32_t reserved[1];
  } channel[NCHANNELS];
  volatile uint32_t reserved_20[2];
  volatile uint32_t frameCount;
  volatile uint32_t reserved_2C[2];
  volatile uint32_t bsaCntlCount;
  volatile uint32_t bsaCntlData;
  volatile uint32_t reserved_b[1+(14-NCHANNELS)*4];
  struct { // 0x200
    volatile uint32_t control; // input, polarity, enabled
    volatile uint32_t delay;
    volatile uint32_t width;
    volatile uint32_t reserved_t;
  } trigger[NTRIGGERS];
};

class DmaControl {
public:
  void dump() const;
  void test();
public:
  volatile uint32_t rxFree;
  volatile uint32_t reserved_4[15];
  volatile uint32_t rxFreeStat;
  volatile uint32_t reserved_14[47];
  volatile uint32_t rxMaxFrame;
  volatile uint32_t rxFifoSize;
  volatile uint32_t rxCount;
  volatile uint32_t lastDesc;
};

class TprCore {
public:
  void rxPolarity (bool p);
  void resetCounts();
  void dump() const;
public:
  volatile uint32_t SOFcounts;
  volatile uint32_t EOFcounts;
  volatile uint32_t Msgcounts;
  volatile uint32_t CRCerrors;
  volatile uint32_t RxRecClks;
  volatile uint32_t RxRstDone;
  volatile uint32_t RxDecErrs;
  volatile uint32_t RxDspErrs;
  volatile uint32_t CSR;
};

class RingB {
public:
  void enable(bool l);
  void clear ();
  void dump() const;
  void dumpFrames() const;
public:
  volatile uint32_t data[0x1fff];
  volatile uint32_t csr;
};

class TpgMini {
public:
  void setBsa(unsigned rate,
	      unsigned ntoavg, unsigned navg);
  void dump() const;
public:
  volatile uint32_t ClkSel;
  volatile uint32_t BaseCntl;
  volatile uint32_t PulseIdU;
  volatile uint32_t PulseIdL;
  volatile uint32_t TStampU;
  volatile uint32_t TStampL;
  volatile uint32_t FixedRate[10];
  volatile uint32_t RateReload;
  volatile uint32_t HistoryCntl;
  volatile uint32_t FwVersion;
  volatile uint32_t Resources;
  volatile uint32_t BsaCompleteU;
  volatile uint32_t BsaCompleteL;
  volatile uint32_t reserved_22[128-22];
  struct {
    volatile uint32_t l;
    volatile uint32_t h;
  } BsaDef[64];
  volatile uint32_t reserved_256[320-256];
  volatile uint32_t CntPLL;
  volatile uint32_t Cnt186M;
  volatile uint32_t reserved_322;
  volatile uint32_t CntIntvl;
  volatile uint32_t CntBRT;
};

#endif
