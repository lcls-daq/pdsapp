#include "pdsapp/config/Epix10kConfigP.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"

#include "pds/config/EpixConfigType.hh"
#include "pds/config/Epix10kASICConfigV1.hh"
#include "pds/config/Epix10kConfigV1.hh"

#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtCore/QObject>

#include <new>
#include <stdio.h>

#include <stdio.h>

namespace Pds_ConfigDb {

  class Epix10kSimpleCount : public ParameterCount {
    public:
    Epix10kSimpleCount(unsigned c) : mycount(c) {};
    ~Epix10kSimpleCount() {};
    bool connect(ParameterSet&) { return false; }
    unsigned count() { return mycount; }
    unsigned mycount;
  };

  class Epix10kASICdata : public Epix10kCopyTarget,
                       public Parameter {
    enum { Off, On };
  public:
    Epix10kASICdata();
    ~Epix10kASICdata();
  public:
    void copy(const Epix10kCopyTarget& s) {
      const Epix10kASICdata& src = static_cast<const Epix10kASICdata&>(s);
      for(unsigned i=0; i<Epix10kASIC_ConfigShadow::NumberOfRegisters; i++) {
        _reg[i]->value = src._reg[i]->value;
      }
    }
  public:
    QLayout* initialize(QWidget*) {
      QVBoxLayout* l = new QVBoxLayout;
      { QGridLayout* layout = new QGridLayout;
        for(unsigned i=0; i<Epix10kASIC_ConfigShadow::NumberOfRegisters; i++)
          layout->addLayout(_reg[i]->initialize(0), i%16, i/16);
        l->addLayout(layout); }

      for(unsigned i=0; i<Epix10kASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->enable((!Epix10kASIC_ConfigShadow::readOnly(Epix10kASIC_ConfigShadow::Registers(i))) ||
            (Epix10kASIC_ConfigShadow::readOnly(Epix10kASIC_ConfigShadow::Registers(i))==Epix10kASIC_ConfigShadow::WriteOnly));

      return l;
    }
    void update() {
      for(unsigned i=0; i<Epix10kASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->update();
    }
    void flush () {
      for(unsigned i=0; i<Epix10kASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->flush();
    }
    void enable(bool v) {
      for(unsigned i=0; i<Epix10kASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->enable(v && ((!Epix10kASIC_ConfigShadow::readOnly(Epix10kASIC_ConfigShadow::Registers(i))) ||
            (Epix10kASIC_ConfigShadow::readOnly(Epix10kASIC_ConfigShadow::Registers(i))==Epix10kASIC_ConfigShadow::WriteOnly)));
    }
  public:
    int pull(const Epix10kASIC_ConfigShadow&);
    int push(void*);
  public:
    NumericInt<uint32_t>*       _reg[Epix10kASIC_ConfigShadow::NumberOfRegisters];
  };

  static std::vector<Epix10kCopyTarget*> targets(Epix10kASICdata* a)
  {
    std::vector<Epix10kCopyTarget*> v(Epix10kConfigShadow::NumberOfAsics);
    for(unsigned i=0; i<Epix10kConfigShadow::NumberOfAsics; i++)
      v[i] = &a[i];
    return v;
  }
};

using namespace Pds_ConfigDb;

Epix10kASICdata::Epix10kASICdata() {
  for (unsigned i=0; i<Epix10kASIC_ConfigShadow::NumberOfRegisters; i++){
    _reg[i] = new NumericInt<uint32_t>(
                                       Epix10kASIC_ConfigShadow::name((Epix10kASIC_ConfigShadow::Registers) i),
                                       Epix10kASIC_ConfigShadow::defaultValue((Epix10kASIC_ConfigShadow::Registers) i),
                                       Epix10kASIC_ConfigShadow::rangeLow((Epix10kASIC_ConfigShadow::Registers) i),
                                       Epix10kASIC_ConfigShadow::rangeHigh((Epix10kASIC_ConfigShadow::Registers) i),
                                       Hex
                                       );
  }
}

Epix10kASICdata::~Epix10kASICdata() 
{
  for (unsigned i=0; i<Epix10kASIC_ConfigShadow::NumberOfRegisters; i++)
    delete _reg[i];
}

int Epix10kASICdata::pull(const Epix10kASIC_ConfigShadow& epixASIC_ConfigShadow) { // pull "from xtc"
  for (uint32_t i=0; i<Epix10kASIC_ConfigShadow::NumberOfRegisters; i++) {
    _reg[i]->value = epixASIC_ConfigShadow.get((Epix10kASIC_ConfigShadow::Registers) i);
  }
  return true;
}

int Epix10kASICdata::push(void* to) {
  Epix10kASIC_ConfigShadow& epixASIC_ConfigShadow = *new(to) Epix10kASIC_ConfigShadow(true);
  for (uint32_t i=0; i<Epix10kASIC_ConfigShadow::NumberOfRegisters; i++) {
    epixASIC_ConfigShadow.set((Epix10kASIC_ConfigShadow::Registers) i, _reg[i]->value);
  }
  
  return sizeof(Epix10kASIC_ConfigShadow);
}


Epix10kConfigP::Epix10kConfigP() :
  _count(new Epix10kSimpleCount(Epix10kConfigShadow::NumberOfAsics)),
  _test(false), _mask(false), _g(false), _ga(false),
  _asic(new Epix10kASICdata[Epix10kConfigShadow::NumberOfAsics]),
  _asicSet("ASICs", _asicArgs, *_count, targets(_asic)) {
  Epix10kConfigShadow::initNames();
  for (unsigned i=0; i<Epix10kConfigShadow::NumberOfRegisters; i++) {
    _reg[i] = new NumericInt<uint32_t>(
                                       Epix10kConfigShadow::name((Epix10kConfigShadow::Registers) i),
                                       Epix10kConfigShadow::defaultValue((Epix10kConfigShadow::Registers) i),
                                       Epix10kConfigShadow::rangeLow((Epix10kConfigShadow::Registers) i),
                                       Epix10kConfigShadow::rangeHigh((Epix10kConfigShadow::Registers) i),
                                       Hex
                                       );
  }
  for (uint32_t i=0; i<Epix10kConfigShadow::NumberOfAsics; i++) {
    _asicArgs[i].insert(&_asic[i]);
  }
  _asicSet.name("ASIC:");
}

Epix10kConfigP::~Epix10kConfigP() 
{
  delete _count;
  delete[] _asic;
  for (unsigned i=0; i<Epix10kConfigShadow::NumberOfRegisters; i++)
    delete _reg[i];
}

QLayout* Epix10kConfigP::initialize(QWidget*) {
  QHBoxLayout* layout = new QHBoxLayout;
  { QVBoxLayout* l = new QVBoxLayout;
    for (unsigned i=Epix10kConfigShadow::Version; i<Epix10kConfigShadow::NumberOfRegisters; i++) {
      if (Epix10kConfigShadow::readOnly((Epix10kConfigShadow::Registers)i) == Epix10kConfigShadow::ReadOnly) {
        l->addLayout(_reg[i]->initialize(0));
      }
    }
    l->addStretch();
    for (unsigned i=Epix10kConfigShadow::Version; i<Epix10kConfigShadow::NumberOfRegisters; i++) {
      if (Epix10kConfigShadow::readOnly((Epix10kConfigShadow::Registers)i) == Epix10kConfigShadow::UseOnly) {
        l->addLayout(_reg[i]->initialize(0));
      }
    }
    layout->addStretch();
    { QHBoxLayout* layout = new QHBoxLayout;
      { layout->addWidget(new QLabel("Pixel Mask"));
        _mask_gr = new QButtonGroup;
        QRadioButton* boff = new QRadioButton("Off");
        QRadioButton* bon  = new QRadioButton("On");
        _mask_gr->addButton(boff,Off);
        _mask_gr->addButton(bon ,On);
        layout->addWidget(boff);
        layout->addWidget(bon); }
      layout->addStretch();
      { layout->addWidget(new QLabel("Pixel Test"));
        _test_gr = new QButtonGroup;
        QRadioButton* boff = new QRadioButton("Off");
        QRadioButton* bon  = new QRadioButton("On");
        _test_gr->addButton(boff,Off);
        _test_gr->addButton(bon ,On);
        layout->addWidget(boff);
        layout->addWidget(bon); }
      layout->addStretch();
      l->addLayout(layout); }
    { QHBoxLayout* layout = new QHBoxLayout;
      { layout->addWidget(new QLabel("Pixel G"));
        _g_gr = new QButtonGroup;
        QRadioButton* boff = new QRadioButton("Off");
        QRadioButton* bon  = new QRadioButton("On");
        _g_gr->addButton(boff,Off);
        _g_gr->addButton(bon ,On);
        layout->addWidget(boff);
        layout->addWidget(bon); }
      layout->addStretch();
      { layout->addWidget(new QLabel("Pixel GA"));
        _ga_gr = new QButtonGroup;
        QRadioButton* boff = new QRadioButton("Off");
        QRadioButton* bon  = new QRadioButton("On");
        _ga_gr->addButton(boff,Off);
        _ga_gr->addButton(bon ,On);
        layout->addWidget(boff);
        layout->addWidget(bon); }
      layout->addStretch();
      l->addLayout(layout);}
    l->addLayout(_asicSet.initialize(0));
    layout->addLayout(l);
  }
  layout->addStretch();
  { QVBoxLayout* l = new QVBoxLayout;
    for (unsigned i=Epix10kConfigShadow::Version; i<Epix10kConfigShadow::prepulseR0En; i++) {
      if (Epix10kConfigShadow::readOnly((Epix10kConfigShadow::Registers)i) == Epix10kConfigShadow::ReadWrite) {
        l->addLayout(_reg[i]->initialize(0));
      }
    }
    layout->addLayout(l);
  }
  { QVBoxLayout* l = new QVBoxLayout;
    for (unsigned i=Epix10kConfigShadow::prepulseR0En; i<Epix10kConfigShadow::SyncWidth; i++) {
      if (Epix10kConfigShadow::readOnly((Epix10kConfigShadow::Registers)i) == Epix10kConfigShadow::ReadWrite) {
        l->addLayout(_reg[i]->initialize(0));
      }
    }
    layout->addLayout(l);
  }
  layout->addStretch();
  { QVBoxLayout* l = new QVBoxLayout;
    for (unsigned i=Epix10kConfigShadow::SyncWidth; i<Epix10kConfigShadow::NumberOfRegisters; i++) {
      if (Epix10kConfigShadow::readOnly((Epix10kConfigShadow::Registers)i) == Epix10kConfigShadow::ReadWrite) {
        l->addLayout(_reg[i]->initialize(0));
      }
    }
    layout->addLayout(l); }
      
  for(unsigned i=0; i<Epix10kConfigShadow::NumberOfRegisters; i++)
    _reg[i]->enable(!(Epix10kConfigShadow::readOnly(Epix10kConfigShadow::Registers(i)) == Epix10kConfigShadow::ReadOnly));
  _mask_gr->button(_mask ? On:Off)->setChecked(true);
  _test_gr->button(_test ? On:Off)->setChecked(true);
  _g_gr->button(_g ? On:Off)->setChecked(true);
  _ga_gr->button(_ga ? On:Off)->setChecked(true);
  return layout;
}

void Epix10kConfigP::update() {
  for(unsigned i=0; i<Epix10kConfigShadow::NumberOfRegisters; i++)
    _reg[i]->update();
  _mask = _mask_gr->button(On)->isChecked();
  _test = _test_gr->button(On)->isChecked();
  _g = _g_gr->button(On)->isChecked();
  _ga = _ga_gr->button(On)->isChecked();
  _asicSet.update();
}

void Epix10kConfigP::flush () {
  for(unsigned i=0; i<Epix10kConfigShadow::NumberOfRegisters; i++)
    _reg[i]->flush();
  _mask_gr->button(_mask ? On:Off)->setChecked(true);
  _test_gr->button(_test ? On:Off)->setChecked(true);
  _g_gr->button(_g ? On:Off)->setChecked(true);
  _ga_gr->button(_ga ? On:Off)->setChecked(true);
  _asicSet.flush();
}

void Epix10kConfigP::enable(bool v) {
  for(unsigned i=0; i<Epix10kConfigShadow::NumberOfRegisters; i++)
    _reg[i]->enable(v && (Epix10kConfigShadow::readOnly(Epix10kConfigShadow::Registers(i)) == Epix10kConfigShadow::ReadWrite));
  _asicSet.enable(v);
}

int Epix10kConfigP::pull(void* from) {
  Epix10kConfigType& epixConf = *new(from) Epix10kConfigType;
  Epix10kConfigShadow& epixConfShadow = *new(from) Epix10kConfigShadow;
  //    printf("Epix10kConfig::readParameters:");
  for (uint32_t i=0; i<Epix10kConfigShadow::NumberOfRegisters; i++) {
    _reg[i]->value = epixConfShadow.get((Epix10kConfigShadow::Registers) i);
    //      printf("%s0x%x",  i%16==0 ? "\n\t" : " ", epixConfShadow.get((Epix10kConfigShadow::Registers) i));
  }
  _pixel = epixConf.asicPixelConfigArray()(0,0,0);
  pixel2bits();
  //    printf("\n returning size(%u), from %u %u %u %u\n",
  //       epixConf._sizeof(),
  //       epixConf.numberOfAsicsPerRow(),
  //       epixConf.numberOfAsicsPerColumn(),
  //       epixConf.numberOfRowsPerAsic(),
  //       epixConf.numberOfPixelsPerAsicRow());
  for (uint32_t i=0; i<Epix10kConfigShadow::NumberOfAsics; i++) {
    _asic[i].pull(reinterpret_cast<const Epix10kASIC_ConfigShadow&>(epixConf.asics(i)));
  }
  return epixConf._sizeof();
}

int Epix10kConfigP::push(void* to) {
  Epix10kConfigType& epixConf = *new(to) Epix10kConfigType;
  Epix10kConfigShadow& epixConfShadow = *new(to) Epix10kConfigShadow(true);
  for (uint32_t i=0; i<Epix10kConfigShadow::NumberOfRegisters; i++) {
    epixConfShadow.set((Epix10kConfigShadow::Registers) i, _reg[i]->value);
  }
  bits2pixel();
  ndarray<const uint16_t,3> array = epixConf.asicPixelConfigArray();
  uint16_t* a = const_cast<uint16_t*>(array.begin());
  while (a!=array.end()) *a++ = _pixel;
  for (uint32_t i=0; i<Epix10kConfigShadow::NumberOfAsics; i++) {
    _asic[i].push((void*)(&(epixConf.asics(i))));
  }
  return epixConf._sizeof();
}

int Epix10kConfigP::dataSize() const {
  Epix10kConfigType* foo = new Epix10kConfigType(
                                                 _reg[Epix10kConfigShadow::NumberOfAsicsPerRow]->value,
                                                 _reg[Epix10kConfigShadow::NumberOfAsicsPerColumn]->value,
                                                 _reg[Epix10kConfigShadow::NumberOfRowsPerAsic]->value,
                                                 _reg[Epix10kConfigShadow::NumberOfPixelsPerAsicRow]->value,
						 _reg[Epix10kConfigShadow::LastRowExclusions]->value);
  int size = (int) foo->_sizeof();
  //    printf("Epix10kConfig::dataSize apr(%u) apc(%u) rpa(%u) ppar(%u) size(%u)\n",
  //        _reg[Epix10kConfigShadow::NumberOfAsicsPerRow]->value,
  //        _reg[Epix10kConfigShadow::NumberOfAsicsPerColumn]->value,
  //        _reg[Epix10kConfigShadow::NumberOfRowsPerAsic]->value,
  //        _reg[Epix10kConfigShadow::NumberOfPixelsPerAsicRow]->value,
  //        size);
  delete foo;
  return size;
}

uint16_t Epix10kConfigP::bits2pixel() {
  _pixel = 0;
  _pixel |= _test ? 1 : 0;
  _pixel |= _mask ? 2 : 0;
  _pixel |= _g    ? 4 : 0;
  _pixel |= _ga   ? 8 : 0;
  printf("bist2pixel %s %s %s %s 0x%x\n",_test ? "true":"false",_mask ? "true":"false",
         _g ?"true":"false",_ga ?"true":"false", _pixel);
  return _pixel;
}

void Epix10kConfigP::pixel2bits() {
  _test = _pixel & 1;
  _mask = _pixel & 2;
  _g    = _pixel & 4;
  _ga   = _pixel & 8;
  printf("pixel2bits 0x%x %s %s %s %s\n", _pixel,_test ? "true":"false",_mask ? "true":"false",
         _g ?"true":"false",_ga ?"true":"false");
}

