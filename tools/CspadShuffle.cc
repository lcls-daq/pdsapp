#include "pdsapp/tools/CspadShuffle.hh"

#include "pds/config/CsPadConfigType.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/cspad/ElementHeader.hh"
#include "pdsdata/cspad/ElementIterator.hh"
#include "pdsdata/cspad/ElementV2.hh"

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
              CsPad::ElementIterator iiter(cfg, *xtc);
              for(const Pds::CsPad::ElementHeader* hdr = iiter.next(); (hdr); hdr=iiter.next()) {
                unsigned iq = 1<<hdr->quad();
                if (qmask & iq) {
                  printf("%s. Found duplicate quad %d.\n",
                         DetInfo::name(static_cast<const DetInfo&>(xtc->src)),hdr->quad());
                  _fatalError = true;
                }
                qmask |= iq;
              }
            }
#endif

	    CsPad::ElementIterator iter(cfg, *xtc);

	    // Copy the xtc header
            uint32_t* pwrite = _pwrite;
	    xtc->contains = TypeId(TypeId::Id_CspadElement,CsPad::ElementV2::Version);
	    _write(xtc, sizeof(Xtc));

	    const CsPad::ElementHeader* hdr;
	    while( (hdr=iter.next()) ) {
	      _write(hdr,sizeof(*hdr));

	      unsigned smask = cfg.roiMask(hdr->quad());
	      unsigned id;
	      const CsPad::Section *s=0,*end=0;
	      while( (s=iter.next(id)) ) {
		if (smask&(1<<id))
		  _write(s,sizeof(*s));
		end = s;
	      }

	      //  Copy the quadrant trailer
	      _write(reinterpret_cast<const uint16_t*>(end+1)-2,2*sizeof(uint16_t));
	    }
            //  Update the extent of the container
            reinterpret_cast<Xtc*>(pwrite)->extent = (_pwrite-pwrite)*sizeof(uint32_t);
	    return;
	  }
      }
      case (TypeId::Id_CspadConfig) : {
	if (xtc->contains.version()==_CsPadConfigType.version()) {
	  _config.push_back(*reinterpret_cast<const CsPadConfigType*>(xtc->payload()));
	  _info  .push_back(info);
	}
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
