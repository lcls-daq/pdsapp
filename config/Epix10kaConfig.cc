#include "pdsapp/config/Epix10kaConfig.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pdsapp/config/Epix10kaCopyAsicDialog.hh"
#include "pdsapp/config/Epix10kaASICdata.hh"
#include "pdsapp/config/Epix10kaPixelMap.hh"
#include "pdsapp/config/Epix10kaCalibMap.hh"
#include "ndarray/ndarray.h"

#include "pds/config/EpixConfigType.hh"
#include "pds/config/Epix10kaASICConfigV1.hh"
#include "pds/config/Epix10kaConfigV1.hh"

#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtGui/QDialog>
#include <QtCore/QObject>

#include <stdio.h>
#include <new>
#include <stdio.h>

namespace Pds_ConfigDb {

  class Epix10kaSimpleCount : public ParameterCount {
    public:
    Epix10kaSimpleCount(unsigned c) : mycount(c) {};
    ~Epix10kaSimpleCount() {};
    bool connect(ParameterSet&) { return false; }
    unsigned count() { return mycount; }
    unsigned mycount;
  };

  static std::vector<Epix10kaCopyTarget*> targets(Epix10kaASICdata* a)
  {
    std::vector<Epix10kaCopyTarget*> v(Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerRow) *
        Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerColumn));
    for(unsigned i=0; i<Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerRow) *
    Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerColumn); i++)
      v[i] = &a[i];
    return v;
  }

  class Epix10kaConfig::PrivateData : public Parameter {
  public:
    PrivateData(bool expert) :
      _expert(expert),
      _count(Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerRow) *
          Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerColumn)),
      _asicSet("ASICs", _asicArgs, _count, targets(_asic)),
      _pixelArray(make_ndarray<uint16_t>(
          Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfRowsPerAsic) *
          Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerColumn),
          Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfPixelsPerAsicRow) *
          Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerRow))),
      _calibArray(make_ndarray<uint8_t>(
          Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::CalibrationRowCountPerASIC)  ,
          // one calibration row per two actuals, assumes two ASICs per column AND two calibration rows per ASIC !!!!
          Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfPixelsPerAsicRow) *
          Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerRow))),
      _dialog(_pixelArray), _calibdialog(_calibArray) {
      Epix10kaConfigShadow::initNames();
      for (unsigned i=0; i<Epix10kaConfigShadow::NumberOfRegisters; i++) {
    	  Epix10kaConfigShadow::Registers r = (Epix10kaConfigShadow::Registers) i;
        _reg[i] = new NumericInt<uint32_t>(
                                           Epix10kaConfigShadow::name(r),
                                           Epix10kaConfigShadow::defaultValue(r),
                                           Epix10kaConfigShadow::rangeLow(r),
                                           Epix10kaConfigShadow::rangeHigh(r),
                                           Epix10kaConfigShadow::type(r)  ==  Epix10kaConfigShadow::hex ? Hex : Decimal
                                           );
      }
      for (uint32_t i=0; i<Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerRow) *
      Epix10kaConfigShadow::defaultValue(Epix10kaConfigShadow::NumberOfAsicsPerColumn); i++) {
        _asicArgs[i].insert(&_asic[i]);
      }
      if (_expert) _asicSet.name("ASIC:");
      uint16_t* b = (uint16_t*)_pixelArray.begin();
      unsigned count = 0;
      while (b!=_pixelArray.end()) {
        *b++ = ((count++)>>2)&3;
      }
      _mapButton   = new QPushButton(QString("Pixel &Map"));
      _calibButton = new QPushButton(QString("&Calib Map"));
      QObject::connect(_mapButton,   SIGNAL(clicked()), &_dialog,      SLOT(exec()));
      QObject::connect(_calibButton, SIGNAL(clicked()), &_calibdialog, SLOT(exec()));
    }
    ~PrivateData() {}
  public:
    QLayout* initialize(QWidget* p) {
      QHBoxLayout* layout = new QHBoxLayout;
      if (_expert) {
        { QVBoxLayout* l = new QVBoxLayout;
        for (unsigned i=Epix10kaConfigShadow::Version; i<Epix10kaConfigShadow::NumberOfRegisters; i++) {
          if (Epix10kaConfigShadow::readOnly((Epix10kaConfigShadow::Registers)i) == Epix10kaConfigShadow::ReadOnly) {
            l->addLayout(_reg[i]->initialize(0));
          }
        }
        l->addStretch();
        for (unsigned i=Epix10kaConfigShadow::Version; i<Epix10kaConfigShadow::NumberOfRegisters; i++) {
          if (Epix10kaConfigShadow::readOnly((Epix10kaConfigShadow::Registers)i) == Epix10kaConfigShadow::UseOnly) {
            l->addLayout(_reg[i]->initialize(0));
          }
        }
        l->addStretch();
//        layout->addStretch();
        l->addLayout(_asicSet.initialize(0));
        l->addWidget(_mapButton);
        l->addWidget(_calibButton);
        l->addStretch();
        layout->addLayout(l);
        }
        layout->addStretch();
        { QVBoxLayout* l = new QVBoxLayout;
        for (unsigned i=Epix10kaConfigShadow::Version; i<Epix10kaConfigShadow::prepulseR0En; i++) {
          if (Epix10kaConfigShadow::readOnly((Epix10kaConfigShadow::Registers)i) == Epix10kaConfigShadow::ReadWrite) {
            l->addLayout(_reg[i]->initialize(0));
          }
        }
        layout->addLayout(l);
        }
        { QVBoxLayout* l = new QVBoxLayout;
        for (unsigned i=Epix10kaConfigShadow::prepulseR0En; i<Epix10kaConfigShadow::SyncWidth; i++) {
          if (Epix10kaConfigShadow::readOnly((Epix10kaConfigShadow::Registers)i) == Epix10kaConfigShadow::ReadWrite) {
            l->addLayout(_reg[i]->initialize(0));
          }
        }
        layout->addLayout(l);
        }
        layout->addStretch();
        { QVBoxLayout* l = new QVBoxLayout;
        for (unsigned i=Epix10kaConfigShadow::SyncWidth; i<Epix10kaConfigShadow::NumberOfRegisters; i++) {
          if (Epix10kaConfigShadow::readOnly((Epix10kaConfigShadow::Registers)i) == Epix10kaConfigShadow::ReadWrite) {
            l->addLayout(_reg[i]->initialize(0));
          }
        }
        layout->addLayout(l); }
      } else {
        // On the non-expert config screen only show the trigger related registers of the EPIX
        { QVBoxLayout* l = new QVBoxLayout;
          for (unsigned i=Epix10kaConfigShadow::Version; i<Epix10kaConfigShadow::EvrRunCode; i++) {
            _reg[i]->initialize(0); // initialize but don't show
          }
          for (unsigned i=Epix10kaConfigShadow::EvrRunCode; i<Epix10kaConfigShadow::DacSetting; i++) {
            if (i==Epix10kaConfigShadow::EvrRunCode)
              _reg[i]->initialize(0); // initialize but don't show daq code
            else
              l->addLayout(_reg[i]->initialize(0));
          }
          for (unsigned i=Epix10kaConfigShadow::DacSetting; i<Epix10kaConfigShadow::NumberOfRegisters; i++) {
            _reg[i]->initialize(0); // initialize but don't show
          }
          layout->addLayout(l);
        }
        _asicSet.initialize(0);
      }

      printf("PrivateData::initialize: NumberOfRegisters %u\n", Epix10kaConfigShadow::NumberOfRegisters);

      for(unsigned i=0; i<Epix10kaConfigShadow::NumberOfRegisters; i++)
        if (Epix10kaConfigShadow::readOnly(Epix10kaConfigShadow::Registers(i)) != Epix10kaConfigShadow::DoNotUse) {
          _reg[i]->enable(!(Epix10kaConfigShadow::readOnly(Epix10kaConfigShadow::Registers(i)) == Epix10kaConfigShadow::ReadOnly));
        }

      return layout;
    }
    void update() {
      for(unsigned i=0; i<Epix10kaConfigShadow::NumberOfRegisters; i++)
        if (Epix10kaConfigShadow::readOnly(Epix10kaConfigShadow::Registers(i)) != Epix10kaConfigShadow::DoNotUse) {
          _reg[i]->update();
        }
      _asicSet.update();
    }
    void flush () {
      for(unsigned i=0; i<Epix10kaConfigShadow::NumberOfRegisters; i++)
        if (Epix10kaConfigShadow::readOnly(Epix10kaConfigShadow::Registers(i)) != Epix10kaConfigShadow::DoNotUse) {
          _reg[i]->flush();
        }
      _asicSet.flush();
    }
    void enable(bool v) {
      for(unsigned i=0; i<Epix10kaConfigShadow::NumberOfRegisters; i++)
        if (Epix10kaConfigShadow::readOnly(Epix10kaConfigShadow::Registers(i)) != Epix10kaConfigShadow::DoNotUse) {
          _reg[i]->enable(v && (Epix10kaConfigShadow::readOnly(Epix10kaConfigShadow::Registers(i)) == Epix10kaConfigShadow::ReadWrite));
        }
      _asicSet.enable(v);
    }
  public:
    int pull(void* from) {
      Epix10kaConfigType& epixConf = *new(from) Epix10kaConfigType;
      Epix10kaConfigShadow& epixConfShadow = *new(from) Epix10kaConfigShadow;
      //    printf("Epix10kaConfig::readParameters:");
      for (uint32_t i=0; i<Epix10kaConfigShadow::NumberOfRegisters; i++) {
        _reg[i]->value = epixConfShadow.get((Epix10kaConfigShadow::Registers) i);
        //      printf("%s0x%x",  i%16==0 ? "\n\t" : " ", epixConfShadow.get((Epix10kaConfigShadow::Registers) i));
      }
      for (uint32_t i=0; i<epixConf.numberOfAsics(); i++) {
        _asic[i].pull(reinterpret_cast<const Epix10kaASIC_ConfigShadow&>(epixConf.asics(i)));
      }
      printf("pull() pixel array [%u][%u]\n",
          epixConf.numberOfRows(),
          epixConf.numberOfColumns());
      ndarray<const uint16_t,2> array = epixConf.asicPixelConfigArray();
      uint16_t* a = const_cast<uint16_t*>(array.begin());
      uint16_t* b = (uint16_t*)_pixelArray.begin();
      while (a!=array.end()) {
        *b++ = *a++;
      }
      ndarray<const uint8_t,2> carray = epixConf.calibPixelConfigArray();
      uint8_t* c = const_cast<uint8_t*>(carray.begin());
      uint8_t* d = (uint8_t*)_calibArray.begin();
      while (c!=carray.end()) {
        *d++ = *c++;
      }
      _dialog.show_map();
      _calibdialog.show_map();
      return epixConf._sizeof();
    }
    int push(void* to) {
      Epix10kaConfigType& epixConf = *new(to) Epix10kaConfigType;
      Epix10kaConfigShadow& epixConfShadow = *new(to) Epix10kaConfigShadow(true);
      for (uint32_t i=0; i<Epix10kaConfigShadow::NumberOfRegisters; i++) {
        epixConfShadow.set((Epix10kaConfigShadow::Registers) i, _reg[i]->value);
      }
      for (uint32_t i=0; i<epixConf.numberOfAsics(); i++) {
        _asic[i].push((void*)(&(epixConf.asics(i))));
      }
      printf("push() pixel array [%u][%u]\n",
          epixConf.numberOfRows(),
          epixConf.numberOfColumns());
      ndarray<const uint16_t,2> array = epixConf.asicPixelConfigArray();
      uint16_t* a = const_cast<uint16_t*>(array.begin());
      uint16_t* b = (uint16_t*)_pixelArray.begin();
      while (a!=array.end()) {
        *a++ = *b++;
      }
      ndarray<const uint8_t,2> carray = epixConf.calibPixelConfigArray();
      uint8_t* c = const_cast<uint8_t*>(carray.begin());
      uint8_t* d = (uint8_t*)_calibArray.begin();
      while (c!=(uint8_t*)carray.end()) {
        *c++ = *d++;
      }
      return epixConf._sizeof();
    }
    int dataSize() const {
      Epix10kaConfigType* foo = new Epix10kaConfigType(
                                             _reg[Epix10kaConfigShadow::NumberOfAsicsPerRow]->value,
                                             _reg[Epix10kaConfigShadow::NumberOfAsicsPerColumn]->value,
                                             _reg[Epix10kaConfigShadow::NumberOfRowsPerAsic]->value,
                                             _reg[Epix10kaConfigShadow::NumberOfPixelsPerAsicRow]->value,
                                             _reg[Epix10kaConfigShadow::CalibrationRowCountPerASIC]->value);
      int size = (int) foo->_sizeof();
          printf("Epix10kaConfig::dataSize apr(%u) apc(%u) rpa(%u) ppar(%u) crcpa(%u) size(%u)\n",
              _reg[Epix10kaConfigShadow::NumberOfAsicsPerRow]->value,
              _reg[Epix10kaConfigShadow::NumberOfAsicsPerColumn]->value,
              _reg[Epix10kaConfigShadow::NumberOfRowsPerAsic]->value,
              _reg[Epix10kaConfigShadow::NumberOfPixelsPerAsicRow]->value,
              _reg[Epix10kaConfigShadow::CalibrationRowCountPerASIC]->value,
              size);
      delete foo;
      return size;
    }
  private:
    bool                        _expert;
    NumericInt<uint32_t>*       _reg[Epix10kaConfigShadow::NumberOfRegisters];
    Epix10kaSimpleCount         _count;
    Epix10kaASICdata            _asic    [Epix10kaConfigShadow::NumberOfAsics];
    Pds::LinkedList<Parameter>  _asicArgs[Epix10kaConfigShadow::NumberOfAsics];
    Epix10kaAsicSet             _asicSet;
    Pds::LinkedList<Parameter>  pList;
    QPushButton*                _mapButton;
    QPushButton*                _calibButton;
    ndarray<uint16_t, 2>        _pixelArray;
    ndarray<uint8_t, 2>         _calibArray;
    Epix10kaPixelMapDialog      _dialog;
    Epix10kaCalibMapDialog      _calibdialog;
  };
};

using namespace Pds_ConfigDb;

Epix10kaConfig::Epix10kaConfig(bool expert) :
  Serializer("Epix10ka_Config"),
  _private(new PrivateData(expert))
{
  name("EPIX10ka Configuration");
  pList.insert(_private);
}

Epix10kaConfig::~Epix10kaConfig()
{
  delete _private;
}

int Epix10kaConfig::readParameters(void* from) { // pull "from xtc"
  return _private->pull(from);
}

int Epix10kaConfig::writeParameters(void* to) {
  return _private->push(to);
}

int Epix10kaConfig::dataSize() const {
  return _private->dataSize();
}

