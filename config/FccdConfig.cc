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
#define dacVoltage(n,v) _dacVoltage##n (FCCD_DAC[n-1].label, v, FCCD_DAC[n-1].v_start, FCCD_DAC[n-1].v_end)
      dacVoltage( 1,  6.0),
      dacVoltage( 2, -2.0),
      dacVoltage( 3,  6.0),
      dacVoltage( 4, -2.0),
      dacVoltage( 5,  8.0),
      dacVoltage( 6, -3.0),
      dacVoltage( 7,  5.0),
      dacVoltage( 8, -5.0),
      dacVoltage( 9,  0.0),
      dacVoltage(10, -6.0),
      dacVoltage(11, 50.0),
      dacVoltage(12,  3.0),
      dacVoltage(13,-14.8),
      dacVoltage(14,-24.0),
      dacVoltage(15,  0.0),
      dacVoltage(16,  0.0),
      dacVoltage(17,  0.0),
#undef dacVoltage
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
      FccdConfigType& tc = *reinterpret_cast<FccdConfigType*>(from);
      _outputMode.value = tc.outputMode();
      _ccdEnable.value = tc.ccdEnable() ? Enums::True : Enums::False;
      _focusMode.value = tc.focusMode() ? Enums::True : Enums::False;
      _exposureTime.value = tc.exposureTime();
#define dacVoltage(n) _dacVoltage##n .value = tc.dacVoltages()[n-1]
      dacVoltage(1);
      dacVoltage(2);
      dacVoltage(3);
      dacVoltage(4);
      dacVoltage(5);
      dacVoltage(6);
      dacVoltage(7);
      dacVoltage(8);
      dacVoltage(9);
      dacVoltage(10);
      dacVoltage(11);
      dacVoltage(12);
      dacVoltage(13);
      dacVoltage(14);
      dacVoltage(15);
      dacVoltage(16);
      dacVoltage(17);
#undef dacVoltage
#define waveform(n) _waveform##n .value = tc.waveforms()[n]
      waveform(0);
      waveform(1);
      waveform(2);
      waveform(3);
      waveform(4);
      waveform(5);
      waveform(6);
      waveform(7);
      waveform(8);
      waveform(9);
      waveform(10);
      waveform(11);
      waveform(12);
      waveform(13);
      waveform(14);
#undef waveform
      return tc._sizeof();
    }

    int push(void* to) {
      float dacVoltages[] = { 
        _dacVoltage1 .value, _dacVoltage2 .value, _dacVoltage3 .value, _dacVoltage4 .value,
        _dacVoltage5 .value, _dacVoltage6 .value, _dacVoltage7 .value, _dacVoltage8 .value,
        _dacVoltage9 .value, _dacVoltage10.value, _dacVoltage11.value, _dacVoltage12.value,
        _dacVoltage13.value, _dacVoltage14.value, _dacVoltage15.value, _dacVoltage16.value,
        _dacVoltage17.value };
      uint16_t waveforms[] = {
        _waveform0 .value, _waveform1 .value, _waveform2 .value, _waveform3 .value, 
        _waveform4 .value, _waveform5 .value, _waveform6 .value, _waveform7 .value, 
        _waveform8 .value, _waveform9 .value, _waveform10.value, _waveform11.value, 
        _waveform12.value, _waveform13.value, _waveform14.value };

      FccdConfigType& tc = *new(to) FccdConfigType(_outputMode.value,
                                                   _ccdEnable.value,
                                                   _focusMode.value,
                                                   _exposureTime.value,
                                                   dacVoltages,
                                                   waveforms);
      return tc._sizeof();
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

