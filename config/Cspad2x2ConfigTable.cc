#include "pdsapp/config/Cspad2x2ConfigTable.hh"
#include "pdsapp/config/Cspad2x2Sector.hh"
#include "pdsapp/config/Cspad2x2GainMap.hh"
#include "pdsdata/cspad2x2/ElementV1.hh"
#include "pdsdata/cspad2x2/Detector.hh"
#include "pds/config/CsPad2x2ConfigType.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QtGui/QMessageBox>

static const int PolarityGroup = 100;

static const unsigned ASICS   = Pds::CsPad2x2::ASICsPerQuad;
static const unsigned Columns = Pds::CsPad2x2::ColumnsPerASIC;
static const unsigned Rows    = Pds::CsPad2x2::MaxRowsPerASIC;

namespace Pds_ConfigDb
{
  static const char* RunModeText [] = { "NoRunning", "RunButDrop", "RunAndSendToServer", "RunAndSendTriggeredByTTL", "ExternalTriggerSendToServer", "ExternalTriggerDrop", NULL };
  //  static const char* DataModeText[] = { "Normal", "ShiftTest", "TestData", "Reserved", NULL };

  class GlobalP2x2 {
    public:
      GlobalP2x2() :
        _inactiveRunMode ( "Inact Run Mode", Pds::CsPad2x2::RunButDrop, RunModeText),
        _activeRunMode   ( "Activ Run Mode", Pds::CsPad2x2::RunAndSendTriggeredByTTL, RunModeText),
        _testDataIndex   ( "Test Data Indx", 4, 0, 7, Decimal ),
        _badAsicMask     ( "Bad ASIC Mask (hex)" , 0, 0, 15, Hex),
        _sectors         ( "Sector Mask (hex)"   , 0x3, 0, 0x3, Hex),
        _protQ0AdcThr      ("ADC Threshold",  67,   0, 0x3fff, Decimal),
        _protQ0PixelThr    ("Pixel Count Threshold",  1200, 0, 574564,  Decimal)
      {}
    public:
      void enable(bool v)
      {
      }
      void reset ()
      {
        _inactiveRunMode.value = Pds::CsPad2x2::RunButDrop;
        _activeRunMode  .value = Pds::CsPad2x2::RunAndSendTriggeredByTTL;
        _testDataIndex  .value = 4;
        _badAsicMask    .value = 0;
        _sectors        .value = 0x3;
        _protQ0AdcThr  .value = 67;
        _protQ0PixelThr .value = 1200;
      }
      void pull   (const CsPad2x2ConfigType& p)
      {
        _inactiveRunMode.value = (Pds::CsPad2x2::RunModes)p.inactiveRunMode();
        _activeRunMode  .value = (Pds::CsPad2x2::RunModes)p.activeRunMode();
        _testDataIndex  .value = p.tdi();
        _badAsicMask    .value = p.badAsicMask();
        _sectors        .value = p.roiMask(0);
        _protQ0AdcThr   .value = p.protectionThreshold()->adcThreshold;
        _protQ0PixelThr .value = p.protectionThreshold()->pixelCountThreshold;
        update_readout();
      }
      void push   (CsPad2x2ConfigType* p)
      {

        *new (p) CsPad2x2ConfigType(
             _inactiveRunMode.value,
             _activeRunMode  .value,
             _testDataIndex  .value,
             sizeof(Pds::CsPad2x2::ElementV1) + sizeof(uint32_t),  // space for the last word
            _badAsicMask    .value,
            1,
            _sectors.value);
        p->protectionEnable(1);
        p->protectionThreshold()->adcThreshold = _protQ0AdcThr.value;
        p->protectionThreshold()->pixelCountThreshold = _protQ0PixelThr.value;
      }
    public:
      void initialize(QWidget* parent, QVBoxLayout* layout)
      {
        layout->addLayout(_inactiveRunMode.initialize(parent));
        layout->addLayout(_activeRunMode  .initialize(parent));
        layout->addLayout(_testDataIndex  .initialize(parent));
        layout->addLayout(_protQ0AdcThr  .initialize(parent));
        layout->addLayout(_protQ0PixelThr .initialize(parent));
         layout->addLayout(_badAsicMask    .initialize(parent));
        layout->addLayout(_sectors        .initialize(parent));

        QGridLayout* gl = new QGridLayout;
        gl->addWidget(_roiCanvas[0] = new Cspad2x2Sector(*_sectors._input,0),0,0,::Qt::AlignTop|::Qt::AlignHCenter);
         layout->addLayout(gl);

        _qlink = new Cspad2x2ConfigTableQ(*this,parent);
        if (Parameter::allowEdit())
          ::QObject::connect(_sectors._input, SIGNAL(editingFinished()), _qlink, SLOT(update_readout()));

        update_readout();
      }

      void insert(Pds::LinkedList<Parameter>& pList) {
        pList.insert(&_inactiveRunMode);
        pList.insert(&_activeRunMode);
        pList.insert(&_testDataIndex);
        pList.insert(&_badAsicMask);
        pList.insert(&_sectors);
        pList.insert(&_protQ0AdcThr);
        pList.insert(&_protQ0PixelThr);
      }

      void update_readout() {
        unsigned m = _sectors.value;
        _roiCanvas[0]->update(m);
      }

    public:
      Enumerated<Pds::CsPad2x2::RunModes> _inactiveRunMode;
      Enumerated<Pds::CsPad2x2::RunModes> _activeRunMode;
      NumericInt<unsigned>             _testDataIndex;
      NumericInt<unsigned>             _badAsicMask;
      NumericInt<unsigned>             _sectors;
      NumericInt<unsigned>             _protQ0AdcThr;
      NumericInt<unsigned>             _protQ0PixelThr;
      Cspad2x2Sector*                     _roiCanvas[1];
      Cspad2x2ConfigTableQ*               _qlink;
  };


  class QuadP2x2 {
    public:
      QuadP2x2() :
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
        _ampReset        ( NULL, 0, 0, 1, Decimal),
        _digCount        ( NULL, 0x3fff, 0, 0x3fff, Hex),
        _digPeriod       ( NULL, 0xc, 0, 0xff, Hex),
        _PeltierEnable   ( NULL, 0, 0, 1, Decimal),
        _kpConstant      ( NULL, 100, 0, 0xfff, Decimal),
        _kiConstant      ( NULL, 0, 0, 0xf, Decimal),
        _kdConstant      ( NULL, 0, 0, 0xf, Decimal),
        _humidThold      ( NULL, 0, 0, 0xfff, Decimal),
        _setPoint        ( NULL, 2026, 0, 0xfff, Decimal),
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
        _analogPrst      ( NULL, 0xfc, 0, 0xff, Decimal)
      {
      }
    public:
      void enable(bool v)
      {
      }
      void reset ()
      {
      }
      void pull   (const Pds::CsPad2x2::ConfigV1QuadReg& p, Pds::CsPad2x2::CsPad2x2GainMapCfg* gm)
      {
        for(int i=0; i<Pds::CsPad2x2::TwoByTwosPerQuad; i++) {
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
        _ampReset        .value = p.ampReset();
        _digCount        .value = p.digCount();
        _digPeriod       .value = p.digPeriod();
        _PeltierEnable   .value = p.PeltierEnable();
        _kpConstant      .value = p.kpConstant();
        _kiConstant      .value = p.kiConstant();
        _kdConstant      .value = p.kdConstant();
        _humidThold      .value = p.humidThold();
        _setPoint        .value = p.setPoint();

        const uint8_t* pots = &p.dp().pots[0];
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

        memcpy(gm, p.gm(), sizeof(*p.gm()));
      }
      void push   (Pds::CsPad2x2::ConfigV1QuadReg* p, Pds::CsPad2x2::CsPad2x2GainMapCfg* gm)
      {
        *new(p) Pds::CsPad2x2::ConfigV1QuadReg(
            _shiftSelect     .value,
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
            _ampReset        .value,
            _digCount        .value,
            _digPeriod       .value,
            _PeltierEnable   .value,
            _kpConstant      .value,
            _kiConstant      .value,
            _kdConstant      .value,
            _humidThold      .value,
            _setPoint        .value);

        uint8_t* pots = &p->dp().pots[0];
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

        memcpy(p->gm(), gm, sizeof(*p->gm()));
      }
    public:
      static void layoutHeader(QGridLayout* layout)
      {
//        layout->addWidget(new QLabel("Quad"), 0, 0, ::Qt::AlignHCenter);
//          layout->addWidget(new QLabel(QString::number(0)), 0, 1, ::Qt::AlignHCenter);

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
        ADDP("Amp Reset");
        ADDP("Dig Count (hex)");
        ADDP("Dig Period (hex)");
        ADDP("Peltier Enable");
        ADDP("kpConstant");
        ADDP("kiConstant");
        ADDP("kdConstant");
        ADDP("humidThold");
        ADDP("setPoint");
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
#undef ADDP
      }

      void initialize(QWidget* parent, QGridLayout* layout)
      {
        unsigned col=1;
        unsigned row=2;
#define ADDP(t) layout->addLayout(t.initialize(parent), row++, col)
        ADDP(_shiftSelect);  //   _shiftSelect.setWidth(40);
        ADDP(_edgeSelect);   //   _edgeSelect .setWidth(40);
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
        ADDP(_ampReset);
        ADDP(_digCount);
        ADDP(_digPeriod);
        ADDP(_PeltierEnable);
        ADDP(_kpConstant);
        ADDP(_kiConstant);
        ADDP(_kdConstant);
        ADDP(_humidThold);
        ADDP(_setPoint);
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
        ADDP(_ampReset);
        ADDP(_digCount);
        ADDP(_digPeriod);
        ADDP(_PeltierEnable);
        ADDP(_kpConstant);
        ADDP(_kiConstant);
        ADDP(_kdConstant);
        ADDP(_humidThold);
        ADDP(_setPoint);
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
#undef ADDP
      }

    public:
      NumericIntArray<uint32_t,Pds::CsPad2x2::TwoByTwosPerQuad> _shiftSelect;
      NumericIntArray<uint32_t,Pds::CsPad2x2::TwoByTwosPerQuad> _edgeSelect;
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
      NumericInt<uint32_t> _ampReset;
      NumericInt<uint32_t> _digCount;
      NumericInt<uint32_t> _digPeriod;
      NumericInt<uint32_t> _PeltierEnable;
      NumericInt<uint32_t> _kpConstant;
      NumericInt<uint32_t> _kiConstant;
      NumericInt<uint32_t> _kdConstant;
      NumericInt<uint32_t> _humidThold;
      NumericInt<uint32_t> _setPoint;
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
  };
};

using namespace Pds_ConfigDb;

Cspad2x2ConfigTable::Cspad2x2ConfigTable(const Cspad2x2Config& c) :
      Parameter(NULL),
      _cfg(c)
{
  _globalP = new GlobalP2x2;
  _gainMap = new Cspad2x2GainMap;
  _quadP[0] = new QuadP2x2;
}

Cspad2x2ConfigTable::~Cspad2x2ConfigTable()
{
}

void Cspad2x2ConfigTable::insert(Pds::LinkedList<Parameter>& pList)
{
  pList.insert(this);

  _globalP->insert(_pList);
  _gainMap->insert(_pList);
  _quadP[0]->insert(_pList);
}

int Cspad2x2ConfigTable::pull(const void* from) {
  const CsPad2x2ConfigType& tc = *reinterpret_cast<const CsPad2x2ConfigType*>(from);

  _globalP->pull(tc);
  _quadP[0]->pull(tc.quad()[0],_gainMap->quad());
  _gainMap->flush();

  return sizeof(tc);
}

int Cspad2x2ConfigTable::push(void* to) const {

  CsPad2x2ConfigType& tc = *reinterpret_cast<CsPad2x2ConfigType*>(to);
  _globalP->push(&tc);
  _quadP[0]->push(&(tc.quad()[0]),_gainMap->quad());

  return sizeof(tc);
}

int Cspad2x2ConfigTable::dataSize() const {
  return sizeof(CsPad2x2ConfigType);
}

bool Cspad2x2ConfigTable::validate()
{
  return true;
}

QLayout* Cspad2x2ConfigTable::initialize(QWidget* parent)
{
  QHBoxLayout* layout = new QHBoxLayout;

  { QVBoxLayout* gl = new QVBoxLayout;
  _globalP->initialize(parent,gl);
  layout->addLayout(gl); }

  layout->addSpacing(40);

  { QGridLayout* ql = new QGridLayout;
  QuadP2x2::layoutHeader(ql);
  _quadP[0]->initialize(parent,ql);
  layout->addLayout(ql); }

  layout->addSpacing(40);

  { QVBoxLayout* gl = new QVBoxLayout;
  _gainMap->initialize(parent,gl);
  layout->addLayout(gl); }

  return layout;
}

void Cspad2x2ConfigTable::update() {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->update();
    p = p->forward();
  }
}

void Cspad2x2ConfigTable::flush () {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->flush();
    p = p->forward();
  }
}

void Cspad2x2ConfigTable::enable(bool)
{
}

Cspad2x2ConfigTableQ::Cspad2x2ConfigTableQ(GlobalP2x2& table,
    QWidget* parent) :
    QObject(parent),
    _table (table)
{
}

void Cspad2x2ConfigTableQ::update_readout() { _table.update_readout(); }

#include "Parameters.icc"
