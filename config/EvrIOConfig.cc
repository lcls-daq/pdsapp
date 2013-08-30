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
  public:
    void insert(Pds::LinkedList<Parameter>& pList)
    {
      pList.insert(this);
      
      for(int i=0; i<MaxOutputs; i++)
  _channel[i]->insert(_pList);
    }

    int pull(const EvrIOConfigType& tc) {
      _conn.value = tc.conn();
      for(unsigned i=0; i<tc.nchannels(); i++)
        _channel[i]->pull(tc.channels()[i]);
      return tc._sizeof();
    }

    int push(void* to) const {
      EvrIOConfigType& tc = *reinterpret_cast<EvrIOConfigType*>(to);
      *new(&tc) EvrIOConfigType(_conn.value, MaxOutputs);

      Pds::EvrData::IOChannel* ch = reinterpret_cast<Pds::EvrData::IOChannel*>(&tc+1);
      for(int i=0; i<MaxOutputs; i++)
        _channel[i]->push(ch[i]);
        
      return tc._sizeof();
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
    unsigned                                  _id;
    Enumerated<Pds::EvrData::OutputMap::Conn> _conn;
    EvrIOChannel*                             _channel[MaxOutputs];
    Pds::LinkedList<Parameter>                _pList;
  };


  class EvrIOConfig::Private_Data : public Parameter {
  public:
    Private_Data() : Parameter(NULL),
                     _nevr(0)
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
      const char* p = reinterpret_cast<const char*>(from);
      unsigned ievr = 0;
      do {
        const EvrIOConfigType& tc = *reinterpret_cast<const EvrIOConfigType*>(p);
        p += tc._sizeof();
        printf("ievr[%d]  nch %d\n",ievr,tc.nchannels());
        if (tc.nchannels()==0) break;
        _evr[ievr++]->pull(tc);
      } while(1);

      for(unsigned i=ievr; i<_nevr; i++)
        _tab->setTabEnabled(i,false);

      for(unsigned i=_nevr; i<ievr; i++)
        _tab->setTabEnabled(i,true);

      _nevr = ievr;

      return p - reinterpret_cast<const char*>(from);
    }

    int push(void* to) const {
      char* p = reinterpret_cast<char*>(to);
      for(unsigned i=0; i<_nevr; i++) {
        p += _evr[i]->push(p);
      }
      p += (new(p) EvrIOConfigType(Pds::EvrData::OutputMap::UnivIO, 0))->_sizeof();
      printf("EvrIOConfig push %d bytes\n", (int)(p - reinterpret_cast<char*>(to)));
      return p - reinterpret_cast<char*>(to);
    }

    int dataSize() const {
      unsigned size = 0;
      for(unsigned i=0; i<_nevr; i++)
        size += _evr[i]->dataSize();
      size += EvrIOConfigType(Pds::EvrData::OutputMap::UnivIO,0)._sizeof();
      return size;
    }

  public:
    QLayout* initialize(QWidget* parent) 
    {
      _nevr  = 0;
      _qlink = new EvrIOConfigQ(*this, parent);

      QVBoxLayout* l = new QVBoxLayout;
      l->addStretch();
      { QHBoxLayout* h = new QHBoxLayout;
        QPushButton* addB = new QPushButton("Add EVR");
        QObject::connect(addB, SIGNAL(pressed()), _qlink, SLOT(addEvr()));
        QPushButton* remB = new QPushButton("Remove EVR");
        QObject::connect(remB, SIGNAL(pressed()), _qlink, SLOT(remEvr()));
        h->addStretch();
        h->addWidget(addB);
        h->addStretch();
        h->addWidget(remB);
        h->addStretch();
        l->addLayout(h); }
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
      if (_nevr < MaxEVRs) {
        _tab->setTabEnabled(_nevr,true);
        _nevr++;
      }
    }

    void remEvr() {
      if (_nevr > 1) {
        _nevr--;
        if (_tab->currentIndex() == (int) _nevr)
          _tab->setCurrentIndex(_nevr-1);
        _tab->setTabEnabled(_nevr,false);
      }
    }

  public:
    EvrIOConfig::Panel*                       _evr[MaxEVRs];
    unsigned                                  _nevr;
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
