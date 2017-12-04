#include "pds/utility/Appliance.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pds/xtc/XtcType.hh"
#include "pdsdata/psddl/epix.ddl.h"

#include <math.h>
#include <fstream>
#include <sstream>

//#define DBUG

namespace Pds {
  class EpixWriter : public Appliance {
  public:
    EpixWriter();
    ~EpixWriter() {}
  public:
    Transition* transitions(Transition* tr) { return tr; }
    InDatagram* events(InDatagram*);
  public:
    const Src& src() const { return _src; }
    const Xtc& cfgtc() const { return *_cfgtc; }
    void fill_event(Xtc&) const;
  private:
    DetInfo _src;
    char*   _cfgpayload;
    Xtc*    _cfgtc;

    double _e0;
    double _sigE;
    double _i0;
    double _sigx;

    unsigned _rows;
    unsigned _columns;

    double _x0,_x1,_y0,_y1;

    bool _mask;
  };

  class EpixAdcData : public Xtc {
  public:
    EpixAdcData(const EpixWriter&);
  };

  class EpixAdcConfig : public Xtc {
  public:
    EpixAdcConfig(const EpixWriter&);
  };

};


using namespace Pds;

static TypeId _EpixConfigType(TypeId::Id_EpixConfig,1);
typedef Epix::ConfigV1 EpixConfigType;

static TypeId _EpixDataType  (TypeId::Id_EpixElement,1);
typedef Epix::ElementV1 EpixDataType;

static const unsigned _nx = 40;
static const unsigned _ny = 40;

static double _ctr[_ny][_nx];
static double _ex [_ny][_nx];
static double _ey [_ny][_nx];
static double _ez [_ny][_nx];

static double ranuni() { return double(random())/double(RAND_MAX); }
static double rangss() { 
  static double _gss;
  static bool _linit=false;
  double v;
  if (!_linit) {
    _linit=true;
    double gr = sqrt(-log(ranuni()));
    double ph = 2*M_PI*ranuni();
    _gss = gr*cos(ph);
    v = gr*sin(ph);
  }
  else {
    _linit=false;
    v = _gss;
  }
  return v;
}
Pds::EpixWriter::EpixWriter() :
  _src(getpid(), DetInfo::AmoEndstation, 0, DetInfo::Epix, 0)
{
  _e0   = 1000;
  _sigE = 10;
  _i0   = 0.001;
  _sigx = 0.25;
  _mask = false;
  _rows = 50;
  _columns = 192;
  _x0   = 0.5;
  _x1   = double(_columns)-0.5;
  _y0   = 0.5;
  _y1   = double(_rows)-0.5;

  char buff[256];
  sprintf(buff,"%s/epixwriter.cnf",getenv("HOME"));
  std::ifstream* in = new std::ifstream(buff);
  if (in) {
    printf("Opened %s\n",buff);
    std::string line;   
    while (getline(*in, line)) {
      std::istringstream ss(line);
      std::istream_iterator<std::string> begin(ss), end;
      std::vector<std::string> arrayTokens(begin, end);

      if (arrayTokens.size()<2 ||
	  arrayTokens[0][0]=='#') continue;

#define GETDPARM(n) if (arrayTokens[0] == #n) _##n = strtod (arrayTokens[1].c_str(),0);
#define GETUPARM(n) if (arrayTokens[0] == #n) _##n = strtoul(arrayTokens[1].c_str(),0,0);

      GETDPARM(e0)
      GETDPARM(sigE)
      GETDPARM(i0)
      GETDPARM(sigx)
      GETUPARM(rows)
      GETUPARM(columns)
      GETDPARM(x0)
      GETDPARM(y0)
      GETDPARM(x1)
      GETDPARM(y1)
	if (arrayTokens[0] == "mask")
	_mask = true;
    }
  }

  printf("\te0 %f\tsige %f\ti0 %f\tsigx %f\trows %d\tcolumns %d\t [%f,%f]x[%f,%f]\tmask %c\n",
	 _e0, _sigE, _i0, _sigx, _rows, _columns, _x0,_x1, _y0,_y1, _mask?'t':'f');

  const unsigned Rows = _rows;
  const unsigned Columns = _columns;
  const unsigned AsicsPerRow = 2;
  const unsigned AsicsPerColumn = 2;
  const unsigned Asics = AsicsPerRow*AsicsPerColumn;
  const unsigned CfgSize = sizeof(Epix::ConfigV1)
    +Asics*Epix::AsicConfigV1::_sizeof()          // AsicsConfigV1
    +2*Asics*Rows*((Columns+31)/32)*sizeof(uint32_t) // PixelArrays [Test,Mask]
    +sizeof(Xtc);

  _cfgpayload = new char[CfgSize];
  _cfgtc = new(_cfgpayload) Xtc(_EpixConfigType,_src);

  Epix::AsicConfigV1 asics[Asics];
  uint32_t* testarray = new uint32_t[Asics*Rows*(Columns+31)/32];
  memset(testarray, 0, Asics*Rows*(Columns+31)/32*sizeof(uint32_t));
  uint32_t* maskarray = new uint32_t[Asics*Rows*(Columns+31)/32];
  memset(maskarray, 0, Asics*Rows*(Columns+31)/32*sizeof(uint32_t));
  unsigned asicMask = 0xf;

  EpixConfigType* cfg = 
    new (_cfgtc->next()) EpixConfigType( 0, 1, 0, 1, 0, // version
					 0, 0, 0, 0, 0, // asicAcq
					 0, 0, 0, 0, 0, // asicGRControl
					 0, 0, 0, 0, 0, // asicR0Control
					 0, 0, 0, 0, 0, // asicR0ToAsicAcq
					 1, 0, 0, 0, 0, // adcClkHalfT
					 0, 0, 0, 0, 0, // digitalCardId0
					 AsicsPerRow, AsicsPerColumn, 
					 Rows, Columns, 
					 0x200000, // 200MHz
					 asicMask,
					 asics, testarray, maskarray );
					 
  _cfgtc->alloc(cfg->_sizeof());

  for(unsigned i=0; i<_ny; i++) {
    double y = (double(i)+0.5)/(double(_ny)*2);
    for(unsigned j=0; j<_nx; j++) {
      double x = (double(j)+0.5)/(double(_nx)*2);
      _ctr[i][j] = 0.25*
	(erf((0.5-x)/_sigx) + erf((0.5+x)/_sigx))*
	(erf((0.5-y)/_sigx) + erf((0.5+y)/_sigx));
      _ex [i][j] = 0.25*
	(erf((1.5-x)/_sigx) - erf((0.5-x)/_sigx))*
	(erf((0.5-y)/_sigx) + erf((0.5+y)/_sigx));
      _ey [i][j] = 0.25*
	(erf((0.5-x)/_sigx) + erf((0.5+x)/_sigx))*
	(erf((1.5-y)/_sigx) - erf((0.5-y)/_sigx));
      _ez [i][j] = 0.25*
	(erf((1.5-x)/_sigx) - erf((0.5-x)/_sigx))*
	(erf((1.5-y)/_sigx) - erf((0.5-y)/_sigx));
#ifdef DBUG
      printf(" (%d,%d)\t%f\t%f\t%f\t%f\n",
	     i,j,_ctr[i][j],_ex[i][j],_ey[i][j],_ez[i][j]);
#endif
    }
  }
}

InDatagram* Pds::EpixWriter::events(InDatagram* dg)
{
  switch (dg->seq.service()) {
  case TransitionId::L1Accept:
    dg->xtc.extent += (new (&dg->xtc) EpixAdcData(*this))->extent;
    break;
  case TransitionId::Configure:
    dg->xtc.extent += (new (&dg->xtc) EpixAdcConfig(*this))->extent;
    break;
  default:
    break;
  }
  return dg;
}

EpixAdcConfig::EpixAdcConfig(const EpixWriter& w) : Xtc(_EpixConfigType,w.src())
{
  const Xtc& cx = w.cfgtc();
  memcpy(alloc(cx.sizeofPayload()), cx.payload(), cx.sizeofPayload());
}

EpixAdcData::EpixAdcData(const EpixWriter& w) : Xtc(_EpixDataType,w.src())
{
  w.fill_event(*this);
}

void EpixWriter::fill_event(Xtc& xtc) const
{
  const EpixConfigType& cfg = *reinterpret_cast<const EpixConfigType*>(_cfgtc->payload());
  EpixDataType& d = *reinterpret_cast<EpixDataType*>(xtc.alloc((EpixDataType::_sizeof(cfg)+3)&~3));

  ndarray<const uint16_t,2> f = d.frame(cfg);

  unsigned nrows = cfg.numberOfRows();
  unsigned ncols = cfg.numberOfColumns();

  ndarray<double,2> q = make_ndarray<double>(nrows,ncols);
  for(double* p=q.begin(); p<q.end(); *p++=0.);
  
  int ngamma;
  if (_i0>0) {
    double iavg = _i0*(_x1-_x0)*(_y1-_y0);
    ngamma = int(iavg+sqrt(iavg)*rangss());
  }
  else
    ngamma = int(-_i0);

#ifdef DBUG
  printf("\n\tngamma %d [%dx%d] [%dx%d]\n",ngamma,nrows,ncols,f.shape()[0],f.shape()[1]);
#endif
  for(int i=0; i<ngamma; i++) {
    double x = _x0 + (_x1-_x0)*ranuni();
    double y = _y0 + (_y1-_y0)*ranuni();

    if (_mask && drem( y + 0.1*x, 20) < 0)
      continue; 

    int ix = int(x);
    int iy = int(y);

    double dx = (x-double(ix)-0.5);
    double dy = (y-double(iy)-0.5);

    unsigned bx = unsigned(fabs(2*dx*_nx));
    unsigned by = unsigned(fabs(2*dy*_ny));

    int idx = (dx>0) ? ix+1:ix-1;
    int idy = (dy>0) ? iy+1:iy-1;
    
    if (ix<0 || ix>=int(ncols) ||
	iy<0 || iy>=int(nrows) ||
	idx<0 || idx>=int(ncols) ||
	idy<0 || idy>=int(nrows) ||
	bx>=_nx || by>=_ny)
      printf("\n == [%f,%f] [%d,%d] [%d,%d] [%d,%d] == \n",
	     x,y,ix,iy,bx,by,idx,idy);

    q(iy ,ix)   += _e0*_ctr[by][bx];
    q(idy,ix)   += _e0*_ey [by][bx];
    q(iy ,idx)  += _e0*_ex [by][bx];
    q(idy,idx)  += _e0*_ez [by][bx];

#ifdef DBUG
    printf(" (%.2f,%.2f)%c",x,y,(i%6)==5 ?'\n':' ');
#endif
  }

  const double doffset = double(0x1000);
  for(unsigned i=0; i<nrows; i++) {
    uint16_t* pf = const_cast<uint16_t*>(&f(i,0));
    for(unsigned j=0; j<ncols; j++) {
      pf[j] = unsigned(q(i,j) + doffset + _sigE*rangss());
    }
  }

#ifdef DBUG
  for(unsigned i=0; i<nrows; i++) {
    for(unsigned j=0; j<ncols; j++)
      printf(" %04x",f[i][j]);
    printf("\n");
  }
#endif  
}

extern "C" Appliance* create() { return new EpixWriter; }
