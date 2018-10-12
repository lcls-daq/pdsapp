#include "pdsapp/config/Epix10ka2MConfig.hh"

#include "pdsapp/config/Epix10kaASICdata.hh"
#include "pdsapp/config/Epix10kaCalibMap.hh"
#include "pdsapp/config/Epix10kaPixelMap.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/CopyBox.hh"
#include "pds/config/EpixConfigType.hh"

#include <stdio.h>
#include <QtCore/QObject>

#include "pdsapp/config/Epix10ka2MMap.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QtGui/QMessageBox>
#include <QtGui/QTabWidget>
#include <QtGui/QGroupBox>
#include <QtGui/QPushButton>

static const int PolarityGroup = 100;

// static const unsigned ASICS   = Pds::CsPad::ASICsPerQuad;
// static const unsigned Columns = Pds::CsPad::ColumnsPerASIC;
// static const unsigned Rows    = Pds::CsPad::MaxRowsPerASIC;

static const unsigned pixelArrayShape[] = {Pds::Epix::Config10ka::_numberOfRowsPerAsic *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerColumn,
                                           Pds::Epix::Config10ka::_numberOfPixelsPerAsicRow *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerRow};

static const unsigned calibArrayShape[] = {Pds::Epix::Config10ka::_calibrationRowCountPerASIC *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerColumn,
                                           Pds::Epix::Config10ka::_numberOfPixelsPerAsicRow *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerRow};

enum Values { _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10 };

//enum ExtPwdnMode { FullPowerDown, Standyby };
static const char* ExtPwdnModeCh[] = { "FullPowerDown","Standby", NULL };

//enum IntPwdnMode { ChipRun, FullPowerDown, Standby, DigitalReset };
static const char* IntPwdnModeCh[] = { "ChipRun", "FullPowerDown", "Standby", "DigitalReset", NULL };

//enum UserTestModeCfg { single, alternate, single_once, alternate_once };
static const char* UserTestModeCh[] = { "Single", "Alternate", "Single Once", "Alternate Once", NULL };

//enum OutputTestMode { Off, MidscaleShort, PosFS, NegFS, AltCheckerBoard, PN23, PN9, OneZeroWordToggle, UserInput, OneZeroBitToggle };
static const char* OutputTestModeCh[] = { "Off", "MidscaleShort", "PosFS", "NegFS", "AltCheckerBoard", 
                                          "PN23", "PN9", "1/0 WordToggle", "UserInput", "1/0 BitToggle", NULL };
static const char* ClockDivideCh[] = { "1","2","3","4","5","6","7","8", NULL };

enum OutputFormat { TwosComplement, OffsetBinary };
static const char* OutputFormatCh[] = { "TwosComplement", "OffsetBinary", NULL };

namespace Pds_ConfigDb {
  namespace Epix10ka2M {
    //  class CspadGainMap;
    class GlobalP;
    class QuadP;

    class ConfigTable : public Parameter {
    public:
      ConfigTable(const Epix10ka2MConfig& c);
      ~ConfigTable();
    public:
      void insert(Pds::LinkedList<Parameter>& pList);
      int  pull  (const Epix10ka2MConfigType& tc);
      int  push  (void* to) const;
      int  dataSize() const;
      bool validate();
      //
      void update_maskg();
      void update_maskv();
      void pixel_map_dialog();
      void calib_map_dialog();
    public:
      QLayout* initialize(QWidget* parent);
      void     flush     ();
      void     update    ();
      void     enable    (bool);
    public:
      const Epix10ka2MConfig&         _cfg;
      Pds::LinkedList<Parameter>      _pList;
      GlobalP*                        _globalP;
      QuadP*                          _quadP[4];
      ConfigTableQ*                   _qlink;
      Epix10ka2MMap*                  _pixelMap;
      QPushButton*                    _pixelMapB;
      QPushButton*                    _calibMapB;
      Epix10kaPixelMapDialog*         _pixelMapD [16];
      Epix10kaCalibMapDialog*         _calibMapD [16];
    };

    // class ConfigTableQ : public QObject {
    //   Q_OBJECT
    // public:
    //   ConfigTableQ(GlobalP&,
    //                          QWidget*);
    // public slots:
    //   void update_readout();
    // private:
    //   GlobalP& _table;
    // };

    class GlobalP {
    public:
      GlobalP() :
        _evrRunCode      ( "EVR Run EventCode"     , 40, 0, 255, Decimal),
        _evrDaqCode      ( "EVR Acq EventCode"     , 40, 0, 255, Decimal),
        _evrRunDelay     ( "EVR Run Delay [119MHz]", 0, 0, (1U<<31)-1, Decimal),
        _asicMask        ( "ASIC Mask"             , -1ULL, 0, -1ULL, Hex)
      {
        for(unsigned a=0; a<16; a++) {
          _pixelArray[a] = ndarray<uint16_t,2>(pixelArrayShape);
          _calibArray[a] = ndarray<uint8_t ,2>(calibArrayShape);
        }
      }
    public:
      void enable(bool v)
      {
      }
      void reset ()
      {
        _evrRunCode.value = 40;
        _evrDaqCode.value = 40;
        _evrRunDelay.value = 0;
      }
      void pull   (const Epix10ka2MConfigType& p)
      {
        _evrRunCode .value = p.evr().runCode ();
        _evrDaqCode .value = p.evr().daqCode ();
        _evrRunDelay.value = p.evr().runDelay();
        _asicMask   .value = 0;
        for(unsigned i=0; i<16; i++) {
          _asicMask .value |= uint64_t(p.elemCfg(i).asicMask())<<(4*i);
          for(unsigned j=0; j<4; j++)
            _asic[j+4*i].pull(reinterpret_cast<const Epix10kaASIC_ConfigShadow*>(&p.elemCfg(i).asics(j)));
          std::copy(p.elemCfg(i).asicPixelConfigArray().begin(),
                    p.elemCfg(i).asicPixelConfigArray().end(),
                    _pixelArray[i].begin());
          std::copy(p.elemCfg(i).calibPixelConfigArray().begin(),
                    p.elemCfg(i).calibPixelConfigArray().end(),
                    _calibArray[i].begin());
        }
      }
      void push   (Epix10ka2MConfigType* p)
      {
        Pds::Epix::PgpEvrConfig evr(1U, _evrRunCode.value, _evrDaqCode.value, _evrRunDelay.value);

        Pds::Epix::Config10ka elemCfg[16];
        for(unsigned i=0; i<16; i++) {
          Pds::Epix::Asic10kaConfigV1 asics[4];
          for(unsigned j=0; j<4; j++)
            _asic[j+4*i].push(reinterpret_cast<Epix10kaASIC_ConfigShadow*>(&asics[j]));
          elemCfg[i] = Pds::Epix::Config10ka( 0, 0, (_asicMask.value>>(4*i))&0xf, 
                                              asics, 
                                              _pixelArray[i].begin(),
                                              _calibArray[i].begin() );
        }
        
        *new (p) Epix10ka2MConfigType( evr, 0, elemCfg );
      }
    public:
      void initialize(QWidget* parent, QVBoxLayout* layout)
      {
        layout->addLayout(_evrRunCode     .initialize(parent));
        layout->addLayout(_evrDaqCode     .initialize(parent));
        layout->addLayout(_evrRunDelay    .initialize(parent));
        layout->addSpacing(40);
        layout->addWidget(_asicMaskMap = new Epix10ka2MMap);
        layout->addLayout(_asicMask.initialize(parent));
      }

      void insert(Pds::LinkedList<Parameter>& pList) {
        pList.insert(&_evrRunCode);
        pList.insert(&_evrDaqCode);
        pList.insert(&_evrRunDelay);
      }

    public:
      NumericInt<unsigned>             _evrRunCode;
      NumericInt<unsigned>             _evrDaqCode;
      NumericInt<unsigned>             _evrRunDelay;
      Epix10ka2MMap*                   _asicMaskMap;
      NumericInt<uint64_t>             _asicMask;
      Epix10kaASICdata                 _asic[64];
      ndarray<uint16_t,2>              _pixelArray[16];
      ndarray<uint8_t ,2>              _calibArray[16];
    };


    class AD9249Display {
    public:
      AD9249Display() :
        _devIndexMask      (NULL, 0xff, 0, 0xff, Hex),
        _devIndexMaskDcoFco(NULL, 0x3, 0, 0x3, Hex),
        _extPwdnMode       (NULL, _0, ExtPwdnModeCh),
        _intPwdnMode       (NULL, _0, IntPwdnModeCh),
        _chopMode          (NULL, Enums::Off, Enums::OnOff_Names),
        _dutyCycleStab     (NULL, Enums::On, Enums::OnOff_Names),
        _outputInvert      (NULL, 0, 0, 1, Decimal),
        _outputFormat      (NULL, _1, OutputFormatCh),
        _clockDivide       (NULL, _0, ClockDivideCh),
        _userTestMode      (NULL, _0, UserTestModeCh),
        _outputTestMode    (NULL, _0, OutputTestModeCh),
        _offsetAdjust      (NULL, 0, 0, 0xff, Decimal),
        _channelDelay      (NULL, 0xa0, 0, 0x3ff, Decimal, 1., 2),
        _frameDelay        (NULL, 0xa0, 0, 0x3ff, Decimal) {}
    public:
      static void initializeTab(QWidget* parent, QGridLayout* gl)
      {
        unsigned row=1;
        gl->addWidget(new QLabel("devIndexMask      "),row,0);
        gl->addWidget(new QLabel("devIndexMaskDcoFco"),row++,2);
        gl->addWidget(new QLabel("extPwdnMode       "),row,0);
        gl->addWidget(new QLabel("intPwdnMode       "),row++,2);
        gl->addWidget(new QLabel("chopMode          "),row,0);
        gl->addWidget(new QLabel("dutyCycleStab     "),row++,2);
        gl->addWidget(new QLabel("outputInvert      "),row,0);
        gl->addWidget(new QLabel("outputFormat      "),row++,2);
        gl->addWidget(new QLabel("clockDivide       "),row,0);
        gl->addWidget(new QLabel("userTestMode      "),row++,2);
        gl->addWidget(new QLabel("outputTestMode    "),row,0);
        gl->addWidget(new QLabel("offsetAdjust      "),row++,2);
        gl->addWidget(new QLabel("channelDelay      "),row++,0);
        gl->addWidget(new QLabel("frameDelay        "),row++,0);
      }
      void initialize(QWidget* parent, QGridLayout* gl)
      {
        unsigned row=1;
        gl->addLayout(_devIndexMask      .initialize(parent),row,1);
        gl->addLayout(_devIndexMaskDcoFco.initialize(parent),row++,3);
        gl->addLayout(_extPwdnMode       .initialize(parent),row,1);
        gl->addLayout(_intPwdnMode       .initialize(parent),row++,3);
        gl->addLayout(_chopMode          .initialize(parent),row,1);
        gl->addLayout(_dutyCycleStab     .initialize(parent),row++,3);
        gl->addLayout(_outputInvert      .initialize(parent),row,1);
        gl->addLayout(_outputFormat      .initialize(parent),row++,3);
        gl->addLayout(_clockDivide       .initialize(parent),row,1);
        gl->addLayout(_userTestMode      .initialize(parent),row++,3);
        gl->addLayout(_outputTestMode    .initialize(parent),row,1);
        gl->addLayout(_offsetAdjust      .initialize(parent),row++,3);
        gl->addLayout(_channelDelay      .initialize(parent),row++,1,1,3);
        gl->addLayout(_frameDelay        .initialize(parent),row++,1);
      }
      void insert(Pds::LinkedList<Parameter>& pList)
      {
        pList.insert(&_devIndexMask      );
        pList.insert(&_devIndexMaskDcoFco);
        pList.insert(&_extPwdnMode       );
        pList.insert(&_intPwdnMode       );
        pList.insert(&_chopMode          );
        pList.insert(&_dutyCycleStab     );
        pList.insert(&_outputInvert      );
        pList.insert(&_outputFormat      );
        pList.insert(&_clockDivide       );
        pList.insert(&_userTestMode      );
        pList.insert(&_outputTestMode    );
        pList.insert(&_offsetAdjust      );
        pList.insert(&_channelDelay      );
        pList.insert(&_frameDelay        );
      }
      void pull(const Pds::Epix::Ad9249Config& p)
      {
        _devIndexMask      .value=p.devIndexMask();
        _devIndexMaskDcoFco.value=p.devIndexMaskDcoFco();
        _extPwdnMode       .value=Values(p.extPwdnMode());
        _intPwdnMode       .value=Values(p.intPwdnMode());
        _chopMode          .value=Enums::OnOff(p.chopMode());
        _dutyCycleStab     .value=Enums::OnOff(p.dutyCycleStab());
        _outputInvert      .value=p.outputInvert();
        _outputFormat      .value=Values(p.outputFormat());
        _clockDivide       .value=Values(p.clockDivide());
        _userTestMode      .value=Values(p.userTestMode());
        _outputTestMode    .value=Values(p.outputTestMode());
        _offsetAdjust      .value=p.offsetAdjust();
        std::copy(p.channelDelay().data(),p.channelDelay().data()+8,_channelDelay.value);
        _frameDelay        .value=p.frameDelay();
      }
      void push(Pds::Epix::Ad9249Config& p)
      {  *new(&p) Pds::Epix::Ad9249Config( 0,
                                           _devIndexMask      .value,
                                           _devIndexMaskDcoFco.value,
                                           _extPwdnMode       .value,
                                           _intPwdnMode       .value,
                                           _chopMode          .value,
                                           _dutyCycleStab     .value,
                                           _outputInvert      .value,
                                           _outputFormat      .value,
                                           _clockDivide       .value,
                                           _userTestMode      .value,
                                           _outputTestMode    .value,
                                           _offsetAdjust      .value,
                                           _channelDelay      .value,
                                           _frameDelay        .value );
      }
    public:
      NumericInt<uint32_t> _devIndexMask      ;
      NumericInt<uint32_t> _devIndexMaskDcoFco;
      Enumerated<Values  > _extPwdnMode       ;
      Enumerated<Values  > _intPwdnMode       ;
      Enumerated<Enums::OnOff> _chopMode      ;
      Enumerated<Enums::OnOff> _dutyCycleStab ;
      NumericInt<uint8_t > _outputInvert      ;
      Enumerated<Values  > _outputFormat      ;
      Enumerated<Values  > _clockDivide       ;
      Enumerated<Values  > _userTestMode      ;
      Enumerated<Values  > _outputTestMode    ;
      NumericInt<uint32_t> _offsetAdjust      ;
      NumericIntArray<uint32_t,8> _channelDelay      ;
      NumericInt<uint32_t> _frameDelay        ;
    };

    class QuadP {
    public:
      static void initialize_tabs(QWidget*     parent,
                                  QGridLayout* gl_sys,
                                  QGridLayout* gl_acq,
                                  QGridLayout* gl_sco,
                                  QGridLayout** gl_adc,
                                  QLayout**     gl_asi) {
        for(unsigned i=0; i<4; i++) {
          gl_sys->addWidget(new QLabel(QString("Quad %1").arg(i)), 0, i+1);
          gl_acq->addWidget(new QLabel(QString("Quad %1").arg(i)), 0, i+1);
          gl_sco->addWidget(new QLabel(QString("Quad %1").arg(i)), 0, i+1);
        }
        unsigned row=1;
        gl_sys->addWidget(new QLabel("dcdcEn"    ),row++,0);
        gl_sys->addWidget(new QLabel("asicAnaEn" ),row++,0);
        gl_sys->addWidget(new QLabel("asicDigEn" ),row++,0);
        gl_sys->addWidget(new QLabel("ddrVttEn"  ),row++,0);
        gl_sys->addWidget(new QLabel("trigSrcSel"),row++,0);
        gl_sys->addWidget(new QLabel("-"),row++,0); gl_sys->setRowStretch(row-1,1);
        row=1;
        gl_acq->addWidget(new QLabel("acqToAsicR0Delay"),row++,0);
        gl_acq->addWidget(new QLabel("asicR0Width"     ),row++,0);
        gl_acq->addWidget(new QLabel("asicR0ToAsicAcq" ),row++,0);
        gl_acq->addWidget(new QLabel("asicAcqWidth"    ),row++,0);
        gl_acq->addWidget(new QLabel("asicAcqLToPPmatL"),row++,0);
        gl_acq->addWidget(new QLabel("asicPPmatToRead" ),row++,0);
        gl_acq->addWidget(new QLabel("asicRoClkHalfT"  ),row++,0);
        gl_acq->addWidget(new QLabel("asicAcqForce"    ),row++,0);
        gl_acq->addWidget(new QLabel("asicAcqValue"    ),row++,0);
        gl_acq->addWidget(new QLabel("asicR0Force"     ),row++,0);
        gl_acq->addWidget(new QLabel("asicR0Value"     ),row++,0);
        gl_acq->addWidget(new QLabel("asicPPmatForce"  ),row++,0);
        gl_acq->addWidget(new QLabel("asicPPmatValue"  ),row++,0);
        gl_acq->addWidget(new QLabel("asicSyncForce"   ),row++,0);
        gl_acq->addWidget(new QLabel("asicSyncValue"   ),row++,0);
        gl_acq->addWidget(new QLabel("asicRoClkForce"  ),row++,0);
        gl_acq->addWidget(new QLabel("asicRoClkValue"  ),row++,0);
        gl_acq->addWidget(new QLabel("adcPipelineDelay"),row++,0);
        gl_acq->addWidget(new QLabel("testData"        ),row++,0);
        gl_acq->addWidget(new QLabel("-"),row++,0); gl_acq->setRowStretch(row-1,1);
        row=1;
        gl_sco->addWidget(new QLabel("enable"       ),row++,0);
        gl_sco->addWidget(new QLabel("trigEdge"     ),row++,0);
        gl_sco->addWidget(new QLabel("trigChan"     ),row++,0);
        gl_sco->addWidget(new QLabel("trigMode"     ),row++,0);
        gl_sco->addWidget(new QLabel("adcThreshold" ),row++,0);
        gl_sco->addWidget(new QLabel("trigHoldoff"  ),row++,0);
        gl_sco->addWidget(new QLabel("trigOffset"   ),row++,0);
        gl_sco->addWidget(new QLabel("trigDelay"    ),row++,0);
        gl_sco->addWidget(new QLabel("traceLength"  ),row++,0);
        gl_sco->addWidget(new QLabel("samplesToSkip"),row++,0);
        gl_sco->addWidget(new QLabel("chanASelect"  ),row++,0);
        gl_sco->addWidget(new QLabel("chanBSelect"  ),row++,0);
        gl_sco->addWidget(new QLabel("-"),row++,0); gl_sco->setRowStretch(row-1,1);

        for(unsigned ch=0; ch<40; ch++) 
          AD9249Display::initializeTab(parent,gl_adc[ch]);
      }
    public:
      QuadP() :
        //  SystemRegs
        _dcdcEn             ( NULL, 0xf,    0, 0xf, Hex),
        _asicAnaEn          ( NULL, 0,      0, 1, Decimal),
        _asicDigEn          ( NULL, 0,      0, 1, Decimal),
        _ddrVttEn           ( NULL, 0,      0, 1, Decimal),
        _trigSrcSel         ( NULL, 0,      0, 3, Decimal),
        //  AcqCore
        _acqToAsicR0Delay   ( NULL, 0,      0, 0xffffffff, Decimal),
        _asicR0Width        ( NULL, 100,    0, 0xffffffff, Decimal),
        _asicR0ToAsicAcq    ( NULL, 100,    0, 0xffffffff, Decimal),
        _asicAcqWidth       ( NULL, 100,    0, 0xffffffff, Decimal),
        _asicAcqLToPPmatL   ( NULL, 0,      0, 0xffffffff, Decimal),
        _asicPPmatToReadout ( NULL, 0,      0, 0xffffffff, Decimal),
        _asicRoClkHalfT     ( NULL, 3,      0, 0xffffffff, Decimal),
        _asicAcqForce       ( NULL, Enums::False, Enums::Bool_Names),
        _asicAcqValue       ( NULL, Enums::False, Enums::Bool_Names),
        _asicR0Force        ( NULL, Enums::False, Enums::Bool_Names),
        _asicR0Value        ( NULL, Enums::False, Enums::Bool_Names),
        _asicPPmatForce     ( NULL, Enums::True , Enums::Bool_Names),
        _asicPPmatValue     ( NULL, Enums::True , Enums::Bool_Names),
        _asicSyncForce      ( NULL, Enums::False, Enums::Bool_Names),
        _asicSyncValue      ( NULL, Enums::False, Enums::Bool_Names),
        _asicRoClkForce     ( NULL, Enums::False, Enums::Bool_Names),
        _asicRoClkValue     ( NULL, Enums::False, Enums::Bool_Names),
        _adcPipelineDelay   ( NULL, 0,      0, 0xffffffff, Decimal),
        _testData           ( NULL, Enums::False,  Enums::Bool_Names),
      // ScopeCore
        _scopeEnable        ( NULL, 0,      0, 1,       Decimal),
        _scopeTrigEdge      ( NULL, 0,      0, 1,     Decimal),
        _scopeTrigChan      ( NULL, 0,      0, 31,     Decimal),
        _scopeTrigMode      ( NULL, 0,      0, 3,     Decimal),
        _scopeAdcThreshold  ( NULL, 0,      0, 0xffff,     Decimal),
        _scopeTrigHoldoff   ( NULL, 0,      0, 0x1fff,     Decimal),
        _scopeTrigOffset    ( NULL, 0,      0, 0x1fff,     Decimal),
        _scopeTrigDelay     ( NULL, 0,      0, 0x1fff,     Decimal),
        _scopeTraceLength   ( NULL, 0,      0, 0x1fff,     Decimal),
        _scopeSamplesToSkip ( NULL, 0,      0, 0x1fff,     Decimal),
        _scopeChanASelect   ( NULL, 0,      0, 0x7f,       Decimal),
        _scopeChanBSelect   ( NULL, 0,      0, 0x7f,       Decimal)
      {
      }
    public:
      void enable(bool v)
      {
      }
      void reset ()
      {
      }
      void pull   (const Epix10kaQuadConfigType& p)
      {
        //  System
        _dcdcEn             .value = p.dcdcEn             ();
        _asicAnaEn          .value = p.asicAnaEn          ();
        _asicDigEn          .value = p.asicDigEn          ();
        _ddrVttEn           .value = p.ddrVttEn           ();
        _trigSrcSel         .value = p.trigSrcSel         ();
        //  AcqCore
        _acqToAsicR0Delay   .value = p.acqToAsicR0Delay   ();
        _asicR0Width        .value = p.asicR0Width        ();
        _asicR0ToAsicAcq    .value = p.asicR0ToAsicAcq    ();
        _asicAcqWidth       .value = p.asicAcqWidth       ();
        _asicAcqLToPPmatL   .value = p.asicAcqLToPPmatL   ();
        _asicPPmatToReadout .value = p.asicPPmatToReadout ();
        _asicRoClkHalfT     .value = p.asicRoClkHalfT     ();
        _asicAcqForce       .value = Enums::Bool(p.asicAcqForce       ());
        _asicAcqValue       .value = Enums::Bool(p.asicAcqValue       ());
        _asicR0Force        .value = Enums::Bool(p.asicR0Force        ());
        _asicR0Value        .value = Enums::Bool(p.asicR0Value        ());
        _asicPPmatForce     .value = Enums::Bool(p.asicPPmatForce     ());
        _asicPPmatValue     .value = Enums::Bool(p.asicPPmatValue     ());
        _asicSyncForce      .value = Enums::Bool(p.asicSyncForce      ());
        _asicSyncValue      .value = Enums::Bool(p.asicSyncValue      ());
        _asicRoClkForce     .value = Enums::Bool(p.asicRoClkForce     ());
        _asicRoClkValue     .value = Enums::Bool(p.asicRoClkValue     ());
        _adcPipelineDelay   .value = p.adcPipelineDelay   ();
        _testData           .value = Enums::Bool(p.testData           ());
        // ScopeCore
        _scopeEnable        .value = p.scopeEnable        ();
        _scopeTrigEdge      .value = p.scopeTrigEdge      ();
        _scopeTrigChan      .value = p.scopeTrigChan      ();
        _scopeTrigMode      .value = p.scopeTrigMode      ();
        _scopeAdcThreshold  .value = p.scopeADCThreshold  ();
        _scopeTrigHoldoff   .value = p.scopeTrigHoldoff   ();
        _scopeTrigOffset    .value = p.scopeTrigOffset    ();
        _scopeTrigDelay     .value = p.scopeTrigDelay     ();
        _scopeTraceLength   .value = p.scopeTraceLength   ();
        _scopeSamplesToSkip .value = p.scopeADCsamplesToSkip ();
        _scopeChanASelect   .value = p.scopeChanAwaveformSelect   ();
        _scopeChanBSelect   .value = p.scopeChanBwaveformSelect   ();
      
        for(unsigned ch=0; ch<10; ch++)
          _adc[ch].pull(p.adc(ch));
      }
      void push   (Epix10kaQuadConfigType* p)
      {
        Pds::Epix::Ad9249Config     adcs[10];
        for(unsigned ch=0; ch<10; ch++)
          _adc[ch].push(adcs[ch]);

        *new(p) Epix10kaQuadConfigType(125000000,  // baseClockFreq
                                       0,          // enableAutomaticRunTrigger
                                       125e6/120,  // numberOf125MhzTicksPerRunTrigger
                                       _dcdcEn             .value,
                                       _asicAnaEn          .value,
                                       _asicDigEn          .value,
                                       _ddrVttEn           .value,
                                       _trigSrcSel         .value,
                                       //  AcqCore
                                       _acqToAsicR0Delay   .value,
                                       _asicR0Width        .value,
                                       _asicR0ToAsicAcq    .value,
                                       _asicAcqWidth       .value,
                                       _asicAcqLToPPmatL   .value,
                                       _asicPPmatToReadout .value,
                                       _asicRoClkHalfT     .value,
                                       _asicAcqForce       .value,
                                       _asicR0Force        .value,
                                       _asicPPmatForce     .value,
                                       _asicSyncForce      .value,
                                       _asicRoClkForce     .value,
                                       _asicAcqValue       .value,
                                       _asicR0Value        .value,
                                       _asicPPmatValue     .value,
                                       _asicSyncValue      .value,
                                       _asicRoClkValue     .value,
                                       _adcPipelineDelay   .value,
                                       _testData           .value,
                                       // ScopeCore
                                       _scopeEnable        .value,
                                       _scopeTrigEdge      .value,
                                       _scopeTrigChan      .value,
                                       _scopeTrigMode      .value,
                                       _scopeAdcThreshold  .value,
                                       _scopeTrigHoldoff   .value,
                                       _scopeTrigOffset    .value,
                                       _scopeTraceLength   .value,
                                       _scopeSamplesToSkip .value,
                                       _scopeChanASelect   .value,
                                       _scopeChanBSelect   .value,
                                       _scopeTrigDelay     .value,
                                       adcs,
                                       0, 0, 0, 0, 0, 0);  // AdcTester
      }
    public:
      void initialize(QWidget* parent, QGridLayout* gl_sys, QGridLayout* gl_acq, QGridLayout* gl_sco, QGridLayout** gl_adc, QLayout** gl_asi, unsigned q)
      {
        unsigned row=1;
        gl_sys->addLayout(_dcdcEn.initialize(parent)    ,row++,q+1);
        gl_sys->addLayout(_asicAnaEn.initialize(parent) ,row++,q+1);
        gl_sys->addLayout(_asicDigEn.initialize(parent) ,row++,q+1);
        gl_sys->addLayout(_ddrVttEn.initialize(parent)  ,row++,q+1);
        gl_sys->addLayout(_trigSrcSel.initialize(parent),row++,q+1);
        row=1;
        gl_acq->addLayout(_acqToAsicR0Delay.initialize(parent),row++,q+1);
        gl_acq->addLayout(_asicR0Width.initialize(parent)     ,row++,q+1);
        gl_acq->addLayout(_asicR0ToAsicAcq.initialize(parent) ,row++,q+1);
        gl_acq->addLayout(_asicAcqWidth.initialize(parent)    ,row++,q+1);
        gl_acq->addLayout(_asicAcqLToPPmatL.initialize(parent),row++,q+1);      
        gl_acq->addLayout(_asicPPmatToReadout.initialize(parent) ,row++,q+1);
        gl_acq->addLayout(_asicRoClkHalfT.initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_asicAcqForce    .initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_asicAcqValue    .initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_asicR0Force     .initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_asicR0Value     .initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_asicPPmatForce  .initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_asicPPmatValue  .initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_asicSyncForce   .initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_asicSyncValue   .initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_asicRoClkForce  .initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_asicRoClkValue  .initialize(parent)  ,row++,q+1);
        gl_acq->addLayout(_adcPipelineDelay.initialize(parent),row++,q+1);
        gl_acq->addLayout(_testData        .initialize(parent),row++,q+1);
        row=1;
        gl_sco->addLayout(_scopeEnable.initialize(parent)       ,row++,q+1);
        gl_sco->addLayout(_scopeTrigEdge.initialize(parent)     ,row++,q+1);
        gl_sco->addLayout(_scopeTrigChan.initialize(parent)     ,row++,q+1);
        gl_sco->addLayout(_scopeTrigMode.initialize(parent)      ,row++,q+1);
        gl_sco->addLayout(_scopeAdcThreshold.initialize(parent) ,row++,q+1);
        gl_sco->addLayout(_scopeTrigHoldoff.initialize(parent)  ,row++,q+1);
        gl_sco->addLayout(_scopeTrigOffset .initialize(parent)  ,row++,q+1);
        gl_sco->addLayout(_scopeTrigDelay  .initialize(parent)  ,row++,q+1);
        gl_sco->addLayout(_scopeTraceLength.initialize(parent)  ,row++,q+1);
        gl_sco->addLayout(_scopeSamplesToSkip.initialize(parent),row++,q+1);
        gl_sco->addLayout(_scopeChanASelect.initialize(parent)  ,row++,q+1);
        gl_sco->addLayout(_scopeChanBSelect.initialize(parent)  ,row++,q+1);

        for(unsigned ch=0; ch<10; ch++)
          _adc[ch].initialize(parent, gl_adc[ch]);

      }

      void insert(Pds::LinkedList<Parameter>& pList) {
#define ADDP(t) pList.insert(&t)
        ADDP(_dcdcEn);
        ADDP(_asicAnaEn);
        ADDP(_asicDigEn);
        ADDP(_ddrVttEn);
        ADDP(_trigSrcSel);
        ADDP(_acqToAsicR0Delay);
        ADDP(_asicR0Width);
        ADDP(_asicR0ToAsicAcq);
        ADDP(_asicAcqWidth);
        ADDP(_asicAcqLToPPmatL);
        ADDP(_asicPPmatToReadout);
        ADDP(_asicRoClkHalfT);
        ADDP(_asicAcqForce);
        ADDP(_asicAcqValue);
        ADDP(_asicR0Force);
        ADDP(_asicR0Value);
        ADDP(_asicPPmatForce);
        ADDP(_asicPPmatValue);
        ADDP(_asicSyncForce);
        ADDP(_asicSyncValue);
        ADDP(_asicRoClkForce);
        ADDP(_asicRoClkValue);
        ADDP(_adcPipelineDelay);
        ADDP(_testData);
        ADDP(_scopeEnable);
        ADDP(_scopeTrigEdge);
        ADDP(_scopeTrigChan);
        ADDP(_scopeTrigMode);
        ADDP(_scopeAdcThreshold);
        ADDP(_scopeTrigHoldoff);
        ADDP(_scopeTrigOffset);
        ADDP(_scopeTraceLength);
        ADDP(_scopeSamplesToSkip);
        ADDP(_scopeChanASelect);
        ADDP(_scopeChanBSelect);
        ADDP(_scopeTrigDelay);
#undef ADDP
        for(unsigned c=0; c<10; c++)
          _adc[c].insert(pList);
      }

    public:
      NumericInt<uint32_t> _dcdcEn;
      NumericInt<uint32_t> _asicAnaEn;
      NumericInt<uint32_t> _asicDigEn;
      NumericInt<uint32_t> _ddrVttEn;
      NumericInt<uint32_t> _trigSrcSel;
      // AcqCore
      NumericInt<uint32_t> _acqToAsicR0Delay;
      NumericInt<uint32_t> _asicR0Width;
      NumericInt<uint32_t> _asicR0ToAsicAcq;
      NumericInt<uint32_t> _asicAcqWidth;
      NumericInt<uint32_t> _asicAcqLToPPmatL;
      NumericInt<uint32_t> _asicPPmatToReadout;
      NumericInt<uint32_t> _asicRoClkHalfT;
      Enumerated<Enums::Bool> _asicAcqForce;
      Enumerated<Enums::Bool> _asicAcqValue;
      Enumerated<Enums::Bool> _asicR0Force;
      Enumerated<Enums::Bool> _asicR0Value;
      Enumerated<Enums::Bool> _asicPPmatForce;
      Enumerated<Enums::Bool> _asicPPmatValue;
      Enumerated<Enums::Bool> _asicSyncForce;
      Enumerated<Enums::Bool> _asicSyncValue;
      Enumerated<Enums::Bool> _asicRoClkForce;
      Enumerated<Enums::Bool> _asicRoClkValue;
      NumericInt<uint32_t> _adcPipelineDelay;
      Enumerated<Enums::Bool> _testData;
      // ScopeCore
      NumericInt<uint8_t> _scopeEnable;
      NumericInt<uint8_t> _scopeTrigEdge;
      NumericInt<uint8_t> _scopeTrigChan;
      NumericInt<uint8_t> _scopeTrigMode;
      NumericInt<uint16_t> _scopeAdcThreshold;
      NumericInt<uint16_t> _scopeTrigHoldoff;
      NumericInt<uint16_t> _scopeTrigOffset;
      NumericInt<uint32_t> _scopeTrigDelay;
      NumericInt<uint16_t> _scopeTraceLength;
      NumericInt<uint16_t> _scopeSamplesToSkip;
      NumericInt<uint8_t> _scopeChanASelect;
      NumericInt<uint8_t> _scopeChanBSelect;
      //      NumericIntArray<uint32_t,4> _shiftSelect;
      //      NumericIntArray<uint32_t,4> _edgeSelect;
      AD9249Display    _adc [10];
    };

    class AdcCopyBox : public CopyBox {
    public:
      AdcCopyBox(ConfigTable& table) :
        CopyBox("Copy ADCs","Q","",4,10), 
        _table(table) {}
    public:
      void initialize() { _table.update(); }
      void finalize  () { _table.flush(); }
      void copyItem(unsigned fromRow,
                    unsigned fromColumn,
                    unsigned toRow,
                    unsigned toColumn) 
      {
        _table._quadP[toRow]->_adc[toColumn] = _table._quadP[fromRow]->_adc[fromColumn];
      }
    private:
      ConfigTable& _table;
    };

    class AsicCopyBox : public CopyBox {
    public:
      AsicCopyBox(ConfigTable& table) :
        CopyBox("Copy ASICs","Q","A",4,16), 
        _table(table) {}
    public:
      void initialize() { _table.update(); }
      void finalize  () { _table.flush(); }
      void copyItem(unsigned fromRow,
                    unsigned fromColumn,
                    unsigned toRow,
                    unsigned toColumn) 
      {
        _table._globalP->_asic[toColumn+16*toRow].copy(_table._globalP->_asic[fromColumn+16*fromRow]);
      }
    private:
      ConfigTable& _table;
    };
  };
};

using namespace Pds_ConfigDb::Epix10ka2M;

ConfigTable::ConfigTable(const Epix10ka2MConfig& c) : 
  Parameter(NULL),
  _cfg(c)
{
  _globalP = new GlobalP;
  //  _gainMap = new Epix10ka2MGainMap;
  for(unsigned q=0; q<4; q++)
    _quadP[q] = new QuadP;
  for(unsigned a=0; a<16; a++) {
    _pixelMapD[a] = new Epix10kaPixelMapDialog(_globalP->_pixelArray[a]);
    _calibMapD[a] = new Epix10kaCalibMapDialog(_globalP->_calibArray[a]);
  }
}

ConfigTable::~ConfigTable()
{
}

void ConfigTable::insert(Pds::LinkedList<Parameter>& pList)
{
  pList.insert(this);

  _globalP->insert(_pList);
  //  _gainMap->insert(_pList);
  for(unsigned q=0; q<4; q++)
    _quadP[q]->insert(_pList);
}

int ConfigTable::pull(const Epix10ka2MConfigType& tc) {
  _globalP->pull(tc);
  for(unsigned q=0; q<4; q++)
    _quadP[q]->pull(tc.quad(q)); //,_gainMap->quad(q));
  //  _gainMap->flush();

  return sizeof(tc);
}

int ConfigTable::push(void* to) const {

  Epix10ka2MConfigType& tc = *reinterpret_cast<Epix10ka2MConfigType*>(to);
  _globalP->push(&tc);
  for(unsigned q=0; q<4; q++)
    _quadP[q]->push(&const_cast<Epix10kaQuadConfigType&>(tc.quad(q)));
  return sizeof(tc);
}

int ConfigTable::dataSize() const {
  return sizeof(Epix10ka2MConfigType);
}

bool ConfigTable::validate()
{
  unsigned diffRows=0;

#if 1
#define CHECKQUADMEMBER(a) for(unsigned q=1; q<4; q++) {if(_quadP[q]->a.value != _quadP[0]->a.value) {diffRows++; break;}}
#else
#define CHECKQUADMEMBER(a) {}
#endif

  CHECKQUADMEMBER(_dcdcEn);
  CHECKQUADMEMBER(_asicAnaEn);
  CHECKQUADMEMBER(_asicDigEn);
  CHECKQUADMEMBER(_ddrVttEn);
  CHECKQUADMEMBER(_trigSrcSel);
  CHECKQUADMEMBER(_acqToAsicR0Delay);
  CHECKQUADMEMBER(_asicR0Width);
  CHECKQUADMEMBER(_asicR0ToAsicAcq);
  CHECKQUADMEMBER(_asicAcqWidth);
  CHECKQUADMEMBER(_asicAcqLToPPmatL);
  CHECKQUADMEMBER(_asicRoClkHalfT);
  CHECKQUADMEMBER(_asicAcqForce);
  CHECKQUADMEMBER(_asicAcqValue);
  CHECKQUADMEMBER(_asicR0Force);
  CHECKQUADMEMBER(_asicR0Value);
  CHECKQUADMEMBER(_asicPPmatForce);
  CHECKQUADMEMBER(_asicPPmatValue);
  CHECKQUADMEMBER(_asicSyncForce);
  CHECKQUADMEMBER(_asicSyncValue);
  CHECKQUADMEMBER(_asicRoClkForce);
  CHECKQUADMEMBER(_asicRoClkValue);
  CHECKQUADMEMBER(_adcPipelineDelay);
  CHECKQUADMEMBER(_testData);
  CHECKQUADMEMBER(_scopeEnable);
  CHECKQUADMEMBER(_scopeTrigEdge);
  CHECKQUADMEMBER(_scopeTrigChan);
  CHECKQUADMEMBER(_scopeTrigMode);
  CHECKQUADMEMBER(_scopeAdcThreshold);
  CHECKQUADMEMBER(_scopeTrigHoldoff);
  CHECKQUADMEMBER(_scopeTrigOffset);
  CHECKQUADMEMBER(_scopeTraceLength);
  CHECKQUADMEMBER(_scopeSamplesToSkip);
  CHECKQUADMEMBER(_scopeChanASelect);
  CHECKQUADMEMBER(_scopeChanBSelect);
  CHECKQUADMEMBER(_scopeTrigDelay);

#undef CHECKQUADMEMBER

  if (diffRows==0) return true;
  QString msg = QString("Have found %1 inconsistent rows\n")
    .arg(diffRows);
  switch (QMessageBox::warning(0,"Possible Input Error",msg, "override", "oops", 0, 0, 1))
    {
    case 0:
      return true;
    case 1:
      return false;
    }

  return false;
}

QLayout* ConfigTable::initialize(QWidget* parent) 
{
#define ADDTAB(layout,title) { \
    QWidget*      w = new QWidget; \
    QVBoxLayout* lv = new QVBoxLayout; lv->addLayout(layout); lv->addStretch(1); \
    QHBoxLayout* lh = new QHBoxLayout; lh->addLayout(lv); lh->addStretch(1); \
    w->setLayout(lh); tab->addTab(w,title); } 

  QHBoxLayout* layout = new QHBoxLayout;
  { QTabWidget* tab = new QTabWidget;
    { QVBoxLayout* gl = new QVBoxLayout;
      _globalP->initialize(parent,gl);
      ADDTAB(gl,"Global");
    { QGridLayout* gl_sys = new QGridLayout;
      QGridLayout* gl_acq = new QGridLayout;
      QGridLayout* gl_sco = new QGridLayout;
      QGridLayout* gl_adc[40];  for(unsigned i=0; i<40; i++) gl_adc[i] = new QGridLayout;
      Epix10kaASICdata::setColumns(4);
      QLayout* gl_asi[64];
      for(unsigned a=0; a<64; a++)
        gl_asi[a] = _globalP->_asic[a].initialize(parent);

      QuadP::initialize_tabs(parent,gl_sys,gl_acq,gl_sco,gl_adc,gl_asi);
      for(unsigned q=0; q<4; q++)
        _quadP[q]->initialize(parent,gl_sys,gl_acq,gl_sco,&gl_adc[q*10],&gl_asi[q*16],q);
      ADDTAB(gl_sys,"System");
      ADDTAB(gl_acq,"Acq+Rdo");
      ADDTAB(gl_sco,"Scope");

      QTabWidget* tab_adc = new QTabWidget;
      tab_adc->setTabPosition(QTabWidget::West);
      for(unsigned q=0; q<4; q++) {
        QTabWidget* quadTab = new QTabWidget;
        quadTab->setTabPosition(QTabWidget::North);
        for(unsigned ch=0; ch<10; ch++) {
          QWidget* w = new QWidget;
          w->setLayout(gl_adc[q*10+ch]);
          quadTab->addTab(w,QString("Chip %1").arg(ch));
        }
        tab_adc->addTab(quadTab,QString("Quad %1").arg(q));
      }
      { QVBoxLayout* vl = new QVBoxLayout;
        vl->addWidget(tab_adc);
        vl->addWidget(new AdcCopyBox(*this));
        vl->addStretch(1);
        ADDTAB(vl, "ADC"); }
      QTabWidget* tab_asi = new QTabWidget;
      tab_asi->setTabPosition(QTabWidget::West);
      for(unsigned q=0; q<4; q++) {
        QTabWidget* quadTab = new QTabWidget;
        quadTab->setTabPosition(QTabWidget::North);
        for(unsigned ch=0; ch<16; ch++) {
          QWidget* w = new QWidget;
          w->setLayout(gl_asi[q*16+ch]);
          quadTab->addTab(w,QString("A%1").arg(ch));
        }
        tab_asi->addTab(quadTab,QString("Quad %1").arg(q));
      }
      { QVBoxLayout* vl = new QVBoxLayout;
        vl->addWidget(tab_asi);
        vl->addWidget(new AsicCopyBox(*this));
        ADDTAB(vl, "ASICs"); }
    }
    { QVBoxLayout* vl = new QVBoxLayout;
      vl->addWidget(_pixelMap  = new Epix10ka2MMap(2,true));
      vl->addWidget(_pixelMapB = new QPushButton("Pixel Maps"));
      vl->addWidget(_calibMapB = new QPushButton("Calib Maps"));
      ADDTAB(vl, "Maps"); }
    }
#undef ADDTAB
    layout->addWidget(tab); }

  update_maskg();
  _pixelMap->update(0);

  _qlink = new ConfigTableQ(*this,parent);
  ::QObject::connect(_globalP->_asicMaskMap, SIGNAL(changed()), _qlink, SLOT(update_maskv()));
  ::QObject::connect(_globalP->_asicMask._input, SIGNAL(editingFinished()), _qlink, SLOT(update_maskg()));
  ::QObject::connect(_pixelMapB, SIGNAL(pressed()), _qlink, SLOT(pixel_map_dialog()));
  ::QObject::connect(_calibMapB, SIGNAL(pressed()), _qlink, SLOT(calib_map_dialog()));

  return layout;
#undef ADDTAB
}

void ConfigTable::update() {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->update();
    p = p->forward();
  }
  update_maskg();
}

void ConfigTable::flush () {
  Parameter* p = _pList.forward();
  while( p != _pList.empty() ) {
    p->flush();
    p = p->forward();
  }
  update_maskg();
}

void ConfigTable::enable(bool) 
{
}

void ConfigTable::update_maskg()
{
  _globalP->_asicMaskMap->update(_globalP->_asicMask.value);
}

void ConfigTable::update_maskv()
{
  _globalP->_asicMask.value = _globalP->_asicMaskMap->value();
  _globalP->_asicMask.flush();
}

void ConfigTable::pixel_map_dialog()
{
  uint64_t v = _pixelMap->value();
  for(unsigned i=0; i<16; i++)
    if ( ((v>>(4*i))&0xf)!=0 ) {
      _pixelMapD[i]->show_map();
      _pixelMapD[i]->exec();
      return;
    }
}

void ConfigTable::calib_map_dialog()
{
  uint64_t v = _pixelMap->value();
  for(unsigned i=0; i<4; i++)
    if ( ((v>>(16*i))&0xffff)!=0 ) {
      _calibMapD[i]->show_map();
      _calibMapD[i]->exec();
      return;
    }
}

// ConfigTableQ::ConfigTableQ(GlobalP& table,
//                                                QWidget* parent) :
//   QObject(parent),
//   _table (table)
// {
// }

// void ConfigTableQ::update_readout() { _table.update_readout(); }

#include "Parameters.icc"


Pds_ConfigDb::Epix10ka2MConfig::Epix10ka2MConfig():
  Serializer("Epix10ka2M_Config"), _table(new ConfigTable(*this))
{
  _table->insert(pList);
}

Pds_ConfigDb::Epix10ka2MConfig::~Epix10ka2MConfig() {}

int Pds_ConfigDb::Epix10ka2MConfig::readParameters(void *from)
{
  return _table->pull(*reinterpret_cast<const Epix10ka2MConfigType*>(from));
}

int Pds_ConfigDb::Epix10ka2MConfig::writeParameters(void *to)
{
  return _table->push(to);
}

int Pds_ConfigDb::Epix10ka2MConfig::dataSize() const
{
  return _table->dataSize();
}

bool Pds_ConfigDb::Epix10ka2MConfig::validate() 
{
  return _table->validate();
}


ConfigTableQ::ConfigTableQ(ConfigTable& table, QObject* parent) : 
  QObject(parent),
  _table(table) {}

void ConfigTableQ::update_maskv    () { _table.update_maskv    (); }
void ConfigTableQ::update_maskg    () { _table.update_maskg    (); }
void ConfigTableQ::pixel_map_dialog() { _table.pixel_map_dialog(); }
void ConfigTableQ::calib_map_dialog() { _table.calib_map_dialog(); }


