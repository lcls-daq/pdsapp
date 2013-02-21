#include "pdsapp/config/FccdConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pds/config/FccdConfigType.hh"

#include <new>

namespace Pds_ConfigDb {

  class FccdConfig::Private_Data {
  public:
    Private_Data() :
      _outputMode  ("Output Mode (0=FIFO,1=DoNotReconfig)",  0,    0,    4),
      _ccdEnable ("CCD Enable", Enums::True, Enums::Bool_Names),
      // Focus Mode OFF for external triggering
      _focusMode ("Focus Mode (internal trigger)", Enums::False, Enums::Bool_Names),
      _exposureTime ("Internal Exposure Time (ms)", 1, 1, 0xffffff),
      _dacVoltage1  (FCCD_DAC1_LABEL, 6.0, FCCD_DAC1_V_START, FCCD_DAC1_V_END),
      _dacVoltage2  (FCCD_DAC2_LABEL, -2.0, FCCD_DAC2_V_END, FCCD_DAC2_V_START),
      _dacVoltage3  (FCCD_DAC3_LABEL, 6.0, FCCD_DAC3_V_START, FCCD_DAC3_V_END),
      _dacVoltage4  (FCCD_DAC4_LABEL, -2.0, FCCD_DAC4_V_END, FCCD_DAC4_V_START),
      _dacVoltage5  (FCCD_DAC5_LABEL, 8.0, FCCD_DAC5_V_START, FCCD_DAC5_V_END),
      _dacVoltage6  (FCCD_DAC6_LABEL, -3.0, FCCD_DAC6_V_END, FCCD_DAC6_V_START),
      _dacVoltage7  (FCCD_DAC7_LABEL, 5.0, FCCD_DAC7_V_START, FCCD_DAC7_V_END),
      _dacVoltage8  (FCCD_DAC8_LABEL, -5.0, FCCD_DAC8_V_END, FCCD_DAC8_V_START),
      _dacVoltage9  (FCCD_DAC9_LABEL, 0.0, FCCD_DAC9_V_START, FCCD_DAC9_V_END),
      _dacVoltage10 (FCCD_DAC10_LABEL, -6.0, FCCD_DAC10_V_END, FCCD_DAC10_V_START),
      _dacVoltage11 (FCCD_DAC11_LABEL, 50.0, FCCD_DAC11_V_START, FCCD_DAC11_V_END),
      _dacVoltage12 (FCCD_DAC12_LABEL, 3.0, FCCD_DAC12_V_START, FCCD_DAC12_V_END),
      _dacVoltage13 (FCCD_DAC13_LABEL, -14.8, FCCD_DAC13_V_END, FCCD_DAC13_V_START),
      _dacVoltage14 (FCCD_DAC14_LABEL, -24.0, FCCD_DAC14_V_END, FCCD_DAC14_V_START),
      _dacVoltage15 (FCCD_DAC15_LABEL, 0.0, FCCD_DAC15_V_START, FCCD_DAC15_V_END),
      _dacVoltage16 (FCCD_DAC16_LABEL, 0.0, FCCD_DAC16_V_START, FCCD_DAC16_V_END),
      _dacVoltage17 (FCCD_DAC17_LABEL, 0.0, FCCD_DAC17_V_START, FCCD_DAC17_V_END),
      _waveform0 ("Hclk1-idle (hex)",  0xfe3f,    0x0, 0xffff, Hex),
      _waveform1 ("Hclk2-idle (hex)",  0x0380,    0x0, 0xffff, Hex),
      _waveform2 ("Hclk3-idle (hex)",  0xf8ff,    0x0, 0xffff, Hex),
      _waveform3 ("Sw-idle (hex)",     0x0004,    0x0, 0xffff, Hex),
      _waveform4 ("Rg-idle (hex)",     0xf83f,    0x0, 0xffff, Hex),
      _waveform5 ("Vclk1-idleRd (hex)",0xf1ff,    0x0, 0xffff, Hex),
      _waveform6 ("Vclk2-idleRd (hex)",0x1c00,    0x0, 0xffff, Hex),
      _waveform7 ("Vclk3-idleRd (hex)",0xc7ff,    0x0, 0xffff, Hex),
      _waveform8 ("Tg-idleRd (hex)",   0x83ff,    0x0, 0xffff, Hex),
      _waveform9 ("Hclk1-Rd (hex)",    0xfe3f,    0x0, 0xffff, Hex),
      _waveform10 ("Hclk2-Rd (hex)",   0x0380,    0x0, 0xffff, Hex),
      _waveform11 ("Hclk3-Rd (hex)",   0xf8ff,    0x0, 0xffff, Hex),
      _waveform12 ("Sw-Rd (hex)",      0x0001,    0x0, 0xffff, Hex),
      _waveform13 ("Rg-Rd (hex)",      0xf83f,    0x0, 0xffff, Hex),
      _waveform14 ("Convt-Rd (hex)",   0x0020,    0x0, 0xffff, Hex)
    {}

    void insert(Pds::LinkedList<Parameter>& pList) {
      pList.insert(&_outputMode);
      pList.insert(&_ccdEnable);
      pList.insert(&_focusMode);
      pList.insert(&_exposureTime);
      pList.insert(&_dacVoltage1);
      pList.insert(&_dacVoltage2);
      pList.insert(&_dacVoltage3);
      pList.insert(&_dacVoltage4);
      pList.insert(&_dacVoltage5);
      pList.insert(&_dacVoltage6);
      pList.insert(&_dacVoltage7);
      pList.insert(&_dacVoltage8);
      pList.insert(&_dacVoltage9);
      pList.insert(&_dacVoltage10);
      pList.insert(&_dacVoltage11);
      pList.insert(&_dacVoltage12);
      pList.insert(&_dacVoltage13);
      pList.insert(&_dacVoltage14);
      pList.insert(&_dacVoltage15);
      pList.insert(&_dacVoltage16);
      pList.insert(&_dacVoltage17);
      pList.insert(&_waveform0);
      pList.insert(&_waveform1);
      pList.insert(&_waveform2);
      pList.insert(&_waveform3);
      pList.insert(&_waveform4);
      pList.insert(&_waveform5);
      pList.insert(&_waveform6);
      pList.insert(&_waveform7);
      pList.insert(&_waveform8);
      pList.insert(&_waveform9);
      pList.insert(&_waveform10);
      pList.insert(&_waveform11);
      pList.insert(&_waveform12);
      pList.insert(&_waveform13);
      pList.insert(&_waveform14);
    }

    int pull(void* from) {
      FccdConfigType& tc = *new(from) FccdConfigType;
      _outputMode.value = tc.outputMode();
      _ccdEnable.value = tc.ccdEnable() ? Enums::True : Enums::False;
      _focusMode.value = tc.focusMode() ? Enums::True : Enums::False;
      _exposureTime.value = tc.exposureTime();
      _dacVoltage1.value = tc.dacVoltage1();
      _dacVoltage2.value = tc.dacVoltage2();
      _dacVoltage3.value = tc.dacVoltage3();
      _dacVoltage4.value = tc.dacVoltage4();
      _dacVoltage5.value = tc.dacVoltage5();
      _dacVoltage6.value = tc.dacVoltage6();
      _dacVoltage7.value = tc.dacVoltage7();
      _dacVoltage8.value = tc.dacVoltage8();
      _dacVoltage9.value = tc.dacVoltage9();
      _dacVoltage10.value = tc.dacVoltage10();
      _dacVoltage11.value = tc.dacVoltage11();
      _dacVoltage12.value = tc.dacVoltage12();
      _dacVoltage13.value = tc.dacVoltage13();
      _dacVoltage14.value = tc.dacVoltage14();
      _dacVoltage15.value = tc.dacVoltage15();
      _dacVoltage16.value = tc.dacVoltage16();
      _dacVoltage17.value = tc.dacVoltage17();
      _waveform0.value = tc.waveform0();
      _waveform1.value = tc.waveform1();
      _waveform2.value = tc.waveform2();
      _waveform3.value = tc.waveform3();
      _waveform4.value = tc.waveform4();
      _waveform5.value = tc.waveform5();
      _waveform6.value = tc.waveform6();
      _waveform7.value = tc.waveform7();
      _waveform8.value = tc.waveform8();
      _waveform9.value = tc.waveform9();
      _waveform10.value = tc.waveform10();
      _waveform11.value = tc.waveform11();
      _waveform12.value = tc.waveform12();
      _waveform13.value = tc.waveform13();
      _waveform14.value = tc.waveform14();
      return tc.size();
    }

    int push(void* to) {
      FccdConfigType& tc = *new(to) FccdConfigType(
        _outputMode.value,
        _ccdEnable.value,
        _focusMode.value,
        _exposureTime.value,
        _dacVoltage1.value,
        _dacVoltage2.value,
        _dacVoltage3.value,
        _dacVoltage4.value,
        _dacVoltage5.value,
        _dacVoltage6.value,
        _dacVoltage7.value,
        _dacVoltage8.value,
        _dacVoltage9.value,
        _dacVoltage10.value,
        _dacVoltage11.value,
        _dacVoltage12.value,
        _dacVoltage13.value,
        _dacVoltage14.value,
        _dacVoltage15.value,
        _dacVoltage16.value,
        _dacVoltage17.value,
        _waveform0.value,
        _waveform1.value,
        _waveform2.value,
        _waveform3.value,
        _waveform4.value,
        _waveform5.value,
        _waveform6.value,
        _waveform7.value,
        _waveform8.value,
        _waveform9.value,
        _waveform10.value,
        _waveform11.value,
        _waveform12.value,
        _waveform13.value,
        _waveform14.value
      );
      return tc.size();
    }

    int dataSize() const {
      return sizeof(FccdConfigType);
    }

  public:
    NumericInt<uint16_t>    _outputMode;
    Enumerated<Enums::Bool> _ccdEnable;
    Enumerated<Enums::Bool> _focusMode;
    NumericInt<uint32_t>    _exposureTime;
    NumericFloat<float>     _dacVoltage1;
    NumericFloat<float>     _dacVoltage2;
    NumericFloat<float>     _dacVoltage3;
    NumericFloat<float>     _dacVoltage4;
    NumericFloat<float>     _dacVoltage5;
    NumericFloat<float>     _dacVoltage6;
    NumericFloat<float>     _dacVoltage7;
    NumericFloat<float>     _dacVoltage8;
    NumericFloat<float>     _dacVoltage9;
    NumericFloat<float>     _dacVoltage10;
    NumericFloat<float>     _dacVoltage11;
    NumericFloat<float>     _dacVoltage12;
    NumericFloat<float>     _dacVoltage13;
    NumericFloat<float>     _dacVoltage14;
    NumericFloat<float>     _dacVoltage15;
    NumericFloat<float>     _dacVoltage16;
    NumericFloat<float>     _dacVoltage17;
    NumericInt<uint16_t>    _waveform0;
    NumericInt<uint16_t>    _waveform1;
    NumericInt<uint16_t>    _waveform2;
    NumericInt<uint16_t>    _waveform3;
    NumericInt<uint16_t>    _waveform4;
    NumericInt<uint16_t>    _waveform5;
    NumericInt<uint16_t>    _waveform6;
    NumericInt<uint16_t>    _waveform7;
    NumericInt<uint16_t>    _waveform8;
    NumericInt<uint16_t>    _waveform9;
    NumericInt<uint16_t>    _waveform10;
    NumericInt<uint16_t>    _waveform11;
    NumericInt<uint16_t>    _waveform12;
    NumericInt<uint16_t>    _waveform13;
    NumericInt<uint16_t>    _waveform14;
  };
};

using namespace Pds_ConfigDb;

FccdConfig::FccdConfig() : 
  Serializer("fccd_Config"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int  FccdConfig::readParameters (void* from) {
  return _private_data->pull(from);
}

int  FccdConfig::writeParameters(void* to) {
  return _private_data->push(to);
}

int  FccdConfig::dataSize() const {
  return _private_data->dataSize();
}

#include "Parameters.icc"

