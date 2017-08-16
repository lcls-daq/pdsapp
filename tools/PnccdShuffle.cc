
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
#include "pdsdata/psddl/pnccd.ddl.h"
#include "PnccdShuffle.hh"

#include <vector>
#include <new>

using namespace Pds;

static std::vector<PNCCD::ConfigV2> _config;
static std::vector<Pds::DetInfo>    _info;

#define NPRINTMAX 32
static unsigned nprint=0;

static PNCCD::ImageQuadrant buffer[4];

class myLevelIter : public XtcIterator {
  public:
    enum {Stop, Continue};
    myLevelIter(Xtc* xtc, unsigned depth) : XtcIterator(xtc), _depth(depth) {}

    void process(const DetInfo& d, PNCCD::FrameV0* f, PNCCD::ConfigV2& cfg) {
      for (unsigned i=0;i<cfg.numLinks();i++) {
        _quads[i] = const_cast<uint16_t*>(f->data());
        f->shuffle(&buffer[f->elementId()]);
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
          if (xtc->contains.version()==0) {
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
