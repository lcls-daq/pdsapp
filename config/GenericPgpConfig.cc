#include "pdsapp/config/GenericPgpConfig.hh"
#include "pdsapp/config/GenericEpixConfig.hh"
#include "pdsapp/config/GenericEpix10kConfig.hh"
#include "pds/config/GenericPgpConfigType.hh"
#include "pds/config/EpixConfigType.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QComboBox>

#include <stdio.h>

enum GenericTypes { Epix100, Epix10k, NumGenericTypes };
static const char* key_names[] = { "Epix100",
                                   "Epix10k",
                                   NULL };
static const uint32_t key_values[] = { _epixConfigType.value(),
				       _epix10kConfigType.value() };

namespace Pds_ConfigDb {
  class GenericPgpConfig::Private_Data : public Parameter {
  public:
    Private_Data() : 
      _key    ("Type",Epix100,key_names), 
      _display(new GenericPgpDisplay) 
    {}
    ~Private_Data() { delete _display; }
  public:
    QLayout* initialize(QWidget* p) {
      QVBoxLayout* layout = new QVBoxLayout;
      { QHBoxLayout* h = static_cast<QHBoxLayout*>(_key.initialize(0));
        h->addStretch();
        layout->addLayout(h); }
      layout->addStretch();
      { QHBoxLayout* l = new QHBoxLayout;
        _display->setup(p, l, _key.allowEdit() ? _key._input:0); 
        layout->addLayout(l); }
      return layout;
    }
    void update() { _display->s().update(); }
    void flush () { _display->s().flush();  }
    void enable(bool v) { _key.enable(v);   }
  public:
    int pull(void* from) { 
      const GenericPgpConfigType& c = *new(from) GenericPgpConfigType;
      GenericTypes t = NumGenericTypes;
      for(unsigned i=0; i<NumGenericTypes; i++) {
	if (key_values[i]==c.stream()[0].config_type()) {
	  t = (GenericTypes)i;
	  break;
	}
      }
      if (t==NumGenericTypes) {
	printf("GenericPgpConfig stream config_type (0x%x) lookup failure\n",
	       c.stream()[0].config_type());
	return dataSize();
      }
      else if (t != _key.value) {
        _key.value = t;
        _key.flush();
	if (!_key.allowEdit())
	  _display->changed(t);
      }
      return _display->s().readParameters(from); 
    }
    int push(void* to)   { return _display->s().writeParameters(to); }
    int dataSize() const { return _display->s().dataSize(); }
  private:
    Enumerated<GenericTypes>     _key;
    GenericPgpDisplay*           _display;
  };
};

using namespace Pds_ConfigDb;

GenericPgpDisplay::GenericPgpDisplay() : _s(new GenericEpixConfig(key_values[Epix100])) {}

GenericPgpDisplay::~GenericPgpDisplay() {}

Serializer& GenericPgpDisplay::s() { return *_s; }

int  GenericPgpDisplay::read(void* from)
{
  const GenericPgpConfigType& c = *reinterpret_cast<const GenericPgpConfigType*>(from);

  GenericTypes t = NumGenericTypes;
  for(unsigned i=0; i<NumGenericTypes; i++) {
    if (key_values[i]==c.stream()[0].config_type()) {
      t = (GenericTypes)i;
      break;
    }
  }
  if (t==NumGenericTypes) {
    printf("GenericPgpConfig stream config_type (0x%x) lookup failure\n",
	   c.stream()[0].config_type());
    return _s->dataSize();
  }

  changed(t);
  return _s->readParameters(from);
}

void GenericPgpDisplay::setup(QWidget* p,QBoxLayout* l,QComboBox* box) 
{
  _s->initialize(_p=p,_l=l);
  if (box)
    connect(box, SIGNAL(currentIndexChanged(int)), this, SLOT(changed(int)));
}

static void deleteChildren(QLayout* l)
{
  QLayoutItem* child;
  while((child = l->takeAt(0)) !=0) {
    if (child->layout())
      deleteChildren(child->layout());
    if (child->widget())
      delete child->widget();
    delete child;
  }
}

void GenericPgpDisplay::changed(int k)
{
  printf("GenericPgpDisplay::changed %d\n",k);

  delete _s;
  deleteChildren(_l);

#define enroll(key, display) { if (k==key) _s = new display(key_values[key]); }
  enroll(Epix100, GenericEpixConfig);
  enroll(Epix10k, GenericEpix10kConfig);
#undef enroll
  _s->initialize(_p, _l);
}


GenericPgpConfig::GenericPgpConfig() :
  Serializer("GenericPgp_Config"),
  _private(new Private_Data)
{
  pList.insert(_private);
  name("GenericPgp Configuration");
}

GenericPgpConfig::~GenericPgpConfig()
{
  delete _private;
}

int GenericPgpConfig::readParameters(void* from) { // pull "from xtc"
  return _private->pull(from);
}

int GenericPgpConfig::writeParameters(void* to) {
  return _private->push(to);
}

int GenericPgpConfig::dataSize() const {
  return _private->dataSize();
}

#include "Parameters.icc"
