#include "pdsapp/config/Epix10kaQuad.hh"
#include "pdsapp/config/Epix10kaASICdata.hh"
#include "pdsapp/config/Epix10kaCalibMap.hh"
#include "pdsapp/config/Epix10kaPixelMap.hh"
#include "pdsapp/config/Parameters.hh"
#include "pds/config/EpixConfigType.hh"

#include <stdio.h>
#include <QtCore/QObject>

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

static const unsigned pixelArrayShape[] = {Pds::Epix::Config10ka::_numberOfRowsPerAsic *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerColumn,
                                           Pds::Epix::Config10ka::_numberOfPixelsPerAsicRow *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerRow};

static const unsigned calibArrayShape[] = {Pds::Epix::Config10ka::_calibrationRowCountPerASIC *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerColumn,
                                           Pds::Epix::Config10ka::_numberOfPixelsPerAsicRow *
                                           Pds::Epix::Config10ka::_numberOfAsicsPerRow};

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

using namespace Pds_ConfigDb::Epix10kaQuad;

AD9249Display::AD9249Display() :
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

void AD9249Display::initializeTab(QWidget* parent, QGridLayout* gl)
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

void AD9249Display::initialize(QWidget* parent, QGridLayout* gl)
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

void AD9249Display::insert(Pds::LinkedList<Parameter>& pList)
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

void AD9249Display::pull(const Pds::Epix::Ad9249Config& p)
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

void AD9249Display::push(Pds::Epix::Ad9249Config& p)
{
  __attribute__((unused)) Pds::Epix::Ad9249Config& c =
    *new(&p) Pds::Epix::Ad9249Config( 0,
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

void QuadP::initialize_tabs(QWidget*     parent,
                            QGridLayout* gl_sys,
                            QGridLayout* gl_acq,
                            QGridLayout* gl_sco,
                            QGridLayout** gl_adc,
                            QLayout**     gl_asi,
                            unsigned      nq) {
  for(unsigned i=0; i<nq; i++) {
    gl_sys->addWidget(new QLabel(QString("Quad %1").arg(i)), 0, i+1);
    gl_acq->addWidget(new QLabel(QString("Quad %1").arg(i)), 0, i+1);
    gl_sco->addWidget(new QLabel(QString("Quad %1").arg(i)), 0, i+1);
  }
  unsigned row=1;
  gl_sys->addWidget(new QLabel("digitalId0"),row++,0);
  gl_sys->addWidget(new QLabel("digitalId1"),row++,0);
  gl_sys->addWidget(new QLabel("dcdcEn"    ),row++,0);
  gl_sys->addWidget(new QLabel("asicAnaEn" ),row++,0);
  gl_sys->addWidget(new QLabel("asicDigEn" ),row++,0);
  gl_sys->addWidget(new QLabel("ddrVttEn"  ),row++,0);
  gl_sys->addWidget(new QLabel("trigSrcSel"),row++,0);
  gl_sys->addWidget(new QLabel("vguardDac" ),row++,0);
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

  for(unsigned ch=0; ch<10*nq; ch++) 
    AD9249Display::initializeTab(parent,gl_adc[ch]);
}

QuadP::QuadP() :
  //  SystemRegs
  _digitalCardId0     ( NULL, 0,      0, 0xffffffff, Hex),
  _digitalCardId1     ( NULL, 0,      0, 0xffffffff, Hex),
  _dcdcEn             ( NULL, 0xf,    0, 0xf, Hex),
  _asicAnaEn          ( NULL, 0,      0, 1, Decimal),
  _asicDigEn          ( NULL, 0,      0, 1, Decimal),
  _ddrVttEn           ( NULL, 0,      0, 1, Decimal),
  _trigSrcSel         ( NULL, 0,      0, 3, Decimal),
  _vguardDac          ( NULL, 0,      0, 0xffff, Decimal),
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

void QuadP::enable(bool v)
{
}

void QuadP::reset ()
{
}

void QuadP::pull   (const Epix10kaQuadConfig& p)
{
  //  AxiVersion (RO)
  _digitalCardId0     .value = p.digitalCardId0     ();
  _digitalCardId1     .value = p.digitalCardId1     ();
  //  System
  _dcdcEn             .value = p.dcdcEn             ();
  _asicAnaEn          .value = p.asicAnaEn          ();
  _asicDigEn          .value = p.asicDigEn          ();
  _ddrVttEn           .value = p.ddrVttEn           ();
  _trigSrcSel         .value = p.trigSrcSel         ();
  _vguardDac          .value = p.vguardDac          ();
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

void QuadP::push   (Epix10kaQuadConfig* p)
{
  Pds::Epix::Ad9249Config     adcs[10];
  for(unsigned ch=0; ch<10; ch++)
    _adc[ch].push(adcs[ch]);

  __attribute__((unused)) Epix10kaQuadConfig& c =
  *new(p) Epix10kaQuadConfig    (125000000,  // baseClockFreq
                                 0,          // enableAutomaticRunTrigger
                                 125000000/120,  // numberOf125MhzTicksPerRunTrigger
                                 _digitalCardId0     .value,
                                 _digitalCardId1     .value,
                                 _dcdcEn             .value,
                                 _asicAnaEn          .value,
                                 _asicDigEn          .value,
                                 _ddrVttEn           .value,
                                 _trigSrcSel         .value,
                                 _vguardDac          .value,
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

void QuadP::initialize(QWidget* parent, QGridLayout* gl_sys, QGridLayout* gl_acq, QGridLayout* gl_sco, QGridLayout** gl_adc, QLayout** gl_asi, unsigned q)
{
  unsigned row=1;
  gl_sys->addLayout(_digitalCardId0.initialize(parent),row++,q+1);
  gl_sys->addLayout(_digitalCardId1.initialize(parent),row++,q+1);
  gl_sys->addLayout(_dcdcEn    .initialize(parent),row++,q+1);
  gl_sys->addLayout(_asicAnaEn .initialize(parent),row++,q+1);
  gl_sys->addLayout(_asicDigEn .initialize(parent),row++,q+1);
  gl_sys->addLayout(_ddrVttEn  .initialize(parent),row++,q+1);
  gl_sys->addLayout(_trigSrcSel.initialize(parent),row++,q+1);
  gl_sys->addLayout(_vguardDac .initialize(parent),row++,q+1);
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

void QuadP::insert(Pds::LinkedList<Parameter>& pList) {
#define ADDP(t) pList.insert(&t)
  ADDP(_digitalCardId0);
  ADDP(_digitalCardId1);
  ADDP(_dcdcEn);
  ADDP(_asicAnaEn);
  ADDP(_asicDigEn);
  ADDP(_ddrVttEn);
  ADDP(_trigSrcSel);
  ADDP(_vguardDac);
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

template class Pds_ConfigDb::Enumerated<Values>;
template class Pds_ConfigDb::NumericIntArray<uint32_t,8>;

#include "Parameters.icc"
