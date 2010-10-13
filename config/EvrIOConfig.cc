#include <new>

#include "EvrIOConfig.hh"
#include "EvrIOChannel.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/EvrIOConfigType.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>

using namespace Pds_ConfigDb;

static const int MaxOutputs = 10;

static const char* connTypes[] = { "FrontPanel", "UnivIO", NULL };

namespace Pds_ConfigDb
{
  class EvrIOConfig::Private_Data : public Parameter {
  public:
    Private_Data() : Parameter(NULL),
      _conn("IO Module", Pds::EvrData::OutputMap::UnivIO, connTypes)
    {
      EvrIOChannel::initialize();
      for(int i=0; i<MaxOutputs; i++)
	_channel[i] = new EvrIOChannel(i);
    }
  public:
    void insert(Pds::LinkedList<Parameter>& pList)
    {
      pList.insert(this);
      
      for(int i=0; i<MaxOutputs; i++)
	_channel[i]->insert(_pList);
    }

    int pull(const void* from) {
      const EvrIOConfigType& tc = *reinterpret_cast<const EvrIOConfigType*>(from);
      _conn.value = tc.conn();
      for(unsigned i=0; i<tc.nchannels(); i++)
	_channel[i]->pull(tc.channel(i));
      return tc.size();
    }

    int push(void* to) const {
      EvrIOConfigType& tc = *reinterpret_cast<EvrIOConfigType*>(to);
      Pds::EvrData::IOChannel ch[MaxOutputs];
      for(int i=0; i<MaxOutputs; i++)
	_channel[i]->push(ch[i]);
      *new(&tc) EvrIOConfigType(_conn.value, ch, MaxOutputs);
      return tc.size();
    }

    int dataSize() const {
      return sizeof(EvrIOConfigType) + MaxOutputs*sizeof(Pds::EvrData::IOChannel);
    }

  public:
    QLayout* initialize(QWidget* parent) 
    {
      QVBoxLayout* layout = new QVBoxLayout;
      QHBoxLayout* h = new QHBoxLayout;
      h->addLayout(_conn.initialize(parent));
      h->addStretch();
      layout->addLayout(h);

      QGridLayout* l = new QGridLayout;
      int row = 0, column = 0;
      l->addWidget( new QLabel("Conn")    , row, column++, Qt::AlignCenter);
      l->addWidget( new QLabel("Label")   , row, column++, Qt::AlignCenter);
      l->addWidget( new QLabel("Devices") , row, column++, Qt::AlignCenter);
      column++;
      column++;
      l->addWidget( new QLabel("Detector"), row, column++, Qt::AlignCenter);
      l->addWidget( new QLabel("DetIndex"), row, column++, Qt::AlignCenter);
      l->addWidget( new QLabel("Device")  , row, column++, Qt::AlignCenter);
      l->addWidget( new QLabel("DevIndex"), row, column++, Qt::AlignCenter);
      row++;
      column = 0;
      for(int i=0; i<MaxOutputs; i++, row++)
	_channel[i]->layout(l,row);
      layout->addLayout(l);

      return layout;
    }

    void update() {
      _conn.update();

      Parameter* p = _pList.forward();
      while( p != _pList.empty() ) {
	p->update();
	p = p->forward();
      }
    }

    void flush() {
      _conn.flush();

      Parameter* p = _pList.forward();
      while( p != _pList.empty() ) {
	p->flush();
	p = p->forward();
      }
    }

    void enable(bool) {
    }

  public:
    Enumerated<Pds::EvrData::OutputMap::Conn> _conn;
    EvrIOChannel*                             _channel[MaxOutputs];
    Pds::LinkedList<Parameter>                _pList;
  };
};


EvrIOConfig::EvrIOConfig():
Serializer("Evr_Config"), _private_data(new Private_Data)
{
  _private_data->insert(pList);
}

int EvrIOConfig::readParameters(void *from)
{
  return _private_data->pull(from);
}

int EvrIOConfig::writeParameters(void *to)
{
  return _private_data->push(to);
}

int EvrIOConfig::dataSize() const
{
  return _private_data->dataSize();
}

#include "Parameters.icc"
