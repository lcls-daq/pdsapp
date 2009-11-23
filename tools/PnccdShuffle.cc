
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "pds/xtc/Datagram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/pnCCD/ConfigV1.hh"
#include "pdsdata/pnCCD/FrameV1.hh"
#include "PnccdFrameDetail.hh"
#include "PnccdShuffle.hh"

static PNCCD::ConfigV1 cfg;

#define NPRINTMAX 32
static unsigned nprint=0;

#define XA0(x)  ((((x) & 0x00000000000000ffull) <<  8) | \
                 (((x) & 0x000000000000ff00ull) >>  8))

#define XA1(x)  ((((x) & 0x00000000000000ffull) << 24) | \
                 (((x) & 0x000000000000ff00ull) <<  8))

#define XA2(x)  ((((x) & 0x00000000000000ffull) << 40) | \
                 (((x) & 0x000000000000ff00ull) << 24))

#define XA3(x)  ((((x) & 0x00000000000000ffull) << 56) | \
                 (((x) & 0x000000000000ff00ull) << 40))

#define XB0(x)  ((((x) & 0x0000000000ff0000ull) >>  8) | \
                 (((x) & 0x00000000ff000000ull) >> 24))

#define XB1(x)  ((((x) & 0x0000000000ff0000ull) <<  8) | \
                 (((x) & 0x00000000ff000000ull) >>  8))

#define XB2(x)  ((((x) & 0x0000000000ff0000ull) << 24) | \
                 (((x) & 0x00000000ff000000ull) <<  8))

#define XB3(x)  ((((x) & 0x0000000000ff0000ull) << 40) | \
                 (((x) & 0x00000000ff000000ull) << 24))

#define XC0(x)  ((((x) & 0x000000ff00000000ull) >> 24) | \
                 (((x) & 0x0000ff0000000000ull) >> 40))

#define XC1(x)  ((((x) & 0x000000ff00000000ull) >>  8) | \
                 (((x) & 0x0000ff0000000000ull) >> 24))

#define XC2(x)  ((((x) & 0x000000ff00000000ull) <<  8) | \
                 (((x) & 0x0000ff0000000000ull) >>  8))

#define XC3(x)  ((((x) & 0x000000ff00000000ull) << 24) | \
                 (((x) & 0x0000ff0000000000ull) <<  8))

#define XD0(x)  ((((x) & 0x00ff000000000000ull) >> 40) | \
                 (((x) & 0xff00000000000000ull) >> 56))

#define XD1(x)  ((((x) & 0x00ff000000000000ull) >> 24) | \
                 (((x) & 0xff00000000000000ull) >> 40))

#define XD2(x)  ((((x) & 0x00ff000000000000ull) >>  8) | \
                 (((x) & 0xff00000000000000ull) >> 24))

#define XD3(x)  ((((x) & 0x00ff000000000000ull) <<  8) | \
                 (((x) & 0xff00000000000000ull) >>  8))

static PNCCD::Line buffer;

using namespace Pds;

/*
 * shuffle - copy and reorder an array of 16-bit values
 *
 * This routine copies data from an input buffer to an output buffer
 * while also reording the data.
 * Input ordering: A1 B1 C1 D1 A2 B2 C2 D2 A3 B3 C3 D3...
 * Output ordering: 1A 2A 3A... 1B 2B 3B... 1C 2C 3C... 1D 2D 3D...
 *
 * INPUT PARAMETERS:
 *
 *   in - Array of 16-bit elements to be shuffled.
 *        Note the size restrictions below.
 *
 *   nelements - the number of 16-bit elements in the input array.
 *               (NOT the number of bytes, but half that).
 *               Must be a positive multiple of 4.
 *
 * OUPUT PARAMETER:
 *
 *   out - Array of 16-bit elements where result of shuffle is stored.
 *         The caller is resposible for providing a valid pointer
 *         to an output array equal in size but not overlapping the
 *         input array.
 *
 * RETURNS: -1 on error, otherwise 0.
 */
int PnccdShuffle::shuffle(void *invoid, void *outvoid, unsigned int nelements)
{
  uint16_t* in = (uint16_t*)invoid;
  uint16_t* out = (uint16_t*)outvoid;
  uint64_t *in0 = (uint64_t *)in;
  uint64_t *out0 = (uint64_t *)out;
  uint64_t *out1 = (uint64_t *)(out + (nelements / 4));
  uint64_t *out2 = (uint64_t *)(out + (nelements / 2));
  uint64_t *out3 = (uint64_t *)(out + (nelements * 3 / 4));
  unsigned ii;

  if (!in || !out || (nelements < 4) || (nelements % 4)) {
    /* error */
    return -1;
  }

  for (ii = 0; ii < nelements / 4; ii += 4) {
    *out0++ = XA0(in0[ii]) | XA1(in0[ii+1]) | XA2(in0[ii+2]) | XA3(in0[ii+3]);
    *out1++ = XB0(in0[ii]) | XB1(in0[ii+1]) | XB2(in0[ii+2]) | XB3(in0[ii+3]);
    *out2++ = XC0(in0[ii]) | XC1(in0[ii+1]) | XC2(in0[ii+2]) | XC3(in0[ii+3]);
    *out3++ = XD0(in0[ii]) | XD1(in0[ii+1]) | XD2(in0[ii+2]) | XD3(in0[ii+3]);
  }
  /* OK */
  return 0;
}

class myLevelIter : public XtcIterator {
public:
  enum {Stop, Continue};
  myLevelIter(Xtc* xtc, unsigned depth) : XtcIterator(xtc), _depth(depth) {}

  void process(const DetInfo& d, const PNCCD::FrameV1* f) {
    for (unsigned i=0;i<cfg.numLinks();i++) {
      PNCCD::Line* line = (PNCCD::Line*)(const_cast<uint16_t*>(f->data()));
      for (unsigned j=0;j<PNCCD::Image::NumLines;j++) {
        PnccdShuffle::shuffle(line,&buffer,sizeof(PNCCD::Line)/sizeof(uint16_t));
        memcpy(line,&buffer,sizeof(PNCCD::Line));
        line++;
      }
      f = f->next(cfg);
    }
  }

  void process(const DetInfo&, const PNCCD::ConfigV1& config) {
    cfg = config;
    printf("*** Processing pnCCD config.  Number of Links: %d, PayloadSize per Link: %d\n",
           cfg.numLinks(),cfg.payloadSizePerLink());
  }

  int process(Xtc* xtc) {
    const DetInfo& info = *(DetInfo*)(&xtc->src);
    Level::Type level = xtc->src.level();
    if (level < 0 || level >= Level::NumberOfLevels )
    {
        printf("Unsupported Level %d\n", (int) level);
        return Continue;
    }    
    switch (xtc->contains.id()) {
    case (TypeId::Id_Xtc) : {
      myLevelIter iter(xtc,_depth+1);
      iter.iterate();
      break;
    }
    case (TypeId::Id_pnCCDframe) : {
      // check size is correct before re-ordering
      int expected = cfg.numLinks()*(sizeof(PNCCD::Image)+sizeof(PNCCD::FrameV1));
      if (xtc->sizeofPayload()==expected) {
        process(info, (const PNCCD::FrameV1*)(xtc->payload()));
      } else {
        if (nprint++ < NPRINTMAX) {
          printf("*** Error: no reordering.  Found payloadsize 0x%x, expected 0x%x\n",
                 xtc->sizeofPayload(),sizeof(PNCCD::Image));
        }
      }
      break;
    }
    case (TypeId::Id_pnCCDconfig) : {
      process(info, *(const PNCCD::ConfigV1*)(xtc->payload()));
      break;
    }
    default :
      break;
    }
    return Continue;
  }
private:
  unsigned _depth;
};

void PnccdShuffle::shuffle(Datagram& dg) {
  myLevelIter iter(&(dg.xtc),0);
  iter.iterate();
}
