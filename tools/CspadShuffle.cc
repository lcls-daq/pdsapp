#include "pdsapp/tools/CspadShuffle.hh"

#include "pds/config/CsPadConfigType.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/psddl/cspad.ddl.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <vector>

#define QUAD_CHECK

using namespace Pds;

static std::vector<CsPadConfigType> _config;
static std::vector<Pds::DetInfo>    _info;

//
//  Need an iterator that is safe for backwards copy
//
class myIter {
public:
  enum Status {Stop, Continue};
  myIter(Xtc* root, uint32_t*& pwrite) : 
    _root  (root), 
    _pwrite(pwrite),
    _fatalError(false) {}

private:
  void _write(const void* p, ssize_t sz) 
  {
    //    printf("_write %p %p %x\n",_pwrite,p,sz);

    const uint32_t* pread = (uint32_t*)p;
    if (_pwrite!=pread) {
      const uint32_t* const end = pread+(sz>>2);
      while(pread < end)
	*_pwrite++ = *pread++;
    }
    else
      _pwrite += sz>>2;
  }

public:
  void iterate() { iterate(_root); }
  bool fatalError() const { return _fatalError; }
private:
  void iterate(Xtc* root) {
    if (root->damage.value() & ( 1 << Damage::IncompleteContribution)) {
      return _write(root,root->extent);
    }

    int remaining = root->sizeofPayload();
    Xtc* xtc     = (Xtc*)root->payload();

    uint32_t* pwrite = _pwrite;
    _write(root, sizeof(Xtc));
    
    while(remaining > 0)
      {
	unsigned extent = xtc->extent;
	if(extent==0) {
	  printf("Breaking on zero extent\n");
	  break; // try to skip corrupt event
	}
	process(xtc);
	remaining -= extent;
	xtc        = (Xtc*)((char*)xtc+extent);
      }

    reinterpret_cast<Xtc*>(pwrite)->extent = (_pwrite-pwrite)*sizeof(uint32_t);
  }

  void process(Xtc* xtc) {
    const DetInfo& info = *(DetInfo*)(&xtc->src);
    Level::Type level = xtc->src.level();
    if (level < 0 || level >= Level::NumberOfLevels ) {
      printf("Unsupported Level %d\n", (int) level);
    }
    else {
      switch (xtc->contains.id()) {
      case (TypeId::Id_Xtc) : {
	myIter iter(xtc,_pwrite);
	iter.iterate();
	return;
      }
      case (TypeId::Id_CspadElement) : {
	if (xtc->damage.value()) break;
	if (xtc->contains.version()!=1) break;
	for(unsigned i=0; i<_info.size(); i++)
	  if (_info[i] == info) {

	    const CsPadConfigType& cfg = _config[i];

#ifdef QUAD_CHECK
            //  check for duplicate quads
            {
              unsigned qmask = 0;
              const Pds::CsPad::DataV2& data = *reinterpret_cast<const Pds::CsPad::DataV2*>(xtc->payload());
              for(int i=0; i<data.quads_shape(cfg)[0]; i++) {
                unsigned iq = 1<<data.quads(cfg,i).quad();
                if (qmask & iq) {
                  printf("%s. Found duplicate quad %d.\n",
                         DetInfo::name(static_cast<const DetInfo&>(xtc->src)),iq);
                  _fatalError = true;
                }
                qmask |= iq;
              }
            }
#endif

            const Pds::CsPad::DataV1& data = *reinterpret_cast<const Pds::CsPad::DataV1*>(xtc->payload());

	    // Copy the xtc header
            uint32_t* pwrite = _pwrite;
	    xtc->contains = TypeId(TypeId::Id_CspadElement,CsPad::DataV2::Version);
	    _write(xtc, sizeof(Xtc));

            for(int i=0; i<data.quads_shape(cfg)[0]; i++) {
              const CsPad::ElementV1& e = data.quads(cfg,i);

	      _write(&e,sizeof(e));

	      unsigned smask = cfg.roiMask(e.quad());
              ndarray<const int16_t,3> p = e.data(cfg);
              
	      for(unsigned id=0; id<p.shape()[0]; id++)
		if (smask&(1<<id))
		  _write(&p(id,0,0),p.strides()[0]*sizeof(int16_t));

	      //  Copy the quadrant trailer
	      _write(p.data()+p.size(),2*sizeof(uint16_t));
	    }
            //  Update the extent of the container
            reinterpret_cast<Xtc*>(pwrite)->extent = (_pwrite-pwrite)*sizeof(uint32_t);
	    return;
	  }
        printf("Found Cspad::ElementV1 from unexpected src %08x.%08x\n",info.log(),info.phy());
        break;
      }
      case (TypeId::Id_CspadConfig) : {
	if (xtc->contains.version()==_CsPadConfigType.version()) {
          printf("Caching Cspad::Configv%d\n",xtc->contains.version());
	  _config.push_back(*reinterpret_cast<const CsPadConfigType*>(xtc->payload()));
	  _info  .push_back(info);
	}
        else
          printf("Failed to cache Cspad::Configv%d\n",xtc->contains.version());
	break;
      }
      default :
	break;
      }
    }
    _write(xtc,xtc->extent);
  }
private:
  Xtc*       _root;
  uint32_t*& _pwrite;
  bool       _fatalError;
};

bool CspadShuffle::shuffle(Dgram& dg) 
{
  if (dg.seq.service() == TransitionId::Configure) {
    _config.clear();
    _info  .clear();
    uint32_t* pdg = reinterpret_cast<uint32_t*>(&(dg.xtc));
    myIter iter(&(dg.xtc),pdg);
    iter.iterate();
  }
  else if (!_config.empty() && dg.seq.service() == TransitionId::L1Accept) {
    uint32_t* pdg = reinterpret_cast<uint32_t*>(&(dg.xtc));
    myIter iter(&(dg.xtc),pdg);
    iter.iterate();
    if (iter.fatalError())
      return false;
  }
  return true;
}
