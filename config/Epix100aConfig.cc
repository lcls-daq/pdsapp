#include "pdsapp/config/Epix100aConfig.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pdsapp/config/Epix100aCopyAsicDialog.hh"
#include "pdsapp/config/Epix100aPixelMap.hh"
#include "pdsapp/config/Epix100aCalibMap.hh"
#include "ndarray/ndarray.h"

#include "pds/config/EpixConfigType.hh"
#include "pds/config/Epix100aASICConfigV1.hh"
#include "pds/config/Epix100aConfigV1.hh"

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

  class Epix100aSimpleCount : public ParameterCount {
    public:
    Epix100aSimpleCount(unsigned c) : mycount(c) {};
    ~Epix100aSimpleCount() {};
    bool connect(ParameterSet&) { return false; }
    unsigned count() { return mycount; }
    unsigned mycount;
  };

  class Epix100aASICdata : public Epix100aCopyTarget,
                       public Parameter {
  public:
    Epix100aASICdata();
    ~Epix100aASICdata() {};
  public:
    void copy(const Epix100aCopyTarget& s) {
      const Epix100aASICdata& src = static_cast<const Epix100aASICdata&>(s);
      for(unsigned i=0; i<Epix100aASIC_ConfigShadow::NumberOfRegisters; i++) {
        if (Epix100aASIC_ConfigShadow::doNotCopy(Epix100aASIC_ConfigShadow::Registers(i)) == Epix100aASIC_ConfigShadow::DoCopy) {
          _reg[i]->value = src._reg[i]->value;
        }
        else {
        	printf("Epix100aASICdata::copy did not copy %s\n", Epix100aASIC_ConfigShadow::name(Epix100aASIC_ConfigShadow::Registers(i)));
        }
      }
    }
  public:
    QLayout* initialize(QWidget*) {
      QVBoxLayout* l = new QVBoxLayout;
      { QGridLayout* layout = new QGridLayout;
        for(unsigned i=0; i<Epix100aASIC_ConfigShadow::NumberOfRegisters; i++)
          layout->addLayout(_reg[i]->initialize(0), i%16, i/16);
        l->addLayout(layout); }

      for(unsigned i=0; i<Epix100aASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->enable((!Epix100aASIC_ConfigShadow::readOnly(Epix100aASIC_ConfigShadow::Registers(i))) ||
            (Epix100aASIC_ConfigShadow::readOnly(Epix100aASIC_ConfigShadow::Registers(i))==Epix100aASIC_ConfigShadow::WriteOnly));
      return l;
    }
    void update() {
      for(unsigned i=0; i<Epix100aASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->update();
    }
    void flush () {
      for(unsigned i=0; i<Epix100aASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->flush();
    }
    void enable(bool v) {
      for(unsigned i=0; i<Epix100aASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->enable(v && ((!Epix100aASIC_ConfigShadow::readOnly(Epix100aASIC_ConfigShadow::Registers(i))) ||
            (Epix100aASIC_ConfigShadow::readOnly(Epix100aASIC_ConfigShadow::Registers(i))==Epix100aASIC_ConfigShadow::WriteOnly)));
    }
  public:
    int pull(const Epix100aASIC_ConfigShadow&);
    int push(void*);
  public:
    NumericInt<uint32_t>*       _reg[Epix100aASIC_ConfigShadow::NumberOfRegisters];
  };

  static std::vector<Epix100aCopyTarget*> targets(Epix100aASICdata* a)
  {
    std::vector<Epix100aCopyTarget*> v(Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerRow) *
        Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerColumn));
    for(unsigned i=0; i<Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerRow) *
    Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerColumn); i++)
      v[i] = &a[i];
    return v;
  }

  class Epix100aConfig::PrivateData : public Parameter {
  public:
    PrivateData(bool expert) :
      _expert(expert),
      _count(Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerRow) *
          Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerColumn)),
      _asicSet("ASICs", _asicArgs, _count, targets(_asic)),
      _pixelArray(make_ndarray<uint16_t>(
          Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfRowsPerAsic) *
          Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerColumn),
          Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfPixelsPerAsicRow) *
          Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerRow))),
      _calibArray(make_ndarray<uint8_t>(
          Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::CalibrationRowCountPerASIC)  ,
          // one calibration row per two actuals, assumes two ASICs per column AND two calibration rows per ASIC !!!!
          Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfPixelsPerAsicRow) *
          Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerRow))),
      _dialog(_pixelArray), _calibdialog(_calibArray) {
      Epix100aConfigShadow::initNames();
      for (unsigned i=0; i<Epix100aConfigShadow::NumberOfRegisters; i++) {
    	  Epix100aConfigShadow::Registers r = (Epix100aConfigShadow::Registers) i;
        _reg[i] = new NumericInt<uint32_t>(
                                           Epix100aConfigShadow::name(r),
                                           Epix100aConfigShadow::defaultValue(r),
                                           Epix100aConfigShadow::rangeLow(r),
                                           Epix100aConfigShadow::rangeHigh(r),
                                           Epix100aConfigShadow::type(r)  ==  Epix100aConfigShadow::hex ? Hex : Decimal
                                           );
      }
      for (uint32_t i=0; i<Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerRow) *
      Epix100aConfigShadow::defaultValue(Epix100aConfigShadow::NumberOfAsicsPerColumn); i++) {
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
        for (unsigned i=Epix100aConfigShadow::Version; i<Epix100aConfigShadow::NumberOfRegisters; i++) {
          if (Epix100aConfigShadow::readOnly((Epix100aConfigShadow::Registers)i) == Epix100aConfigShadow::ReadOnly) {
            l->addLayout(_reg[i]->initialize(0));
          }
        }
        l->addStretch();
        for (unsigned i=Epix100aConfigShadow::Version; i<Epix100aConfigShadow::NumberOfRegisters; i++) {
          if (Epix100aConfigShadow::readOnly((Epix100aConfigShadow::Registers)i) == Epix100aConfigShadow::UseOnly) {
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
        for (unsigned i=Epix100aConfigShadow::Version; i<Epix100aConfigShadow::prepulseR0En; i++) {
          if (Epix100aConfigShadow::readOnly((Epix100aConfigShadow::Registers)i) == Epix100aConfigShadow::ReadWrite) {
            l->addLayout(_reg[i]->initialize(0));
          }
        }
        layout->addLayout(l);
        }
        { QVBoxLayout* l = new QVBoxLayout;
        for (unsigned i=Epix100aConfigShadow::prepulseR0En; i<Epix100aConfigShadow::SyncWidth; i++) {
          if (Epix100aConfigShadow::readOnly((Epix100aConfigShadow::Registers)i) == Epix100aConfigShadow::ReadWrite) {
            l->addLayout(_reg[i]->initialize(0));
          }
        }
        layout->addLayout(l);
        }
        layout->addStretch();
        { QVBoxLayout* l = new QVBoxLayout;
        for (unsigned i=Epix100aConfigShadow::SyncWidth; i<Epix100aConfigShadow::NumberOfRegisters; i++) {
          if (Epix100aConfigShadow::readOnly((Epix100aConfigShadow::Registers)i) == Epix100aConfigShadow::ReadWrite) {
            l->addLayout(_reg[i]->initialize(0));
          }
        }
        layout->addLayout(l); }
      } else {
        // On the non-expert config screen only show the trigger related registers of the EPIX
        { QVBoxLayout* l = new QVBoxLayout;
          for (unsigned i=Epix100aConfigShadow::Version; i<Epix100aConfigShadow::EvrRunCode; i++) {
            _reg[i]->initialize(0); // initialize but don't show
          }
          for (unsigned i=Epix100aConfigShadow::EvrRunCode; i<Epix100aConfigShadow::DacSetting; i++) {
            if (i==Epix100aConfigShadow::EvrDaqCode)
              _reg[i]->initialize(0); // initialize but don't show daq code
            else
              l->addLayout(_reg[i]->initialize(0));
          }
          for (unsigned i=Epix100aConfigShadow::DacSetting; i<Epix100aConfigShadow::NumberOfRegisters; i++) {
            _reg[i]->initialize(0); // initialize but don't show
          }
          layout->addLayout(l);
        }
        _asicSet.initialize(0);
      }

      printf("PrivateData::initialize: NumberOfRegisters %u\n", Epix100aConfigShadow::NumberOfRegisters);

      for(unsigned i=0; i<Epix100aConfigShadow::NumberOfRegisters; i++)
        if (Epix100aConfigShadow::readOnly(Epix100aConfigShadow::Registers(i)) != Epix100aConfigShadow::DoNotUse) {
          _reg[i]->enable(!(Epix100aConfigShadow::readOnly(Epix100aConfigShadow::Registers(i)) == Epix100aConfigShadow::ReadOnly));
        }

      return layout;
    }
    void update() {
      for(unsigned i=0; i<Epix100aConfigShadow::NumberOfRegisters; i++)
        if (Epix100aConfigShadow::readOnly(Epix100aConfigShadow::Registers(i)) != Epix100aConfigShadow::DoNotUse) {
          _reg[i]->update();
        }
      _asicSet.update();
    }
    void flush () {
      for(unsigned i=0; i<Epix100aConfigShadow::NumberOfRegisters; i++)
        if (Epix100aConfigShadow::readOnly(Epix100aConfigShadow::Registers(i)) != Epix100aConfigShadow::DoNotUse) {
          _reg[i]->flush();
        }
      _asicSet.flush();
    }
    void enable(bool v) {
      for(unsigned i=0; i<Epix100aConfigShadow::NumberOfRegisters; i++)
        if (Epix100aConfigShadow::readOnly(Epix100aConfigShadow::Registers(i)) != Epix100aConfigShadow::DoNotUse) {
          _reg[i]->enable(v && (Epix100aConfigShadow::readOnly(Epix100aConfigShadow::Registers(i)) == Epix100aConfigShadow::ReadWrite));
        }
      _asicSet.enable(v);
    }
  public:
    int pull(void* from) {
      Epix100aConfigType& epixConf = *new(from) Epix100aConfigType;
      Epix100aConfigShadow& epixConfShadow = *new(from) Epix100aConfigShadow;
      //    printf("Epix100aConfig::readParameters:");
      for (uint32_t i=0; i<Epix100aConfigShadow::NumberOfRegisters; i++) {
        _reg[i]->value = epixConfShadow.get((Epix100aConfigShadow::Registers) i);
        //      printf("%s0x%x",  i%16==0 ? "\n\t" : " ", epixConfShadow.get((Epix100aConfigShadow::Registers) i));
      }
      for (uint32_t i=0; i<epixConf.numberOfAsics(); i++) {
        _asic[i].pull(reinterpret_cast<const Epix100aASIC_ConfigShadow&>(epixConf.asics(i)));
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
      Epix100aConfigType& epixConf = *new(to) Epix100aConfigType;
      Epix100aConfigShadow& epixConfShadow = *new(to) Epix100aConfigShadow(true);
      for (uint32_t i=0; i<Epix100aConfigShadow::NumberOfRegisters; i++) {
        epixConfShadow.set((Epix100aConfigShadow::Registers) i, _reg[i]->value);
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
      Epix100aConfigType* foo = new Epix100aConfigType(
                                             _reg[Epix100aConfigShadow::NumberOfAsicsPerRow]->value,
                                             _reg[Epix100aConfigShadow::NumberOfAsicsPerColumn]->value,
                                             _reg[Epix100aConfigShadow::NumberOfRowsPerAsic]->value,
                                             _reg[Epix100aConfigShadow::NumberOfPixelsPerAsicRow]->value,
                                             _reg[Epix100aConfigShadow::CalibrationRowCountPerASIC]->value);
      int size = (int) foo->_sizeof();
          printf("Epix100aConfig::dataSize apr(%u) apc(%u) rpa(%u) ppar(%u) crcpa(%u) size(%u)\n",
              _reg[Epix100aConfigShadow::NumberOfAsicsPerRow]->value,
              _reg[Epix100aConfigShadow::NumberOfAsicsPerColumn]->value,
              _reg[Epix100aConfigShadow::NumberOfRowsPerAsic]->value,
              _reg[Epix100aConfigShadow::NumberOfPixelsPerAsicRow]->value,
              _reg[Epix100aConfigShadow::CalibrationRowCountPerASIC]->value,
              size);
      delete foo;
      return size;
    }
  private:
    bool                        _expert;
    NumericInt<uint32_t>*       _reg[Epix100aConfigShadow::NumberOfRegisters];
    Epix100aSimpleCount         _count;
    Epix100aASICdata            _asic    [Epix100aConfigShadow::NumberOfAsics];
    Pds::LinkedList<Parameter>  _asicArgs[Epix100aConfigShadow::NumberOfAsics];
    Epix100aAsicSet             _asicSet;
    Pds::LinkedList<Parameter>  pList;
    QPushButton*                _mapButton;
    QPushButton*                _calibButton;
    ndarray<uint16_t, 2>        _pixelArray;
    ndarray<uint8_t, 2>         _calibArray;
    Epix100aPixelMapDialog      _dialog;
    Epix100aCalibMapDialog      _calibdialog;
  };
};

using namespace Pds_ConfigDb;

Epix100aASICdata::Epix100aASICdata() {
  for (unsigned i=0; i<Epix100aASIC_ConfigShadow::NumberOfRegisters; i++){
    _reg[i] = new NumericInt<uint32_t>(
                                       Epix100aASIC_ConfigShadow::name((Epix100aASIC_ConfigShadow::Registers) i),
                                       Epix100aASIC_ConfigShadow::defaultValue((Epix100aASIC_ConfigShadow::Registers) i),
                                       Epix100aASIC_ConfigShadow::rangeLow((Epix100aASIC_ConfigShadow::Registers) i),
                                       Epix100aASIC_ConfigShadow::rangeHigh((Epix100aASIC_ConfigShadow::Registers) i),
                                       Decimal
                                       );
  }
}

int Epix100aASICdata::pull(const Epix100aASIC_ConfigShadow& epixASIC_ConfigShadow) { // pull "from xtc"
  for (uint32_t i=0; i<Epix100aASIC_ConfigShadow::NumberOfRegisters; i++) {
    _reg[i]->value = epixASIC_ConfigShadow.get((Epix100aASIC_ConfigShadow::Registers) i);
  }
  return true;
}

int Epix100aASICdata::push(void* to) {
  Epix100aASIC_ConfigShadow& epixASIC_ConfigShadow = *new(to) Epix100aASIC_ConfigShadow(true);
  for (uint32_t i=0; i<Epix100aASIC_ConfigShadow::NumberOfRegisters; i++) {
    epixASIC_ConfigShadow.set((Epix100aASIC_ConfigShadow::Registers) i, _reg[i]->value);
  }
  
  return sizeof(Epix100aASIC_ConfigShadow);
}

Epix100aConfig::Epix100aConfig(bool expert) :
  Serializer("Epix100a_Config"),
  _private(new PrivateData(expert))
{
  name("EPIX100a Configuration");
  pList.insert(_private);
}

Epix100aConfig::~Epix100aConfig()
{
  delete _private;
}

int Epix100aConfig::readParameters(void* from) { // pull "from xtc"
  return _private->pull(from);
}

int Epix100aConfig::writeParameters(void* to) {
  return _private->push(to);
}

int Epix100aConfig::dataSize() const {
  return _private->dataSize();
}

