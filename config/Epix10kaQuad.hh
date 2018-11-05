#ifndef PdsApp_Configdb_Epix10kaQuad_hh
#define PdsApp_Configdb_Epix10kaQuad_hh

#include "pdsapp/config/Parameters.hh"
#include "pds/config/EpixConfigType.hh"

class QWidget;
class QGridLayout;
class QLayout;

enum Values { _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10 };

namespace Pds_ConfigDb {
  namespace Epix10kaQuad {
    class AD9249Display {
    public:
      AD9249Display();
    public:
      static void initializeTab(QWidget* parent, QGridLayout* gl);
      void initialize(QWidget* parent, QGridLayout* gl);
      void insert    (Pds::LinkedList<Parameter>& pList);
      void pull      (const Pds::Epix::Ad9249Config& p);
      void push      (Pds::Epix::Ad9249Config& p);
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
                                  QLayout**     gl_asi,
                                  unsigned      nq=4);
    public:
      QuadP();
    public:
      void enable(bool v);
      void reset ();
      void pull   (const Epix10kaQuadConfig& p);
      void push   (Epix10kaQuadConfig* p);
    public:
      void initialize(QWidget* parent, QGridLayout* gl_sys, QGridLayout* gl_acq, QGridLayout* gl_sco, QGridLayout** gl_adc, QLayout** gl_asi, unsigned q);
      void insert(Pds::LinkedList<Parameter>& pList);
    public:
      NumericInt<uint32_t> _dcdcEn;
      NumericInt<uint32_t> _asicAnaEn;
      NumericInt<uint32_t> _asicDigEn;
      NumericInt<uint32_t> _ddrVttEn;
      NumericInt<uint32_t> _trigSrcSel;
      NumericInt<uint32_t> _vguardDac;
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
  };
};

#endif
