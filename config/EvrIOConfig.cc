#include <new>

#include "EvrIOConfig.hh"
#include "EvrIOChannel.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/EvrConfigType.hh"
#include "pds/config/EvrIOConfigType.hh"

#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QIntValidator>
#include <QtGui/QDoubleValidator>
#include <QtGui/QTabWidget>
#include <QtGui/QPushButton>

#include <stdio.h>

using namespace Pds_ConfigDb;

static const int MaxOutputs = 13;
static const unsigned MaxEVRs = 8;

static const char* connTypes[] = { "FrontPanel", "UnivIO", NULL };

namespace Pds_ConfigDb
{
  class EvrIOConfig::Panel : public Parameter {
  public:
    Panel(unsigned id) : 
      Parameter(NULL),
      _id  (id),
      _conn("IO Module", Pds::EvrData::OutputMap::UnivIO, connTypes)
    {
      for(int i=0; i<MaxOutputs; i++)
	_channel[i] = new EvrIOChannel(i);
    }
    ~Panel()
    {
      for(int i=0; i<MaxOutputs; i++)
	delete _channel[i];
    }
  public:
    void insert(Pds::LinkedList<Parameter>& pList)
    {
      pList.insert(this);
      
      for(int i=0; i<MaxOutputs; i++)
	_channel[i]->insert(_pList);
    }

    void clear() {
    }

    int pull(const EvrIOChannelType& tc) {
      _channel[tc.output().conn_id()]->pull(tc);
      return tc._sizeof();
    }

    int push(std::vector<EvrIOChannelType>& ch) const {
      for(int i=0; i<MaxOutputs; i++) {
	EvrIOChannelType chan;
        _channel[i]->push(chan,_id);
	ch.push_back(chan);
      }
      return MaxOutputs*sizeof(EvrIOChannelType);
    }

    int dataSize() const {
      return MaxOutputs*sizeof(EvrIOChannelType);
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
      if (allowEdit()) {
	column++;
	column++;
	l->addWidget( new QLabel("Detector"), row, column++, Qt::AlignCenter);
	l->addWidget( new QLabel("DetIndex"), row, column++, Qt::AlignCenter);
	l->addWidget( new QLabel("Device")  , row, column++, Qt::AlignCenter);
	l->addWidget( new QLabel("DevIndex"), row, column++, Qt::AlignCenter);
      }
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
    unsigned                                  _id;
    Enumerated<Pds::EvrData::OutputMap::Conn> _conn;
    EvrIOChannel*                             _channel[MaxOutputs];
    Pds::LinkedList<Parameter>                _pList;
  };


  class EvrIOConfig::Private_Data : public Parameter {
  public:
    Private_Data() : Parameter(NULL)
    {
      EvrIOChannel::initialize();
      for(unsigned i=0; i<MaxEVRs; i++)
        _evr[i] = new EvrIOConfig::Panel(i);
    }
    ~Private_Data() 
    {
      for(unsigned i=0; i<MaxEVRs; i++)
        delete _evr[i];
    }
  public:
    void insert(Pds::LinkedList<Parameter>& pList)
    {
      pList.insert(this);
      for(unsigned i=0; i<MaxEVRs; i++)
        _evr[i]->insert(_pList);
    }

    int pull(const void* from) {
      const EvrIOConfigType& tc = *reinterpret_cast<const EvrIOConfigType*>(from);

      for(unsigned i=0; i<MaxEVRs; i++)
	_evr[i]->clear();

      unsigned mask=0;
      for(unsigned i=0; i<tc.nchannels(); i++) {
	const EvrIOChannelType& ch = tc.channels()[i];
	_evr[ch.output().module()]->pull(ch);
	mask |= (1<<ch.output().module());
      }

      for(unsigned i=0; i<MaxEVRs; i++)
	_tab->setTabEnabled(i,(mask&(1<<i)));

      return tc._sizeof();
    }

    int push(void* to) const {
      std::vector<EvrIOChannelType> ch;
      for(unsigned i=0; i<MaxEVRs; i++)
	if (_tab->isTabEnabled(i))
	  _evr[i]->push(ch);

      EvrIOConfigType& tc = *new (to) EvrIOConfigType(ch.size(),ch.data());
      return tc._sizeof();
    }

    int dataSize() const {
      std::vector<EvrIOChannelType> ch;
      for(unsigned i=0; i<MaxEVRs; i++)
	if (_tab->isTabEnabled(i))
	  _evr[i]->push(ch);
      
      EvrIOConfigType tc(ch.size());
      return tc._sizeof();
    }

  public:
    QLayout* initialize(QWidget* parent) 
    {
      _qlink = new EvrIOConfigQ(*this, parent);

      QVBoxLayout* l = new QVBoxLayout;
      l->addStretch();
      if (allowEdit()) {
	QHBoxLayout* h = new QHBoxLayout;
        QPushButton* addB = new QPushButton("Add EVR");
        QObject::connect(addB, SIGNAL(pressed()), _qlink, SLOT(addEvr()));
        QPushButton* remB = new QPushButton("Remove EVR");
        QObject::connect(remB, SIGNAL(pressed()), _qlink, SLOT(remEvr()));
        h->addStretch();
        h->addWidget(addB);
        h->addStretch();
        h->addWidget(remB);
        h->addStretch();
        l->addLayout(h); 
      }
      l->addStretch();
      { _tab = new QTabWidget;
        for(unsigned i=0; i<MaxEVRs; i++) {
          QWidget* w = new QWidget;
          w->setLayout(_evr[i]->initialize(0));
          _tab->addTab(w,QString("EVR %1").arg(i));
          _tab->setTabEnabled(i,false);
        }
        l->addWidget(_tab); 
        }

      addEvr();

      return l;
    }

    void update() {
      Parameter* p = _pList.forward();
      while( p != _pList.empty() ) {
	p->update();
	p = p->forward();
      }
    }

    void flush() {
      Parameter* p = _pList.forward();
      while( p != _pList.empty() ) {
	p->flush();
	p = p->forward();
      }
    } 

    void enable(bool) {
    }

    void addEvr() {
      for(unsigned i=0; i<MaxEVRs; i++)
	if (!_tab->isTabEnabled(i)) {
	  _tab->setTabEnabled(i,true);
	  break;
	}
    }

    void remEvr() {
      for(unsigned i=MaxEVRs-1; i!=0; i--)
	if (_tab->isTabEnabled(i)) {
	  _tab->setTabEnabled(i,false);
	  break;
	}
    }

  public:
    EvrIOConfig::Panel*                       _evr[MaxEVRs];
    Pds::LinkedList<Parameter>                _pList;
    QTabWidget*                               _tab;
    EvrIOConfigQ*                             _qlink;
  };
};


EvrIOConfig::EvrIOConfig():
  Serializer("Evr_Config"), 
  _private_data(new Private_Data)
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


EvrIOConfigQ::EvrIOConfigQ(EvrIOConfig::Private_Data& c,
                           QWidget* w) : QObject(w), _c(c) {}

void EvrIOConfigQ::addEvr() { _c.addEvr(); }

void EvrIOConfigQ::remEvr() { _c.remEvr(); }

#include "Parameters.icc"
