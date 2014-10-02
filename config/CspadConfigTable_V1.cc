#include "pdsapp/config/CspadConfigTable_V1.hh"
#include "pdsdata/psddl/cspad.ddl.h"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

static const int PolarityGroup = 100;

static const unsigned ASICS   = Pds::CsPad::ASICsPerQuad;
static const unsigned Columns = Pds::CsPad::ColumnsPerASIC;
static const unsigned Rows    = Pds::CsPad::MaxRowsPerASIC;

namespace Pds_ConfigDb
{
  namespace V1 {
    static const char* RunModeText [] = { "NoRunning", "RunButDrop", "RunAndSendToRCE", "RunAndSendTriggeredByTTL", "ExternalTriggerSendToRCE", "ExternalTriggerDrop", NULL };
    //    static const char* DataModeText[] = { "Normal", "ShiftTest", "TestData", "Reserved", NULL };

    class GlobalP {
    public:
      GlobalP() :
	_runDelay        ( "Run Delay"     , 0,0, 0x7fffffff, Decimal),
	_eventCode       ( "Event Code"    , 40,0,       0xff, Decimal),
	_inactiveRunMode ( "Inact Run Mode", Pds::CsPad::RunButDrop, RunModeText),
	_activeRunMode   ( "Activ Run Mode", Pds::CsPad::RunAndSendTriggeredByTTL, RunModeText),
	_testDataIndex   ( "Test Data Indx", 4, 0, 7, Decimal ),
	_payloadPerQuad  ( "Quad Payload"  , sizeof(Pds::CsPad::ElementV1) + 
			   ASICS*Columns*Rows*sizeof(uint16_t) + 4,
			   0, 0x7fffffff, Decimal),
	_badAsicMask     ( "Bad ASIC Mask (hex)" , 0, 0, -1ULL, Hex),
	_AsicMask        ( "ASIC Mask (hex)"     , 0xf, 0, 0xf, Hex),
	_quadMask        ( "Quad Mask (hex)"     , 0xf, 0, 0xf, Hex)
      {}
    public:
      void enable(bool v) 
      {
      }
      void reset () 
      {
	_runDelay .value = 0; 
	_eventCode.value = 40; 
	_inactiveRunMode.value = Pds::CsPad::RunButDrop;
	_activeRunMode  .value = Pds::CsPad::RunAndSendTriggeredByTTL;
	_testDataIndex  .value = 4;
	_payloadPerQuad .value = sizeof(Pds::CsPad::ElementV1) + ASICS*Columns*Rows*sizeof(uint16_t) + 4;
	_badAsicMask    .value = 0;
	_AsicMask       .value = 0xf;
	_quadMask       .value = 0xf;
      }
      void pull   (const Pds::CsPad::ConfigV1& p) 
      {
	_runDelay .value = p.runDelay(); 
	_eventCode.value = p.eventCode(); 
	_inactiveRunMode.value = (Pds::CsPad::RunModes)p.inactiveRunMode();
	_activeRunMode  .value = (Pds::CsPad::RunModes)p.activeRunMode();
	_testDataIndex  .value = p.tdi();
	_payloadPerQuad .value = p.payloadSize();
	_badAsicMask    .value = (uint64_t(p.badAsicMask1())<<32) | p.badAsicMask0();
	_AsicMask       .value = p.asicMask();
	_quadMask       .value = p.quadMask();
      }
      void push   (Pds::CsPad::ConfigV1* p) 
      {
        Pds::CsPad::ConfigV1QuadReg dummy[4];
	*new (p) Pds::CsPad::ConfigV1( 0,
                                       _runDelay .value,
				       _eventCode.value,
				       _inactiveRunMode.value,
				       _activeRunMode  .value,
				       _testDataIndex  .value,
				       _payloadPerQuad .value,
				       _badAsicMask    .value & 0xffffffff,
				       _badAsicMask    .value >> 32,
				       _AsicMask       .value,
				       _quadMask       .value,
                                       dummy);
      }
    public:
      void initialize(QWidget* parent, QVBoxLayout* layout)
      {
	layout->addLayout(_runDelay       .initialize(parent));
	layout->addLayout(_eventCode      .initialize(parent));
	layout->addLayout(_inactiveRunMode.initialize(parent));
	layout->addLayout(_activeRunMode  .initialize(parent));
	layout->addLayout(_testDataIndex  .initialize(parent));
	layout->addLayout(_badAsicMask    .initialize(parent));
	layout->addLayout(_AsicMask       .initialize(parent));
	layout->addLayout(_quadMask       .initialize(parent));
      }

      void insert(Pds::LinkedList<Parameter>& pList) {
	pList.insert(&_runDelay);
	pList.insert(&_eventCode);
	pList.insert(&_inactiveRunMode);
	pList.insert(&_activeRunMode);
	pList.insert(&_testDataIndex);
	pList.insert(&_badAsicMask);
	pList.insert(&_AsicMask);
	pList.insert(&_quadMask);
      }

    public:
      NumericInt<unsigned>             _runDelay;
      NumericInt<unsigned>             _eventCode;
      Enumerated<Pds::CsPad::RunModes> _inactiveRunMode;
      Enumerated<Pds::CsPad::RunModes> _activeRunMode;
      NumericInt<unsigned>             _testDataIndex;
      NumericInt<unsigned>             _payloadPerQuad;
      NumericInt<uint64_t>             _badAsicMask;
      NumericInt<unsigned>             _AsicMask;
      NumericInt<unsigned>             _quadMask;
    };


    class QuadP {
    public:
      QuadP() :
	_shiftSelect     ( NULL, 4, 0, 0x7fffffff, Decimal),
	_edgeSelect      ( NULL, 0, 0, 0x7fffffff, Decimal),
	_readClkSet      ( NULL, 2, 0, 0x7fffffff, Decimal),
	_readClkHold     ( NULL, 1, 0, 0x7fffffff, Decimal),
	_dataMode        ( NULL, 0, 0, 0x7fffffff, Decimal),
	_prstSel         ( NULL, 1, 0, 0x7fffffff, Decimal),
	_acqDelay        ( NULL, 0x118, 0, 0x7fffffff, Decimal),
	_intTime         ( NULL, 0x5dc, 0, 0x7fffffff, Decimal),
	_digDelay        ( NULL, 0x3c0, 0, 0x7fffffff, Decimal),
	_ampIdle         ( NULL, 0, 0, 0x7fffffff, Decimal),
	_injTotal        ( NULL, 0, 0, 0x7fffffff, Decimal),
	_rowColShiftPer  ( NULL, 5, 0, 0x7fffffff, Decimal),
	// digital pots fields
	_vref            ( NULL, 0xba, 0, 0xff, Decimal),
	_vinj            ( NULL, 0xba, 0, 0xff, Decimal),
	_rampCurrR1      ( NULL, 0x04, 0, 0xff, Decimal),
	_rampCurrR2      ( NULL, 0x25, 0, 0xff, Decimal),
	_rampCurrRef     ( NULL, 0, 0, 0xff, Decimal),
	_rampVoltRef     ( NULL, 0x61, 0, 0xff, Decimal),
	_compBias1       ( NULL, 0xff, 0, 0xff, Decimal),
	_compBias2       ( NULL, 0xc8, 0, 0xff, Decimal),
	_iss2            ( NULL, 0x3c, 0, 0xff, Decimal),
	_iss5            ( NULL, 0x25, 0, 0xff, Decimal),
	_analogPrst      ( NULL, 0xfc, 0, 0xff, Decimal),
	// gain map field
	_gainMapValue    ( NULL, 1, 0, 1, Decimal)
      {
      }
    public:
      void enable(bool v) 
      {
      }
      void reset () 
      {
      }
      void pull   (const Pds::CsPad::ConfigV1QuadReg& p) 
      {
	for(int i=0; i<4; i++) {
	  _shiftSelect.value[i] = p.shiftSelect()[i];
	  _edgeSelect .value[i] = p.edgeSelect()[i];
	}
	_readClkSet      .value = p.readClkSet();
	_readClkHold     .value = p.readClkHold();
	_dataMode        .value = p.dataMode();
	_prstSel         .value = p.prstSel();
	_acqDelay        .value = p.acqDelay();
	_intTime         .value = p.intTime();
	_digDelay        .value = p.digDelay();
	_ampIdle         .value = p.ampIdle();
	_injTotal        .value = p.injTotal();
	_rowColShiftPer  .value = p.rowColShiftPer();

	const uint8_t* pots = p.dp().pots().data();
	_vref.value        = pots[0];
	_vinj.value        = pots[23];
	_analogPrst .value = pots[63];
	_rampCurrR1 .value = pots[2];
	_rampCurrR2 .value = pots[22];
	_rampCurrRef.value = pots[42];
	_rampVoltRef.value = pots[62];
	_compBias1  .value = pots[4];
	_compBias2  .value = pots[24];
	_iss2       .value = pots[44];
	_iss5       .value = pots[64];

	_gainMapValue.value = *reinterpret_cast<const uint16_t*>(p.gm().gainMap().data()) ? 1 : 0;
      }
      void push   (Pds::CsPad::ConfigV1QuadReg* p) 
      {
        Pds::CsPad::CsPadReadOnlyCfg dummy(-1U,-1U);

        uint32_t potscfg[(Pds::CsPad::PotsPerQuad+3)>>2];
	uint8_t* pots = reinterpret_cast<uint8_t*>(potscfg);
	*pots++ = _vref.value;
	*pots++ = _vref.value;
	*pots++ = _rampCurrR1.value;
	*pots++ = 0;
	for(uint8_t* end = pots+16; pots<end; pots++)
	  *pots = _compBias1.value;

	*pots++ = _vref.value;
	*pots++ = _vref.value;
	*pots++ = _rampCurrR2.value;
	*pots++ = _vinj.value;
	for(uint8_t* end = pots+16; pots<end; pots++)
	  *pots = _compBias2.value;

	*pots++ = _vref.value;
	*pots++ = _vref.value;
	*pots++ = _rampCurrRef.value;
	*pots++ = 0;
	for(uint8_t* end = pots+16; pots<end; pots++)
	  *pots = _iss2.value;

	*pots++ = _vref.value;
	*pots++ = _vref.value;
	*pots++ = _rampVoltRef.value;
	*pots++ = _analogPrst.value;
	for(uint8_t* end = pots+16; pots<end; pots++)
	  *pots = _iss5.value;

        const unsigned gsz = Pds::CsPad::ColumnsPerASIC*Pds::CsPad::MaxRowsPerASIC;
        uint16_t* gainMap = new uint16_t[gsz];
        memset(gainMap, _gainMapValue.value ? 0xff : 0, gsz*sizeof(uint16_t));

	*new(p) Pds::CsPad::ConfigV1QuadReg(_shiftSelect     .value,
					    _edgeSelect      .value,
					    _readClkSet      .value,
					    _readClkHold     .value,
					    _dataMode        .value,
					    _prstSel         .value,
					    _acqDelay        .value,
					    _intTime         .value,
					    _digDelay        .value,
					    _ampIdle         .value,
					    _injTotal        .value,
					    _rowColShiftPer  .value,
                                            dummy,
                                            reinterpret_cast<const Pds::CsPad::CsPadDigitalPotsCfg&>(potscfg),
                                            Pds::CsPad::CsPadGainMapCfg(gainMap)
                                            );

        delete[] gainMap;
      }
    public:
      static void layoutHeader(QGridLayout* layout)
      {
	layout->addWidget(new QLabel("Quad"), 0, 0, ::Qt::AlignHCenter);
	for(unsigned q=0; q<4; q++)
	  layout->addWidget(new QLabel(QString::number(q)), 0, q+1, ::Qt::AlignHCenter);

	unsigned col=0;
	unsigned row = 1;
#define ADDP(t) layout->addWidget(new QLabel(t), row++, col, ::Qt::AlignHCenter)
	layout->addWidget(new QLabel("Quad Registers"), row++, 1, 1, 4, ::Qt::AlignHCenter);
	ADDP("Shift Sel");
	ADDP("Edge Sel");
	ADDP("Read Clk Set");
	ADDP("Read Clk Hold");
	ADDP("Data Mode");
	ADDP("PRst Sel");
	ADDP("Acq Delay");
	ADDP("Int Time");
	ADDP("Dig Delay");
	ADDP("Amp Idle");
	ADDP("Inj Total");
	ADDP("Row/Col Shift");
	layout->addWidget(new QLabel("Digital Pots Fields"), row++, 1, 1, 4, ::Qt::AlignHCenter);
	ADDP("Vref");
	ADDP("Vin");
	ADDP("RampCurrR1");
	ADDP("RampCurrR2");
	ADDP("RampCurrRef");
	ADDP("RampVoltRef");
	ADDP("CompBias1");
	ADDP("CompBias2");
	ADDP("Iss2");
	ADDP("Iss5");
	ADDP("Analog Prst");
	layout->addWidget(new QLabel("GainMap Fields"), row++, 1, 1, 4, ::Qt::AlignHCenter);
	ADDP("Value");
#undef ADDP
      }

      void initialize(QWidget* parent, QGridLayout* layout, unsigned q)
      {
	unsigned col=q+1;
	unsigned row=2;
#define ADDP(t) layout->addLayout(t.initialize(parent), row++, col)
	ADDP(_shiftSelect);     _shiftSelect.setWidth(40);
	ADDP(_edgeSelect);      _edgeSelect .setWidth(40);
	ADDP(_readClkSet);
	ADDP(_readClkHold);
	ADDP(_dataMode);
	ADDP(_prstSel);
	ADDP(_acqDelay);
	ADDP(_intTime);
	ADDP(_digDelay);
	ADDP(_ampIdle);
	ADDP(_injTotal);
	ADDP(_rowColShiftPer);
	row++;
	ADDP(_vref);
	ADDP(_vinj);
	ADDP(_rampCurrR1);
	ADDP(_rampCurrR2);
	ADDP(_rampCurrRef);
	ADDP(_rampVoltRef);
	ADDP(_compBias1);
	ADDP(_compBias2);
	ADDP(_iss2);
	ADDP(_iss5);
	ADDP(_analogPrst);
	row++;
	ADDP(_gainMapValue);
#undef ADDP      
      }

      void insert(Pds::LinkedList<Parameter>& pList) {
#define ADDP(t) pList.insert(&t)
	ADDP(_shiftSelect);
	ADDP(_edgeSelect);
	ADDP(_readClkSet);
	ADDP(_readClkHold);
	ADDP(_dataMode);
	ADDP(_prstSel);
	ADDP(_acqDelay);
	ADDP(_intTime);
	ADDP(_digDelay);
	ADDP(_ampIdle);
	ADDP(_injTotal);
	ADDP(_rowColShiftPer);
	ADDP(_vref);
	ADDP(_vinj);
	ADDP(_rampCurrR1);
	ADDP(_rampCurrR2);
	ADDP(_rampCurrRef);
	ADDP(_rampVoltRef);
	ADDP(_compBias1);
	ADDP(_compBias2);
	ADDP(_iss2);
	ADDP(_iss5);
	ADDP(_analogPrst);
	ADDP(_gainMapValue);
#undef ADDP
      }

    public:
      NumericIntArray<uint32_t,4> _shiftSelect;
      NumericIntArray<uint32_t,4> _edgeSelect;
      NumericInt<uint32_t> _readClkSet;
      NumericInt<uint32_t> _readClkHold;
      NumericInt<uint32_t> _dataMode;
      NumericInt<uint32_t> _prstSel;
      NumericInt<uint32_t> _acqDelay;
      NumericInt<uint32_t> _intTime;
      NumericInt<uint32_t> _digDelay;
      NumericInt<uint32_t> _ampIdle;
      NumericInt<uint32_t> _injTotal;
      NumericInt<uint32_t> _rowColShiftPer;
      // digital pots fields
      NumericInt<uint8_t> _vref;
      NumericInt<uint8_t> _vinj;
      NumericInt<uint8_t> _rampCurrR1;
      NumericInt<uint8_t> _rampCurrR2;
      NumericInt<uint8_t> _rampCurrRef;
      NumericInt<uint8_t> _rampVoltRef;
      NumericInt<uint8_t> _compBias1;
      NumericInt<uint8_t> _compBias2;
      NumericInt<uint8_t> _iss2;
      NumericInt<uint8_t> _iss5;
      NumericInt<uint8_t> _analogPrst;
      // gain map field
      NumericInt<uint8_t> _gainMapValue;
    };
  };
};

using namespace Pds_ConfigDb;

CspadConfigTable_V1::CspadConfigTable_V1(const CspadConfig_V1& c) : 
  Parameter(NULL),
  _cfg(c)
{
  _globalP = new V1::GlobalP;
  for(unsigned q=0; q<4; q++)
    _quadP[q] = new V1::QuadP;
}

CspadConfigTable_V1::~CspadConfigTable_V1()
{
}

void CspadConfigTable_V1::insert(Pds::LinkedList<Parameter>& pList)
{
  pList.insert(this);
  
  _globalP->insert(_pList);
  for(unsigned q=0; q<4; q++)
    _quadP[q]->insert(_pList);
}

int CspadConfigTable_V1::pull(const Pds::CsPad::ConfigV1& tc) 
{
  _globalP->pull(tc);
  for(unsigned q=0; q<4; q++)
    _quadP[q]->pull(tc.quads(q));

  return sizeof(tc);
}

int CspadConfigTable_V1::push(void* to) const {

  Pds::CsPad::ConfigV1& tc = *reinterpret_cast<Pds::CsPad::ConfigV1*>(to);
  _globalP->push(&tc);

  for(unsigned q=0; q<4; q++)
    _quadP[q]->push(const_cast<Pds::CsPad::ConfigV1QuadReg*>(&(tc.quads(q))));

  return sizeof(tc);
}

int CspadConfigTable_V1::dataSize() const {
  return sizeof(Pds::CsPad::ConfigV1);
}

bool CspadConfigTable_V1::validate()
{
  return true;
}

QLayout* CspadConfigTable_V1::initialize(QWidget* parent) 
{
  QHBoxLayout* layout = new QHBoxLayout;
  { QVBoxLayout* gl = new QVBoxLayout;
    _globalP->initialize(parent,gl);
    layout->addLayout(gl); }
  layout->addSpacing(40);
  { QGridLayout* ql = new QGridLayout;
    V1::QuadP::layoutHeader(ql);
    for(unsigned q=0; q<4; q++)
      _quadP[q]->initialize(parent,ql,q);
    layout->addLayout(ql); }

  return layout;
}

void CspadConfigTable_V1::update() {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->update();
    p = p->forward();
  }
}

void CspadConfigTable_V1::flush () {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->flush();
    p = p->forward();
  }
}

void CspadConfigTable_V1::enable(bool) 
{
}

#include "Parameters.icc"
