#include "pdsapp/config/Epix10kaASICdata.hh"
#include "pdsapp/config/Parameters.hh"
#include "pds/config/EpixConfigType.hh"

#include <stdio.h>

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QGridLayout>

using namespace Pds_ConfigDb;

static unsigned Columns=3;

void Epix10kaASICdata::setColumns(unsigned c) { Columns=c; }

Epix10kaASICdata::Epix10kaASICdata() {
  for (unsigned i=0; i<Epix10kaASIC_ConfigShadow::NumberOfRegisters; i++){
    _reg[i] = new NumericInt<uint32_t>(
                                       Epix10kaASIC_ConfigShadow::name((Epix10kaASIC_ConfigShadow::Registers) i),
                                       Epix10kaASIC_ConfigShadow::defaultValue((Epix10kaASIC_ConfigShadow::Registers) i),
                                       Epix10kaASIC_ConfigShadow::rangeLow((Epix10kaASIC_ConfigShadow::Registers) i),
                                       Epix10kaASIC_ConfigShadow::rangeHigh((Epix10kaASIC_ConfigShadow::Registers) i),
                                       Hex
                                       );
  }
}

Epix10kaASICdata::~Epix10kaASICdata() {}

void Epix10kaASICdata::copy(const Epix10kaCopyTarget& s) 
{
  const Epix10kaASICdata& src = static_cast<const Epix10kaASICdata&>(s);
  for(unsigned i=0; i<Epix10kaASIC_ConfigShadow::NumberOfRegisters; i++) {
    if (Epix10kaASIC_ConfigShadow::doNotCopy(Epix10kaASIC_ConfigShadow::Registers(i)) == Epix10kaASIC_ConfigShadow::DoCopy) {
      _reg[i]->value = src._reg[i]->value;
    }
    else {
      printf("Epix10kaASICdata::copy did not copy %s\n", Epix10kaASIC_ConfigShadow::name(Epix10kaASIC_ConfigShadow::Registers(i)));
    }
  }
}

QLayout* Epix10kaASICdata::initialize(QWidget*) 
{
  QVBoxLayout* l = new QVBoxLayout;
  { QGridLayout* layout = new QGridLayout;
    unsigned mod = ( Epix10kaASIC_ConfigShadow::NumberOfRegisters + Columns-1) / Columns;
    for(unsigned i=0; i<Epix10kaASIC_ConfigShadow::NumberOfRegisters; i++)
      layout->addLayout(_reg[i]->initialize(0), i%mod, i/mod);
    l->addLayout(layout);
  }

  for(unsigned i=0; i<Epix10kaASIC_ConfigShadow::NumberOfRegisters; i++)
    _reg[i]->enable((!Epix10kaASIC_ConfigShadow::readOnly(Epix10kaASIC_ConfigShadow::Registers(i))) ||
                    (Epix10kaASIC_ConfigShadow::readOnly(Epix10kaASIC_ConfigShadow::Registers(i))==Epix10kaASIC_ConfigShadow::WriteOnly));
  return l;
}

void Epix10kaASICdata::update() 
{
  for(unsigned i=0; i<Epix10kaASIC_ConfigShadow::NumberOfRegisters; i++)
    _reg[i]->update();
}

void Epix10kaASICdata::flush () 
{
  for(unsigned i=0; i<Epix10kaASIC_ConfigShadow::NumberOfRegisters; i++)
    _reg[i]->flush();
}

void Epix10kaASICdata::enable(bool v) 
{
  for(unsigned i=0; i<Epix10kaASIC_ConfigShadow::NumberOfRegisters; i++)
    _reg[i]->enable(v && ((!Epix10kaASIC_ConfigShadow::readOnly(Epix10kaASIC_ConfigShadow::Registers(i))) ||
                          (Epix10kaASIC_ConfigShadow::readOnly(Epix10kaASIC_ConfigShadow::Registers(i))==Epix10kaASIC_ConfigShadow::WriteOnly)));
}

int Epix10kaASICdata::pull(const Epix10kaASIC_ConfigShadow& epixASIC_ConfigShadow) { // pull "from xtc"
  for (uint32_t i=0; i<Epix10kaASIC_ConfigShadow::NumberOfRegisters; i++) {
    _reg[i]->value = epixASIC_ConfigShadow.get((Epix10kaASIC_ConfigShadow::Registers) i);
  }
  return true;
}

int Epix10kaASICdata::push(void* to) {
  Epix10kaASIC_ConfigShadow& epixASIC_ConfigShadow = *new(to) Epix10kaASIC_ConfigShadow(true);
  for (uint32_t i=0; i<Epix10kaASIC_ConfigShadow::NumberOfRegisters; i++) {
    epixASIC_ConfigShadow.set((Epix10kaASIC_ConfigShadow::Registers) i, _reg[i]->value);
  }
  
  return sizeof(Epix10kaASIC_ConfigShadow);
}

