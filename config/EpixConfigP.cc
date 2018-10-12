#include "pdsapp/config/EpixConfigP.hh"
#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"

#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixASICConfigV1.hh"
#include "pds/config/EpixConfigV1.hh"

#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QButtonGroup>
#include <QtGui/QRadioButton>
#include <QtCore/QObject>

#include <new>

namespace Pds_ConfigDb {

  class EpixSimpleCount : public ParameterCount {
    public:
    EpixSimpleCount(unsigned c) : mycount(c) {};
    ~EpixSimpleCount() {};
    bool connect(ParameterSet&) { return false; }
    unsigned count() { return mycount; }
    unsigned mycount;
  };

  class EpixASICdata : public EpixCopyTarget,
                       public Parameter {
    enum { Off, On };
  public:
    EpixASICdata();
    ~EpixASICdata();
  public:
    void copy(const EpixCopyTarget& s) {
      const EpixASICdata& src = static_cast<const EpixASICdata&>(s);
      for(unsigned i=0; i<EpixASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->value = src._reg[i]->value;
      _mask = src._mask;
      _test = src._test;
    }
  public:
    QLayout* initialize(QWidget*) {
      QVBoxLayout* l = new QVBoxLayout;
      { QGridLayout* layout = new QGridLayout;
        for(unsigned i=0; i<EpixASIC_ConfigShadow::NumberOfRegisters; i++)
          layout->addLayout(_reg[i]->initialize(0), i%15, i/15);
        l->addLayout(layout); }
      { QHBoxLayout* layout = new QHBoxLayout;
        layout->addStretch();
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

      for(unsigned i=0; i<EpixASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->enable((!EpixASIC_ConfigShadow::readOnly(EpixASIC_ConfigShadow::Registers(i))) ||
            (EpixASIC_ConfigShadow::readOnly(EpixASIC_ConfigShadow::Registers(i))==EpixASIC_ConfigShadow::WriteOnly));

      _mask_gr->button(_mask ? On:Off)->setChecked(true);
      _test_gr->button(_test ? On:Off)->setChecked(true);
      return l;
    }
    void update() {
      for(unsigned i=0; i<EpixASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->update();
      _mask = _mask_gr->button(On)->isChecked();
      _test = _test_gr->button(On)->isChecked();
    }
    void flush () {
      for(unsigned i=0; i<EpixASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->flush();
      _mask_gr->button(_mask ? On:Off)->setChecked(true);
      _test_gr->button(_test ? On:Off)->setChecked(true);
    }
    void enable(bool v) {
      for(unsigned i=0; i<EpixASIC_ConfigShadow::NumberOfRegisters; i++)
        _reg[i]->enable(v && ((!EpixASIC_ConfigShadow::readOnly(EpixASIC_ConfigShadow::Registers(i))) ||
            (EpixASIC_ConfigShadow::readOnly(EpixASIC_ConfigShadow::Registers(i))==EpixASIC_ConfigShadow::WriteOnly)));
    }
  public:
    int pull(const EpixASIC_ConfigShadow&,
             const ndarray<const uint32_t,2> mask,
             const ndarray<const uint32_t,2> test);
    int push(void*,
             ndarray<const uint32_t,2> mask,
             ndarray<const uint32_t,2> test);
  public:
    NumericInt<uint32_t>*       _reg[EpixASIC_ConfigShadow::NumberOfRegisters];
    bool                        _mask;
    bool                        _test;
    QButtonGroup*               _mask_gr;
    QButtonGroup*               _test_gr;
  };

  static std::vector<EpixCopyTarget*> targets(EpixASICdata* a)
  {
    std::vector<EpixCopyTarget*> v(EpixConfigShadow::NumberOfAsics);
    for(unsigned i=0; i<EpixConfigShadow::NumberOfAsics; i++)
      v[i] = &a[i];
    return v;
  }
};

using namespace Pds_ConfigDb;

EpixASICdata::EpixASICdata() :
  _mask(false), _test(false) {
  for (unsigned i=0; i<EpixASIC_ConfigShadow::NumberOfRegisters; i++){
    _reg[i] = new NumericInt<uint32_t>(
                                       EpixASIC_ConfigShadow::name((EpixASIC_ConfigShadow::Registers) i),
                                       EpixASIC_ConfigShadow::defaultValue((EpixASIC_ConfigShadow::Registers) i),
                                       EpixASIC_ConfigShadow::rangeLow((EpixASIC_ConfigShadow::Registers) i),
                                       EpixASIC_ConfigShadow::rangeHigh((EpixASIC_ConfigShadow::Registers) i),
                                       Hex
                                       );
  }
}

EpixASICdata::~EpixASICdata()
{
  for (unsigned i=0; i<EpixASIC_ConfigShadow::NumberOfRegisters; i++)
    delete _reg[i];
}

int EpixASICdata::pull(const EpixASIC_ConfigShadow& epixASIC_ConfigShadow,
                       const ndarray<const uint32_t,2> mask,
                       const ndarray<const uint32_t,2> test) { // pull "from xtc"
  for (uint32_t i=0; i<EpixASIC_ConfigShadow::NumberOfRegisters; i++) {
    _reg[i]->value = epixASIC_ConfigShadow.get((EpixASIC_ConfigShadow::Registers) i);
  }
  _mask = (mask(0,0)!=0);
  _test = (test(0,0)!=0);
  return true;
}

int EpixASICdata::push(void* to,
                       ndarray<const uint32_t,2> mask,
                       ndarray<const uint32_t,2> test) {
  EpixASIC_ConfigShadow& epixASIC_ConfigShadow = *new(to) EpixASIC_ConfigShadow(true);
  for (uint32_t i=0; i<EpixASIC_ConfigShadow::NumberOfRegisters; i++) {
    epixASIC_ConfigShadow.set((EpixASIC_ConfigShadow::Registers) i, _reg[i]->value);
  }
  
  { uint32_t* a = const_cast<uint32_t*>(mask.begin());
    if (_mask)
      while (a!=mask.end())
        *a++ = 0xffffffff;
    else
      while (a!=mask.end())
        *a++ = 0; }
  { uint32_t* a = const_cast<uint32_t*>(test.begin());
    if (_test)
      while (a!=test.end())
        *a++ = 0xffffffff;
    else
      while (a!=test.end())
        *a++ = 0; }

  return sizeof(EpixASIC_ConfigShadow);
}

EpixConfigP::EpixConfigP() :
  _count(new EpixSimpleCount(EpixConfigShadow::NumberOfAsics)),
  _asic(new EpixASICdata[EpixConfigShadow::NumberOfAsics]),
  _asicSet("ASICs", _asicArgs, *_count, targets(_asic)) {
  EpixConfigShadow::initNames();
  for (unsigned i=0; i<EpixConfigShadow::NumberOfRegisters; i++) {
    _reg[i] = new NumericInt<uint32_t>(
                                       EpixConfigShadow::name((EpixConfigShadow::Registers) i),
                                       EpixConfigShadow::defaultValue((EpixConfigShadow::Registers) i),
                                       EpixConfigShadow::rangeLow((EpixConfigShadow::Registers) i),
                                       EpixConfigShadow::rangeHigh((EpixConfigShadow::Registers) i),
                                       Hex
                                       );
  }
  for (uint32_t i=0; i<EpixConfigShadow::NumberOfAsics; i++) {
    _asicArgs[i].insert(&_asic[i]);
  }
  _asicSet.name("ASIC:");
}

EpixConfigP::~EpixConfigP() 
{
  delete _count;
  delete[] _asic;
  for(unsigned i=0; i<EpixConfigShadow::NumberOfRegisters; i++)
    delete _reg[i];
}

QLayout* EpixConfigP::initialize(QWidget*) {
  QHBoxLayout* layout = new QHBoxLayout;
  { QVBoxLayout* l = new QVBoxLayout;
    l->addLayout(_reg[EpixConfigShadow::Version]->initialize(0));
    l->addLayout(_reg[EpixConfigShadow::DigitalCardId0]->initialize(0));
    l->addLayout(_reg[EpixConfigShadow::DigitalCardId1]->initialize(0));
    l->addLayout(_reg[EpixConfigShadow::AnalogCardId0]->initialize(0));
    l->addLayout(_reg[EpixConfigShadow::AnalogCardId1]->initialize(0));
    l->addLayout(_reg[EpixConfigShadow::BaseClockFrequency]->initialize(0));
    l->addStretch();
    l->addLayout(_reg[EpixConfigShadow::NumberOfAsicsPerRow]->initialize(0));
    l->addLayout(_reg[EpixConfigShadow::NumberOfAsicsPerColumn]->initialize(0));
    l->addLayout(_reg[EpixConfigShadow::NumberOfRowsPerAsic]->initialize(0));
    l->addLayout(_reg[EpixConfigShadow::NumberOfPixelsPerAsicRow]->initialize(0));
    l->addLayout(_reg[EpixConfigShadow::AsicMask]->initialize(0));
    l->addLayout(_asicSet.initialize(0));
    layout->addLayout(l); }
  layout->addStretch();
  { QVBoxLayout* l = new QVBoxLayout;
    for(unsigned i=EpixConfigShadow::RunTrigDelay; 
        i<EpixConfigShadow::adcStreamMode; i++)
      l->addLayout(_reg[i]->initialize(0));
    layout->addLayout(l); }
  layout->addStretch();
  { QVBoxLayout* l = new QVBoxLayout;
    for(unsigned i=EpixConfigShadow::adcStreamMode; 
        i<EpixConfigShadow::DigitalCardId0; i++)
      l->addLayout(_reg[i]->initialize(0));
    l->addLayout(_reg[EpixConfigShadow::LastRowExclusions]->initialize(0));
    layout->addLayout(l); }
      
  for(unsigned i=0; i<EpixConfigShadow::NumberOfRegisters; i++)
    _reg[i]->enable(!(EpixConfigShadow::readOnly(EpixConfigShadow::Registers(i)) == EpixConfigShadow::ReadOnly));
  return layout;
}

void EpixConfigP::update() {
  for(unsigned i=0; i<EpixConfigShadow::NumberOfRegisters; i++)
    _reg[i]->update();
  _asicSet.update();
}

void EpixConfigP::flush () {
  for(unsigned i=0; i<EpixConfigShadow::NumberOfRegisters; i++)
    _reg[i]->flush();
  _asicSet.flush();
}

void EpixConfigP::enable(bool v) {
  for(unsigned i=0; i<EpixConfigShadow::NumberOfRegisters; i++)
    _reg[i]->enable(v && (EpixConfigShadow::readOnly(EpixConfigShadow::Registers(i)) == EpixConfigShadow::ReadWrite));
  _asicSet.enable(v);
}

int EpixConfigP::pull(void* from) {
  EpixConfigType& epixConf = *new(from) EpixConfigType;
  EpixConfigShadow& epixConfShadow = *new(from) EpixConfigShadow;
  //    printf("EpixConfig::readParameters:");
  for (uint32_t i=0; i<EpixConfigShadow::NumberOfRegisters; i++) {
    _reg[i]->value = epixConfShadow.get((EpixConfigShadow::Registers) i);
    //      printf("%s0x%x",  i%16==0 ? "\n\t" : " ", epixConfShadow.get((EpixConfigShadow::Registers) i));
  }
  //    printf("\n returning size(%u), from %u %u %u %u\n",
  //       epixConf._sizeof(),
  //       epixConf.numberOfAsicsPerRow(),
  //       epixConf.numberOfAsicsPerColumn(),
  //       epixConf.numberOfRowsPerAsic(),
  //       epixConf.numberOfPixelsPerAsicRow());
  for (uint32_t i=0; i<EpixConfigShadow::NumberOfAsics; i++) {
    _asic[i].pull(reinterpret_cast<const EpixASIC_ConfigShadow&>(epixConf.asics(i)),
                  epixConf.asicPixelMaskArray()[i],
                  epixConf.asicPixelTestArray()[i]);
  }
  return epixConf._sizeof();
}

int EpixConfigP::push(void* to) {
  EpixConfigType& epixConf = *new(to) EpixConfigType;
  EpixConfigShadow& epixConfShadow = *new(to) EpixConfigShadow(true);
  for (uint32_t i=0; i<EpixConfigShadow::NumberOfRegisters; i++) {
    epixConfShadow.set((EpixConfigShadow::Registers) i, _reg[i]->value);
  }
  for (uint32_t i=0; i<EpixConfigShadow::NumberOfAsics; i++) {
    _asic[i].push((void*)(&(epixConf.asics(i))),
                  epixConf.asicPixelMaskArray()[i],
                  epixConf.asicPixelTestArray()[i]);
  }
  return epixConf._sizeof();
}

int EpixConfigP::dataSize() const {
  EpixConfigType* foo = new EpixConfigType(
                                           _reg[EpixConfigShadow::NumberOfAsicsPerRow]->value,
                                           _reg[EpixConfigShadow::NumberOfAsicsPerColumn]->value,
                                           _reg[EpixConfigShadow::NumberOfRowsPerAsic]->value,
                                           _reg[EpixConfigShadow::NumberOfPixelsPerAsicRow]->value);
  int size = (int) foo->_sizeof();
  //    printf("EpixConfig::dataSize apr(%u) apc(%u) rpa(%u) ppar(%u) size(%u)\n",
  //        _reg[EpixConfigShadow::NumberOfAsicsPerRow]->value,
  //        _reg[EpixConfigShadow::NumberOfAsicsPerColumn]->value,
  //        _reg[EpixConfigShadow::NumberOfRowsPerAsic]->value,
  //        _reg[EpixConfigShadow::NumberOfPixelsPerAsicRow]->value,
  //        size);
  delete foo;
  return size;
}

