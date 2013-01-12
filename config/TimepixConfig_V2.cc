// $Id$

#include "pdsapp/config/TimepixConfig_V2.hh"
#include "pdsapp/config/TimepixConfigDefaults.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsdata/timepix/ConfigV2.hh"

#include <new>

namespace Pds_ConfigDb {

  // these must end in NULL
  static const char* readoutSpeed_to_name[] = { "62.5 MHz", "125 MHz", NULL };
  static const char* timepixSpeed_to_name[] = { "100 MHz", "80 MHz", "40 MHz", "10 MHz", "2.5 MHz", NULL };

  // ------- expert mode ---------
  class TimepixExpertConfig_V2::Private_Data {
  public:
    Private_Data() :
      // readout speed is NOT frequently changed
      _readoutSpeed   ("Chip readout speed", Pds::Timepix::ConfigV2::ReadoutSpeed_Fast, readoutSpeed_to_name),

      // the following six values are frequently changed
      _timepixSpeed   ("Timepix speed", 0 /* 100 MHz */, timepixSpeed_to_name),
      _dac0ThlFine    ("DAC0 thl fine",     TIMEPIX_DAC_THLFINE_DEFAULT,    0,1023),
      _dac1ThlFine    ("DAC1 thl fine",     TIMEPIX_DAC_THLFINE_DEFAULT,    0,1023),
      _dac2ThlFine    ("DAC2 thl fine",     TIMEPIX_DAC_THLFINE_DEFAULT,    0,1023),
      _dac3ThlFine    ("DAC3 thl fine",     TIMEPIX_DAC_THLFINE_DEFAULT,    0,1023),
      _timepixMode    ("Timepix mode (0=Count, 1=TOT)", 1,                  0,   1),

      // remaining values are NOT frequently changed
      _dac0Ikrum      ("DAC0 ikrum",        TIMEPIX_DAC_IKRUM_DEFAULT,      0, 255),
      _dac0Disc       ("DAC0 disc",         TIMEPIX_DAC_DISC_DEFAULT,       0, 255),
      _dac0Preamp     ("DAC0 preamp",       TIMEPIX_DAC_PREAMP_DEFAULT,     0, 255),
      _dac0BufAnalogA ("DAC0 buf analog A", TIMEPIX_DAC_BUFANALOGA_DEFAULT, 0, 255),
      _dac0BufAnalogB ("DAC0 buf analog B", TIMEPIX_DAC_BUFANALOGB_DEFAULT, 0, 255),
      _dac0Hist       ("DAC0 hist",         TIMEPIX_DAC_HIST_DEFAULT,       0, 255),
      _dac0ThlCourse  ("DAC0 thl course",   TIMEPIX_DAC_THLCOURSE_DEFAULT,  0,  15),
      _dac0Vcas       ("DAC0 vcas",         TIMEPIX_DAC_VCAS_DEFAULT,       0, 255),
      _dac0Fbk        ("DAC0 fbk",          TIMEPIX_DAC_FBK_DEFAULT,        0, 255),
      _dac0Gnd        ("DAC0 gnd",          TIMEPIX_DAC_GND_DEFAULT,        0, 255),
      _dac0Ths        ("DAC0 ths",          TIMEPIX_DAC_THS_DEFAULT,        0, 255),
      _dac0BiasLvds   ("DAC0 bias lvds",    TIMEPIX_DAC_BIASLVDS_DEFAULT,   0, 255),
      _dac0RefLvds    ("DAC0 ref lvds",     TIMEPIX_DAC_REFLVDS_DEFAULT,    0, 255),
      _dac1Ikrum      ("DAC1 ikrum",        TIMEPIX_DAC_IKRUM_DEFAULT,      0, 255),
      _dac1Disc       ("DAC1 disc",         TIMEPIX_DAC_DISC_DEFAULT,       0, 255),
      _dac1Preamp     ("DAC1 preamp",       TIMEPIX_DAC_PREAMP_DEFAULT,     0, 255),
      _dac1BufAnalogA ("DAC1 buf analog A", TIMEPIX_DAC_BUFANALOGA_DEFAULT, 0, 255),
      _dac1BufAnalogB ("DAC1 buf analog B", TIMEPIX_DAC_BUFANALOGB_DEFAULT, 0, 255),
      _dac1Hist       ("DAC1 hist",         TIMEPIX_DAC_HIST_DEFAULT,       0, 255),
      _dac1ThlCourse  ("DAC1 thl course",   TIMEPIX_DAC_THLCOURSE_DEFAULT,  0,  15),
      _dac1Vcas       ("DAC1 vcas",         TIMEPIX_DAC_VCAS_DEFAULT,       0, 255),
      _dac1Fbk        ("DAC1 fbk",          TIMEPIX_DAC_FBK_DEFAULT,        0, 255),
      _dac1Gnd        ("DAC1 gnd",          TIMEPIX_DAC_GND_DEFAULT,        0, 255),
      _dac1Ths        ("DAC1 ths",          TIMEPIX_DAC_THS_DEFAULT,        0, 255),
      _dac1BiasLvds   ("DAC1 bias lvds",    TIMEPIX_DAC_BIASLVDS_DEFAULT,   0, 255),
      _dac1RefLvds    ("DAC1 ref lvds",     TIMEPIX_DAC_REFLVDS_DEFAULT,    0, 255),
      _dac2Ikrum      ("DAC2 ikrum",        TIMEPIX_DAC_IKRUM_DEFAULT,      0, 255),
      _dac2Disc       ("DAC2 disc",         TIMEPIX_DAC_DISC_DEFAULT,       0, 255),
      _dac2Preamp     ("DAC2 preamp",       TIMEPIX_DAC_PREAMP_DEFAULT,     0, 255),
      _dac2BufAnalogA ("DAC2 buf analog A", TIMEPIX_DAC_BUFANALOGA_DEFAULT, 0, 255),
      _dac2BufAnalogB ("DAC2 buf analog B", TIMEPIX_DAC_BUFANALOGB_DEFAULT, 0, 255),
      _dac2Hist       ("DAC2 hist",         TIMEPIX_DAC_HIST_DEFAULT,       0, 255),
      _dac2ThlCourse  ("DAC2 thl course",   TIMEPIX_DAC_THLCOURSE_DEFAULT,  0,  15),
      _dac2Vcas       ("DAC2 vcas",         TIMEPIX_DAC_VCAS_DEFAULT,       0, 255),
      _dac2Fbk        ("DAC2 fbk",          TIMEPIX_DAC_FBK_DEFAULT,        0, 255),
      _dac2Gnd        ("DAC2 gnd",          TIMEPIX_DAC_GND_DEFAULT,        0, 255),
      _dac2Ths        ("DAC2 ths",          TIMEPIX_DAC_THS_DEFAULT,        0, 255),
      _dac2BiasLvds   ("DAC2 bias lvds",    TIMEPIX_DAC_BIASLVDS_DEFAULT,   0, 255),
      _dac2RefLvds    ("DAC2 ref lvds",     TIMEPIX_DAC_REFLVDS_DEFAULT,    0, 255),
      _dac3Ikrum      ("DAC3 ikrum",        TIMEPIX_DAC_IKRUM_DEFAULT,      0, 255),
      _dac3Disc       ("DAC3 disc",         TIMEPIX_DAC_DISC_DEFAULT,       0, 255),
      _dac3Preamp     ("DAC3 preamp",       TIMEPIX_DAC_PREAMP_DEFAULT,     0, 255),
      _dac3BufAnalogA ("DAC3 buf analog A", TIMEPIX_DAC_BUFANALOGA_DEFAULT, 0, 255),
      _dac3BufAnalogB ("DAC3 buf analog B", TIMEPIX_DAC_BUFANALOGB_DEFAULT, 0, 255),
      _dac3Hist       ("DAC3 hist",         TIMEPIX_DAC_HIST_DEFAULT,       0, 255),
      _dac3ThlCourse  ("DAC3 thl course",   TIMEPIX_DAC_THLCOURSE_DEFAULT,  0,  15),
      _dac3Vcas       ("DAC3 vcas",         TIMEPIX_DAC_VCAS_DEFAULT,       0, 255),
      _dac3Fbk        ("DAC3 fbk",          TIMEPIX_DAC_FBK_DEFAULT,        0, 255),
      _dac3Gnd        ("DAC3 gnd",          TIMEPIX_DAC_GND_DEFAULT,        0, 255),
      _dac3Ths        ("DAC3 ths",          TIMEPIX_DAC_THS_DEFAULT,        0, 255),
      _dac3BiasLvds   ("DAC3 bias lvds",    TIMEPIX_DAC_BIASLVDS_DEFAULT,   0, 255),
      _dac3RefLvds    ("DAC3 ref lvds",     TIMEPIX_DAC_REFLVDS_DEFAULT,    0, 255)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_readoutSpeed);
      pList.insert(&_timepixSpeed);
      // the following four values are frequently changed
      pList.insert(&_dac0ThlFine);
      pList.insert(&_dac1ThlFine);
      pList.insert(&_dac2ThlFine);
      pList.insert(&_dac3ThlFine);
      // the mode is frequently changed
      pList.insert(&_timepixMode);
      // remaining values are NOT frequently changed
      pList.insert(&_dac0Ikrum);
      pList.insert(&_dac0Disc);
      pList.insert(&_dac0Preamp);
      pList.insert(&_dac0BufAnalogA);
      pList.insert(&_dac0BufAnalogB);
      pList.insert(&_dac0Hist);
      pList.insert(&_dac0ThlCourse);
      pList.insert(&_dac0Vcas);
      pList.insert(&_dac0Fbk);
      pList.insert(&_dac0Gnd);
      pList.insert(&_dac0Ths);
      pList.insert(&_dac0BiasLvds);
      pList.insert(&_dac0RefLvds);
      pList.insert(&_dac1Ikrum);
      pList.insert(&_dac1Disc);
      pList.insert(&_dac1Preamp);
      pList.insert(&_dac1BufAnalogA);
      pList.insert(&_dac1BufAnalogB);
      pList.insert(&_dac1Hist);
      pList.insert(&_dac1ThlCourse);
      pList.insert(&_dac1Vcas);
      pList.insert(&_dac1Fbk);
      pList.insert(&_dac1Gnd);
      pList.insert(&_dac1Ths);
      pList.insert(&_dac1BiasLvds);
      pList.insert(&_dac1RefLvds);
      pList.insert(&_dac2Ikrum);
      pList.insert(&_dac2Disc);
      pList.insert(&_dac2Preamp);
      pList.insert(&_dac2BufAnalogA);
      pList.insert(&_dac2BufAnalogB);
      pList.insert(&_dac2Hist);
      pList.insert(&_dac2ThlCourse);
      pList.insert(&_dac2Vcas);
      pList.insert(&_dac2Fbk);
      pList.insert(&_dac2Gnd);
      pList.insert(&_dac2Ths);
      pList.insert(&_dac2BiasLvds);
      pList.insert(&_dac2RefLvds);
      pList.insert(&_dac3Ikrum);
      pList.insert(&_dac3Disc);
      pList.insert(&_dac3Preamp);
      pList.insert(&_dac3BufAnalogA);
      pList.insert(&_dac3BufAnalogB);
      pList.insert(&_dac3Hist);
      pList.insert(&_dac3ThlCourse);
      pList.insert(&_dac3Vcas);
      pList.insert(&_dac3Fbk);
      pList.insert(&_dac3Gnd);
      pList.insert(&_dac3Ths);
      pList.insert(&_dac3BiasLvds);
      pList.insert(&_dac3RefLvds);
    }

    int pull(void* from) {
      Pds::Timepix::ConfigV2& tc = *new(from) Pds::Timepix::ConfigV2;

      _readoutSpeed.value = (Pds::Timepix::ConfigV2::ReadoutSpeed)tc.readoutSpeed();
      _timepixSpeed.value = tc.timepixSpeed();
      _dac0Ikrum.value = tc.dac0Ikrum();
      _dac0Disc.value = tc.dac0Disc();
      _dac0Preamp.value = tc.dac0Preamp();
      _dac0BufAnalogA.value = tc.dac0BufAnalogA();
      _dac0BufAnalogB.value = tc.dac0BufAnalogB();
      _dac0Hist.value = tc.dac0Hist();
      _dac0ThlFine.value = tc.dac0ThlFine();
      _dac0ThlCourse.value = tc.dac0ThlCourse();
      _dac0Vcas.value = tc.dac0Vcas();
      _dac0Fbk.value = tc.dac0Fbk();
      _dac0Gnd.value = tc.dac0Gnd();
      _dac0Ths.value = tc.dac0Ths();
      _dac0BiasLvds.value = tc.dac0BiasLvds();
      _dac0RefLvds.value = tc.dac0RefLvds();
      _dac1Ikrum.value = tc.dac1Ikrum();
      _dac1Disc.value = tc.dac1Disc();
      _dac1Preamp.value = tc.dac1Preamp();
      _dac1BufAnalogA.value = tc.dac1BufAnalogA();
      _dac1BufAnalogB.value = tc.dac1BufAnalogB();
      _dac1Hist.value = tc.dac1Hist();
      _dac1ThlFine.value = tc.dac1ThlFine();
      _dac1ThlCourse.value = tc.dac1ThlCourse();
      _dac1Vcas.value = tc.dac1Vcas();
      _dac1Fbk.value = tc.dac1Fbk();
      _dac1Gnd.value = tc.dac1Gnd();
      _dac1Ths.value = tc.dac1Ths();
      _dac1BiasLvds.value = tc.dac1BiasLvds();
      _dac1RefLvds.value = tc.dac1RefLvds();
      _dac2Ikrum.value = tc.dac2Ikrum();
      _dac2Disc.value = tc.dac2Disc();
      _dac2Preamp.value = tc.dac2Preamp();
      _dac2BufAnalogA.value = tc.dac2BufAnalogA();
      _dac2BufAnalogB.value = tc.dac2BufAnalogB();
      _dac2Hist.value = tc.dac2Hist();
      _dac2ThlFine.value = tc.dac2ThlFine();
      _dac2ThlCourse.value = tc.dac2ThlCourse();
      _dac2Vcas.value = tc.dac2Vcas();
      _dac2Fbk.value = tc.dac2Fbk();
      _dac2Gnd.value = tc.dac2Gnd();
      _dac2Ths.value = tc.dac2Ths();
      _dac2BiasLvds.value = tc.dac2BiasLvds();
      _dac2RefLvds.value = tc.dac2RefLvds();
      _dac3Ikrum.value = tc.dac3Ikrum();
      _dac3Disc.value = tc.dac3Disc();
      _dac3Preamp.value = tc.dac3Preamp();
      _dac3BufAnalogA.value = tc.dac3BufAnalogA();
      _dac3BufAnalogB.value = tc.dac3BufAnalogB();
      _dac3Hist.value = tc.dac3Hist();
      _dac3ThlFine.value = tc.dac3ThlFine();
      _dac3ThlCourse.value = tc.dac3ThlCourse();
      _dac3Vcas.value = tc.dac3Vcas();
      _dac3Fbk.value = tc.dac3Fbk();
      _dac3Gnd.value = tc.dac3Gnd();
      _dac3Ths.value = tc.dac3Ths();
      _dac3BiasLvds.value = tc.dac3BiasLvds();
      _dac3RefLvds.value = tc.dac3RefLvds();
      _timepixMode.value = tc.triggerMode();  // ext/neg assumed for trigger, so pass timepix mode here
      return tc.size();
    }

    int push(void* to) {
      Pds::Timepix::ConfigV2& tc = *new(to) Pds::Timepix::ConfigV2(
        _readoutSpeed.value,
        _timepixMode.value,   // ext/neg assumed for trigger, so pass timepix mode here
        _timepixSpeed.value,
        _dac0Ikrum.value,
        _dac0Disc.value,
        _dac0Preamp.value,
        _dac0BufAnalogA.value,
        _dac0BufAnalogB.value,
        _dac0Hist.value,
        _dac0ThlFine.value,
        _dac0ThlCourse.value,
        _dac0Vcas.value,
        _dac0Fbk.value,
        _dac0Gnd.value,
        _dac0Ths.value,
        _dac0BiasLvds.value,
        _dac0RefLvds.value,
        _dac1Ikrum.value,
        _dac1Disc.value,
        _dac1Preamp.value,
        _dac1BufAnalogA.value,
        _dac1BufAnalogB.value,
        _dac1Hist.value,
        _dac1ThlFine.value,
        _dac1ThlCourse.value,
        _dac1Vcas.value,
        _dac1Fbk.value,
        _dac1Gnd.value,
        _dac1Ths.value,
        _dac1BiasLvds.value,
        _dac1RefLvds.value,
        _dac2Ikrum.value,
        _dac2Disc.value,
        _dac2Preamp.value,
        _dac2BufAnalogA.value,
        _dac2BufAnalogB.value,
        _dac2Hist.value,
        _dac2ThlFine.value,
        _dac2ThlCourse.value,
        _dac2Vcas.value,
        _dac2Fbk.value,
        _dac2Gnd.value,
        _dac2Ths.value,
        _dac2BiasLvds.value,
        _dac2RefLvds.value,
        _dac3Ikrum.value,
        _dac3Disc.value,
        _dac3Preamp.value,
        _dac3BufAnalogA.value,
        _dac3BufAnalogB.value,
        _dac3Hist.value,
        _dac3ThlFine.value,
        _dac3ThlCourse.value,
        _dac3Vcas.value,
        _dac3Fbk.value,
        _dac3Gnd.value,
        _dac3Ths.value,
        _dac3BiasLvds.value,
        _dac3RefLvds.value
      );

      return tc.size();
    }

    int dataSize() const {
      return sizeof(Pds::Timepix::ConfigV2);
    }

  public:
    Enumerated<Pds::Timepix::ConfigV2::ReadoutSpeed> _readoutSpeed;
    Enumerated<int32_t> _timepixSpeed;
    NumericInt<int32_t> _dac0ThlFine;
    NumericInt<int32_t> _dac1ThlFine;
    NumericInt<int32_t> _dac2ThlFine;
    NumericInt<int32_t> _dac3ThlFine;
    NumericInt<uint8_t> _timepixMode;
    NumericInt<int32_t> _dac0Ikrum;
    NumericInt<int32_t> _dac0Disc;
    NumericInt<int32_t> _dac0Preamp;
    NumericInt<int32_t> _dac0BufAnalogA;
    NumericInt<int32_t> _dac0BufAnalogB;
    NumericInt<int32_t> _dac0Hist;
    NumericInt<int32_t> _dac0ThlCourse;
    NumericInt<int32_t> _dac0Vcas;
    NumericInt<int32_t> _dac0Fbk;
    NumericInt<int32_t> _dac0Gnd;
    NumericInt<int32_t> _dac0Ths;
    NumericInt<int32_t> _dac0BiasLvds;
    NumericInt<int32_t> _dac0RefLvds;
    NumericInt<int32_t> _dac1Ikrum;
    NumericInt<int32_t> _dac1Disc;
    NumericInt<int32_t> _dac1Preamp;
    NumericInt<int32_t> _dac1BufAnalogA;
    NumericInt<int32_t> _dac1BufAnalogB;
    NumericInt<int32_t> _dac1Hist;
    NumericInt<int32_t> _dac1ThlCourse;
    NumericInt<int32_t> _dac1Vcas;
    NumericInt<int32_t> _dac1Fbk;
    NumericInt<int32_t> _dac1Gnd;
    NumericInt<int32_t> _dac1Ths;
    NumericInt<int32_t> _dac1BiasLvds;
    NumericInt<int32_t> _dac1RefLvds;
    NumericInt<int32_t> _dac2Ikrum;
    NumericInt<int32_t> _dac2Disc;
    NumericInt<int32_t> _dac2Preamp;
    NumericInt<int32_t> _dac2BufAnalogA;
    NumericInt<int32_t> _dac2BufAnalogB;
    NumericInt<int32_t> _dac2Hist;
    NumericInt<int32_t> _dac2ThlCourse;
    NumericInt<int32_t> _dac2Vcas;
    NumericInt<int32_t> _dac2Fbk;
    NumericInt<int32_t> _dac2Gnd;
    NumericInt<int32_t> _dac2Ths;
    NumericInt<int32_t> _dac2BiasLvds;
    NumericInt<int32_t> _dac2RefLvds;
    NumericInt<int32_t> _dac3Ikrum;
    NumericInt<int32_t> _dac3Disc;
    NumericInt<int32_t> _dac3Preamp;
    NumericInt<int32_t> _dac3BufAnalogA;
    NumericInt<int32_t> _dac3BufAnalogB;
    NumericInt<int32_t> _dac3Hist;
    NumericInt<int32_t> _dac3ThlCourse;
    NumericInt<int32_t> _dac3Vcas;
    NumericInt<int32_t> _dac3Fbk;
    NumericInt<int32_t> _dac3Gnd;
    NumericInt<int32_t> _dac3Ths;
    NumericInt<int32_t> _dac3BiasLvds;
    NumericInt<int32_t> _dac3RefLvds;
  };

  // ------- user mode ---------
  class TimepixConfig_V2::Private_Data {
  public:
    Private_Data() :
      _readoutSpeed   ("Chip readout speed", Pds::Timepix::ConfigV2::ReadoutSpeed_Fast, readoutSpeed_to_name),

      _timepixSpeed   ("Timepix speed", 0 /* 100 MHz */, timepixSpeed_to_name),

      // the following four values are frequently changed
      _dac0ThlFine    ("DAC0 thl fine",     TIMEPIX_DAC_THLFINE_DEFAULT,    0,1023),
      _dac1ThlFine    ("DAC1 thl fine",     TIMEPIX_DAC_THLFINE_DEFAULT,    0,1023),
      _dac2ThlFine    ("DAC2 thl fine",     TIMEPIX_DAC_THLFINE_DEFAULT,    0,1023),
      _dac3ThlFine    ("DAC3 thl fine",     TIMEPIX_DAC_THLFINE_DEFAULT,    0,1023),
      // the mode is frequently changed
      _timepixMode    ("Timepix mode (0=Count, 1=TOT)", 1,                  0, 1),

      // remaining values are NOT frequently changed
      _dac0Ikrum      ("DAC0 ikrum",        TIMEPIX_DAC_IKRUM_DEFAULT,      0, 255),
      _dac0Disc       ("DAC0 disc",         TIMEPIX_DAC_DISC_DEFAULT,       0, 255),
      _dac0Preamp     ("DAC0 preamp",       TIMEPIX_DAC_PREAMP_DEFAULT,     0, 255),
      _dac0BufAnalogA ("DAC0 buf analog A", TIMEPIX_DAC_BUFANALOGA_DEFAULT, 0, 255),
      _dac0BufAnalogB ("DAC0 buf analog B", TIMEPIX_DAC_BUFANALOGB_DEFAULT, 0, 255),
      _dac0Hist       ("DAC0 hist",         TIMEPIX_DAC_HIST_DEFAULT,       0, 255),
      _dac0ThlCourse  ("DAC0 thl course",   TIMEPIX_DAC_THLCOURSE_DEFAULT,  0,  15),
      _dac0Vcas       ("DAC0 vcas",         TIMEPIX_DAC_VCAS_DEFAULT,       0, 255),
      _dac0Fbk        ("DAC0 fbk",          TIMEPIX_DAC_FBK_DEFAULT,        0, 255),
      _dac0Gnd        ("DAC0 gnd",          TIMEPIX_DAC_GND_DEFAULT,        0, 255),
      _dac0Ths        ("DAC0 ths",          TIMEPIX_DAC_THS_DEFAULT,        0, 255),
      _dac0BiasLvds   ("DAC0 bias lvds",    TIMEPIX_DAC_BIASLVDS_DEFAULT,   0, 255),
      _dac0RefLvds    ("DAC0 ref lvds",     TIMEPIX_DAC_REFLVDS_DEFAULT,    0, 255),
      _dac1Ikrum      ("DAC1 ikrum",        TIMEPIX_DAC_IKRUM_DEFAULT,      0, 255),
      _dac1Disc       ("DAC1 disc",         TIMEPIX_DAC_DISC_DEFAULT,       0, 255),
      _dac1Preamp     ("DAC1 preamp",       TIMEPIX_DAC_PREAMP_DEFAULT,     0, 255),
      _dac1BufAnalogA ("DAC1 buf analog A", TIMEPIX_DAC_BUFANALOGA_DEFAULT, 0, 255),
      _dac1BufAnalogB ("DAC1 buf analog B", TIMEPIX_DAC_BUFANALOGB_DEFAULT, 0, 255),
      _dac1Hist       ("DAC1 hist",         TIMEPIX_DAC_HIST_DEFAULT,       0, 255),
      _dac1ThlCourse  ("DAC1 thl course",   TIMEPIX_DAC_THLCOURSE_DEFAULT,  0,  15),
      _dac1Vcas       ("DAC1 vcas",         TIMEPIX_DAC_VCAS_DEFAULT,       0, 255),
      _dac1Fbk        ("DAC1 fbk",          TIMEPIX_DAC_FBK_DEFAULT,        0, 255),
      _dac1Gnd        ("DAC1 gnd",          TIMEPIX_DAC_GND_DEFAULT,        0, 255),
      _dac1Ths        ("DAC1 ths",          TIMEPIX_DAC_THS_DEFAULT,        0, 255),
      _dac1BiasLvds   ("DAC1 bias lvds",    TIMEPIX_DAC_BIASLVDS_DEFAULT,   0, 255),
      _dac1RefLvds    ("DAC1 ref lvds",     TIMEPIX_DAC_REFLVDS_DEFAULT,    0, 255),
      _dac2Ikrum      ("DAC2 ikrum",        TIMEPIX_DAC_IKRUM_DEFAULT,      0, 255),
      _dac2Disc       ("DAC2 disc",         TIMEPIX_DAC_DISC_DEFAULT,       0, 255),
      _dac2Preamp     ("DAC2 preamp",       TIMEPIX_DAC_PREAMP_DEFAULT,     0, 255),
      _dac2BufAnalogA ("DAC2 buf analog A", TIMEPIX_DAC_BUFANALOGA_DEFAULT, 0, 255),
      _dac2BufAnalogB ("DAC2 buf analog B", TIMEPIX_DAC_BUFANALOGB_DEFAULT, 0, 255),
      _dac2Hist       ("DAC2 hist",         TIMEPIX_DAC_HIST_DEFAULT,       0, 255),
      _dac2ThlCourse  ("DAC2 thl course",   TIMEPIX_DAC_THLCOURSE_DEFAULT,  0,  15),
      _dac2Vcas       ("DAC2 vcas",         TIMEPIX_DAC_VCAS_DEFAULT,       0, 255),
      _dac2Fbk        ("DAC2 fbk",          TIMEPIX_DAC_FBK_DEFAULT,        0, 255),
      _dac2Gnd        ("DAC2 gnd",          TIMEPIX_DAC_GND_DEFAULT,        0, 255),
      _dac2Ths        ("DAC2 ths",          TIMEPIX_DAC_THS_DEFAULT,        0, 255),
      _dac2BiasLvds   ("DAC2 bias lvds",    TIMEPIX_DAC_BIASLVDS_DEFAULT,   0, 255),
      _dac2RefLvds    ("DAC2 ref lvds",     TIMEPIX_DAC_REFLVDS_DEFAULT,    0, 255),
      _dac3Ikrum      ("DAC3 ikrum",        TIMEPIX_DAC_IKRUM_DEFAULT,      0, 255),
      _dac3Disc       ("DAC3 disc",         TIMEPIX_DAC_DISC_DEFAULT,       0, 255),
      _dac3Preamp     ("DAC3 preamp",       TIMEPIX_DAC_PREAMP_DEFAULT,     0, 255),
      _dac3BufAnalogA ("DAC3 buf analog A", TIMEPIX_DAC_BUFANALOGA_DEFAULT, 0, 255),
      _dac3BufAnalogB ("DAC3 buf analog B", TIMEPIX_DAC_BUFANALOGB_DEFAULT, 0, 255),
      _dac3Hist       ("DAC3 hist",         TIMEPIX_DAC_HIST_DEFAULT,       0, 255),
      _dac3ThlCourse  ("DAC3 thl course",   TIMEPIX_DAC_THLCOURSE_DEFAULT,  0,  15),
      _dac3Vcas       ("DAC3 vcas",         TIMEPIX_DAC_VCAS_DEFAULT,       0, 255),
      _dac3Fbk        ("DAC3 fbk",          TIMEPIX_DAC_FBK_DEFAULT,        0, 255),
      _dac3Gnd        ("DAC3 gnd",          TIMEPIX_DAC_GND_DEFAULT,        0, 255),
      _dac3Ths        ("DAC3 ths",          TIMEPIX_DAC_THS_DEFAULT,        0, 255),
      _dac3BiasLvds   ("DAC3 bias lvds",    TIMEPIX_DAC_BIASLVDS_DEFAULT,   0, 255),
      _dac3RefLvds    ("DAC3 ref lvds",     TIMEPIX_DAC_REFLVDS_DEFAULT,    0, 255)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_timepixSpeed);   // "Timepix speed"
      pList.insert(&_dac0ThlFine);
      pList.insert(&_dac1ThlFine);
      pList.insert(&_dac2ThlFine);
      pList.insert(&_dac3ThlFine);
      pList.insert(&_timepixMode);
    }

    int pull(void* from) {
      Pds::Timepix::ConfigV2& tc = *new(from) Pds::Timepix::ConfigV2;


      _readoutSpeed.value = (Pds::Timepix::ConfigV2::ReadoutSpeed)tc.readoutSpeed();
      _timepixSpeed.value = tc.timepixSpeed();
      _dac0Ikrum.value = tc.dac0Ikrum();
      _dac0Disc.value = tc.dac0Disc();
      _dac0Preamp.value = tc.dac0Preamp();
      _dac0BufAnalogA.value = tc.dac0BufAnalogA();
      _dac0BufAnalogB.value = tc.dac0BufAnalogB();
      _dac0Hist.value = tc.dac0Hist();
      _dac0ThlFine.value = tc.dac0ThlFine();
      _dac0ThlCourse.value = tc.dac0ThlCourse();
      _dac0Vcas.value = tc.dac0Vcas();
      _dac0Fbk.value = tc.dac0Fbk();
      _dac0Gnd.value = tc.dac0Gnd();
      _dac0Ths.value = tc.dac0Ths();
      _dac0BiasLvds.value = tc.dac0BiasLvds();
      _dac0RefLvds.value = tc.dac0RefLvds();
      _dac1Ikrum.value = tc.dac1Ikrum();
      _dac1Disc.value = tc.dac1Disc();
      _dac1Preamp.value = tc.dac1Preamp();
      _dac1BufAnalogA.value = tc.dac1BufAnalogA();
      _dac1BufAnalogB.value = tc.dac1BufAnalogB();
      _dac1Hist.value = tc.dac1Hist();
      _dac1ThlFine.value = tc.dac1ThlFine();
      _dac1ThlCourse.value = tc.dac1ThlCourse();
      _dac1Vcas.value = tc.dac1Vcas();
      _dac1Fbk.value = tc.dac1Fbk();
      _dac1Gnd.value = tc.dac1Gnd();
      _dac1Ths.value = tc.dac1Ths();
      _dac1BiasLvds.value = tc.dac1BiasLvds();
      _dac1RefLvds.value = tc.dac1RefLvds();
      _dac2Ikrum.value = tc.dac2Ikrum();
      _dac2Disc.value = tc.dac2Disc();
      _dac2Preamp.value = tc.dac2Preamp();
      _dac2BufAnalogA.value = tc.dac2BufAnalogA();
      _dac2BufAnalogB.value = tc.dac2BufAnalogB();
      _dac2Hist.value = tc.dac2Hist();
      _dac2ThlFine.value = tc.dac2ThlFine();
      _dac2ThlCourse.value = tc.dac2ThlCourse();
      _dac2Vcas.value = tc.dac2Vcas();
      _dac2Fbk.value = tc.dac2Fbk();
      _dac2Gnd.value = tc.dac2Gnd();
      _dac2Ths.value = tc.dac2Ths();
      _dac2BiasLvds.value = tc.dac2BiasLvds();
      _dac2RefLvds.value = tc.dac2RefLvds();
      _dac3Ikrum.value = tc.dac3Ikrum();
      _dac3Disc.value = tc.dac3Disc();
      _dac3Preamp.value = tc.dac3Preamp();
      _dac3BufAnalogA.value = tc.dac3BufAnalogA();
      _dac3BufAnalogB.value = tc.dac3BufAnalogB();
      _dac3Hist.value = tc.dac3Hist();
      _dac3ThlFine.value = tc.dac3ThlFine();
      _dac3ThlCourse.value = tc.dac3ThlCourse();
      _dac3Vcas.value = tc.dac3Vcas();
      _dac3Fbk.value = tc.dac3Fbk();
      _dac3Gnd.value = tc.dac3Gnd();
      _dac3Ths.value = tc.dac3Ths();
      _dac3BiasLvds.value = tc.dac3BiasLvds();
      _dac3RefLvds.value = tc.dac3RefLvds();
      _timepixMode.value = tc.triggerMode();  // ext/neg assumed for trigger, so pass timepix mode here
      return tc.size();
    }

    int push(void* to) {
      Pds::Timepix::ConfigV2& tc = *new(to) Pds::Timepix::ConfigV2(
        _readoutSpeed.value,
        _timepixMode.value,   // ext/neg assumed for trigger, so pass timepix mode here
        _timepixSpeed.value,
        _dac0Ikrum.value,
        _dac0Disc.value,
        _dac0Preamp.value,
        _dac0BufAnalogA.value,
        _dac0BufAnalogB.value,
        _dac0Hist.value,
        _dac0ThlFine.value,
        _dac0ThlCourse.value,
        _dac0Vcas.value,
        _dac0Fbk.value,
        _dac0Gnd.value,
        _dac0Ths.value,
        _dac0BiasLvds.value,
        _dac0RefLvds.value,
        _dac1Ikrum.value,
        _dac1Disc.value,
        _dac1Preamp.value,
        _dac1BufAnalogA.value,
        _dac1BufAnalogB.value,
        _dac1Hist.value,
        _dac1ThlFine.value,
        _dac1ThlCourse.value,
        _dac1Vcas.value,
        _dac1Fbk.value,
        _dac1Gnd.value,
        _dac1Ths.value,
        _dac1BiasLvds.value,
        _dac1RefLvds.value,
        _dac2Ikrum.value,
        _dac2Disc.value,
        _dac2Preamp.value,
        _dac2BufAnalogA.value,
        _dac2BufAnalogB.value,
        _dac2Hist.value,
        _dac2ThlFine.value,
        _dac2ThlCourse.value,
        _dac2Vcas.value,
        _dac2Fbk.value,
        _dac2Gnd.value,
        _dac2Ths.value,
        _dac2BiasLvds.value,
        _dac2RefLvds.value,
        _dac3Ikrum.value,
        _dac3Disc.value,
        _dac3Preamp.value,
        _dac3BufAnalogA.value,
        _dac3BufAnalogB.value,
        _dac3Hist.value,
        _dac3ThlFine.value,
        _dac3ThlCourse.value,
        _dac3Vcas.value,
        _dac3Fbk.value,
        _dac3Gnd.value,
        _dac3Ths.value,
        _dac3BiasLvds.value,
        _dac3RefLvds.value
      );

      return tc.size();
    }

    int dataSize() const {
      return sizeof(Pds::Timepix::ConfigV2);
    }

  public:
    Enumerated<Pds::Timepix::ConfigV2::ReadoutSpeed> _readoutSpeed;
    Enumerated<int32_t> _timepixSpeed;
    NumericInt<int32_t> _dac0ThlFine;
    NumericInt<int32_t> _dac1ThlFine;
    NumericInt<int32_t> _dac2ThlFine;
    NumericInt<int32_t> _dac3ThlFine;
    NumericInt<uint8_t> _timepixMode;
    NumericInt<int32_t> _dac0Ikrum;
    NumericInt<int32_t> _dac0Disc;
    NumericInt<int32_t> _dac0Preamp;
    NumericInt<int32_t> _dac0BufAnalogA;
    NumericInt<int32_t> _dac0BufAnalogB;
    NumericInt<int32_t> _dac0Hist;
    NumericInt<int32_t> _dac0ThlCourse;
    NumericInt<int32_t> _dac0Vcas;
    NumericInt<int32_t> _dac0Fbk;
    NumericInt<int32_t> _dac0Gnd;
    NumericInt<int32_t> _dac0Ths;
    NumericInt<int32_t> _dac0BiasLvds;
    NumericInt<int32_t> _dac0RefLvds;
    NumericInt<int32_t> _dac1Ikrum;
    NumericInt<int32_t> _dac1Disc;
    NumericInt<int32_t> _dac1Preamp;
    NumericInt<int32_t> _dac1BufAnalogA;
    NumericInt<int32_t> _dac1BufAnalogB;
    NumericInt<int32_t> _dac1Hist;
    NumericInt<int32_t> _dac1ThlCourse;
    NumericInt<int32_t> _dac1Vcas;
    NumericInt<int32_t> _dac1Fbk;
    NumericInt<int32_t> _dac1Gnd;
    NumericInt<int32_t> _dac1Ths;
    NumericInt<int32_t> _dac1BiasLvds;
    NumericInt<int32_t> _dac1RefLvds;
    NumericInt<int32_t> _dac2Ikrum;
    NumericInt<int32_t> _dac2Disc;
    NumericInt<int32_t> _dac2Preamp;
    NumericInt<int32_t> _dac2BufAnalogA;
    NumericInt<int32_t> _dac2BufAnalogB;
    NumericInt<int32_t> _dac2Hist;
    NumericInt<int32_t> _dac2ThlCourse;
    NumericInt<int32_t> _dac2Vcas;
    NumericInt<int32_t> _dac2Fbk;
    NumericInt<int32_t> _dac2Gnd;
    NumericInt<int32_t> _dac2Ths;
    NumericInt<int32_t> _dac2BiasLvds;
    NumericInt<int32_t> _dac2RefLvds;
    NumericInt<int32_t> _dac3Ikrum;
    NumericInt<int32_t> _dac3Disc;
    NumericInt<int32_t> _dac3Preamp;
    NumericInt<int32_t> _dac3BufAnalogA;
    NumericInt<int32_t> _dac3BufAnalogB;
    NumericInt<int32_t> _dac3Hist;
    NumericInt<int32_t> _dac3ThlCourse;
    NumericInt<int32_t> _dac3Vcas;
    NumericInt<int32_t> _dac3Fbk;
    NumericInt<int32_t> _dac3Gnd;
    NumericInt<int32_t> _dac3Ths;
    NumericInt<int32_t> _dac3BiasLvds;
    NumericInt<int32_t> _dac3RefLvds;
  };
};

using namespace Pds_ConfigDb;

// ---------- expert mode ------------
TimepixExpertConfig_V2::TimepixExpertConfig_V2() :
  Serializer("Timepix_Expert_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  TimepixExpertConfig_V2::readParameters (void* from) {
  return _private_data->pull(from);
}

int  TimepixExpertConfig_V2::writeParameters(void* to) {
  return _private_data->push(to);
}

int  TimepixExpertConfig_V2::dataSize() const {
  return _private_data->dataSize();
}

// ---------- user mode ------------
TimepixConfig_V2::TimepixConfig_V2() :
  Serializer("Timepix_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  TimepixConfig_V2::readParameters (void* from) {
  return _private_data->pull(from);
}

int  TimepixConfig_V2::writeParameters(void* to) {
  return _private_data->push(to);
}

int  TimepixConfig_V2::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

