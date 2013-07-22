
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "pds/xtc/Datagram.hh"
#include "pds/pnccd/FrameV0.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/pnCCD/ConfigV2.hh"
#include "pdsdata/pnCCD/FrameV1.hh"
#include "PnccdFrameDetail.hh"
#include "PnccdShuffle.hh"

#include <vector>
#include <new>

static std::vector<PNCCD::ConfigV2> _config;
static std::vector<Pds::DetInfo>    _info;

#define NPRINTMAX 32
static unsigned nprint=0;

#define XA0(x)  (((x) & 0x000000000000ffffull))

#define XA1(x)  (((x) & 0x000000000000ffffull) << 16)

#define XA2(x)  (((x) & 0x000000000000ffffull) << 32)

#define XA3(x)  (((x) & 0x000000000000ffffull) << 48)

#define XB0(x)  (((x) & 0x00000000ffff0000ull) >> 16)

#define XB1(x)  (((x) & 0x00000000ffff0000ull))

#define XB2(x)  (((x) & 0x00000000ffff0000ull) << 16)

#define XB3(x)  (((x) & 0x00000000ffff0000ull) << 32)

#define XC0(x)  (((x) & 0x0000ffff00000000ull) >> 32)

#define XC1(x)  (((x) & 0x0000ffff00000000ull) >> 16)

#define XC2(x)  (((x) & 0x0000ffff00000000ull))

#define XC3(x)  (((x) & 0x0000ffff00000000ull) << 16)

#define XD0(x)  (((x) & 0xffff000000000000ull) >> 48)

#define XD1(x)  (((x) & 0xffff000000000000ull) >> 32)

#define XD2(x)  (((x) & 0xffff000000000000ull) >> 16)

#define XD3(x)  (((x) & 0xffff000000000000ull))

using namespace Pds;

/*
 * shuffle - copy and reorder an array of 16-bit values
 *
 * This routine copies data from an input buffer to an output buffer
 * while also reordering the data.
 * Input ordering: A1 B1 C1 D1 A2 B2 C2 D2 A3 B3 C3 D3...
 * Output ordering: A1 A2 A3... B1 B2 B3... C1 C2 C3... D1 D2 D3...
 *
 * INPUT PARAMETERS:
 *
 *   in - Array of 16-bit elements to be shuffled.
 *        Note the size restrictions below.
 *
 *   nelements - the number of 16-bit elements in the input array.
 *               (NOT the number of bytes, but half that).
 *               Must be a positive multiple of PNCCD::Camex::NumChan * 4.
 *
 * OUPUT PARAMETER:
 *
 *   out - Array of 16-bit elements where result of shuffle is stored.
 *         The caller is responsible for providing a valid pointer
 *         to an output array equal in size but not overlapping the
 *         input array.
 *
 * RETURNS: -1 on error, otherwise 0.
 */
int PnccdShuffle::shuffle(void *invoid, void *outvoid, unsigned int nelements)
{
  uint64_t *in0;
  uint64_t *out0;
  uint64_t *out1;
  uint64_t *out2;
  uint64_t *out3;
  unsigned ii, jj;
  uint16_t* in = (uint16_t*)invoid;
  uint16_t* out = (uint16_t*)outvoid;
  int width = PNCCD::Camex::NumChan * 4;

  if (!in || !out || (nelements < width) || (nelements % width)) {
    /* error */
    return -1;
  }

  // outer loop: shuffle camexes within a line
  for (jj = 0; jj < nelements / width; jj++) {
    in0 = (uint64_t *)in;
    out0 = (uint64_t *)out;
    out1 = (uint64_t *)(out + (width / 4));
    out2 = (uint64_t *)(out + (width / 2));
    out3 = (uint64_t *)(out + (width * 3 / 4));
    // inner loop: shuffle channels within a camex
    for (ii = 0; ii < width / 4; ii += 4) {
      *out0++ = XA0(in0[ii]) | XA1(in0[ii+1]) | XA2(in0[ii+2]) | XA3(in0[ii+3]);
      *out1++ = XB0(in0[ii]) | XB1(in0[ii+1]) | XB2(in0[ii+2]) | XB3(in0[ii+3]);
      *out2++ = XC0(in0[ii]) | XC1(in0[ii+1]) | XC2(in0[ii+2]) | XC3(in0[ii+3]);
      *out3++ = XD0(in0[ii]) | XD1(in0[ii+1]) | XD2(in0[ii+2]) | XD3(in0[ii+3]);
    }
    in += width;
    out += width;
  }
  /* OK */
  return 0;
}

static PNCCD::ImageQuadrant buffer[4];

class myLevelIter : public XtcIterator {
  public:
    enum {Stop, Continue};
    myLevelIter(Xtc* xtc, unsigned depth) : XtcIterator(xtc), _depth(depth) {}

    void process(const DetInfo& d, PNCCD::FrameV0* f, PNCCD::ConfigV2& cfg) {
      for (unsigned i=0;i<cfg.numLinks();i++) {
        _quads[i] = f->data();
        PnccdShuffle::shuffle(
            (f->data()),
            &buffer[f->elementId()],
            sizeof(PNCCD::ImageQuadrant)/sizeof(uint16_t));
        f->convertThisToFrameV1();
        f = f->next(cfg);
      }
      for (unsigned i=0;i<cfg.numLinks();i++) {
        memcpy(_quads[i], &buffer[i].line[0].cmx[0].data[0], sizeof(PNCCD::ImageQuadrant));
      }
      new(&(this->_xtc->contains))Pds::TypeId(TypeId::Id_pnCCDframe, 1);
    }

    void process(const DetInfo& info, PNCCD::ConfigV2& config) {
      _config.push_back(config);
      _info  .push_back(info);
      printf("*** Processing pnCCD config.  Number of Links: %d, PayloadSize per Link: %d\n",
          config.numLinks(),config.payloadSizePerLink());
    }

    int process(Xtc* xtc) {
      _xtc = xtc;
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
          for (unsigned k=0; k<_info.size(); k++)
            if (_info[k] == info) {
              PNCCD::ConfigV2& cfg = _config[k];
              int expected = cfg.numLinks()*(sizeof(PNCCD::ImageQuadrant)+sizeof(PNCCD::FrameV0));
              if (xtc->sizeofPayload()==expected) {
                process(info, (PNCCD::FrameV0*)(xtc->payload()), cfg);
              } else {
                if (nprint++ < NPRINTMAX) {
                  printf("*** Error: no reordering.  Found payloadsize 0x%x, expected 0x%x\n",
                      xtc->sizeofPayload(),expected);
                }
              }
              break;
            }
          break;
        }
        case (TypeId::Id_pnCCDconfig) : {
          process(info, *(PNCCD::ConfigV2*)(xtc->payload()));
          break;
        }
        default :
          break;
      }
      return Continue;
    }
  private:
    unsigned    _depth;
    Xtc*        _xtc;
    uint16_t*   _quads[4];
};

void PnccdShuffle::shuffle(Datagram& dg) 
{
  if (dg.seq.service() == TransitionId::Configure) {
    _config.clear();
    _info  .clear();
    myLevelIter iter(&(dg.xtc),0);
    iter.iterate();
  }
  else if (!_config.empty() && dg.seq.service() == TransitionId::L1Accept) {
    myLevelIter iter(&(dg.xtc),0);
    iter.iterate();
  }
}
