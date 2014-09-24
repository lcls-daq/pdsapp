#include "pdsapp/config/EvrIOChannel.hh"

#include "pds/config/EvrIOConfigType.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <QtGui/QGridLayout>
#include <QtGui/QComboBox>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>

static const char* detTypes[Pds::DetInfo::NumDetector+1];
static const char* devTypes[Pds::DetInfo::NumDevice  +1];

using namespace Pds_ConfigDb;

void EvrIOChannel::initialize()
{
  static bool _initialized = false;
  if (!_initialized) {
    _initialized = true;
    for(unsigned i=0; i<Pds::DetInfo::NumDetector; i++)
      detTypes[i] = Pds::DetInfo::name(Pds::DetInfo::Detector(i));
    detTypes[Pds::DetInfo::NumDetector] = NULL;
    for(unsigned i=0; i<Pds::DetInfo::NumDevice  ; i++)
      devTypes[i] = Pds::DetInfo::name(Pds::DetInfo::Device  (i));
    devTypes[Pds::DetInfo::NumDevice  ] = NULL;
  }
}

EvrIOChannel::EvrIOChannel(unsigned i) :
  _id     (i),
  _label  (NULL, "", Pds::EvrData::IOChannel::NameLength),
  _ninfo  (0),
  _dettype(NULL, Pds::DetInfo::NoDetector, detTypes),
  _detid  (NULL,0,0,0x7fffffff,Decimal),
  _devtype(NULL, Pds::DetInfo::NoDevice  , devTypes),
  _devid  (NULL,0,0,0x7fffffff,Decimal)
{
}

void EvrIOChannel::layout(QGridLayout* l, unsigned row) {
  int column = 0;
  _channel  = new QLabel(QString::number(_id));

  _detnames = new QComboBox;
  for(unsigned i=0; i<_ninfo; i++)
    _detnames->addItem(Pds::DetInfo::name(_detinfo[i]));

  if (_label.allowEdit()) {
    _add      = new QPushButton("Add");
    _remove   = new QPushButton("Remove");

    l->addWidget(_channel              , row, column++, Qt::AlignCenter);
    l->addLayout(_label  .initialize(0), row, column++, Qt::AlignCenter);
    l->addWidget(_detnames             , row, column++, Qt::AlignCenter);
    
    l->addWidget(_remove               , row, column++, Qt::AlignCenter);
    l->addWidget(new QLabel("  ")      , row, column++, Qt::AlignCenter);
    l->addLayout(_dettype.initialize(0), row, column++, Qt::AlignCenter);
    l->addLayout(_detid  .initialize(0), row, column++, Qt::AlignCenter);  _detid.widget()->setMaximumWidth(60);
    l->addLayout(_devtype.initialize(0), row, column++, Qt::AlignCenter);
    l->addLayout(_devid  .initialize(0), row, column++, Qt::AlignCenter);  _devid.widget()->setMaximumWidth(60);
    l->addWidget(_add                  , row, column++, Qt::AlignCenter);
    
    connect(_add   , SIGNAL(clicked()), this, SLOT(add_info()));
    connect(_remove, SIGNAL(clicked()), this, SLOT(remove_info()));
  }
  else {
    l->addWidget(_channel              , row, column++, Qt::AlignCenter);
    l->addLayout(_label  .initialize(0), row, column++, Qt::AlignCenter);
    l->addWidget(_detnames             , row, column++, Qt::AlignCenter);
  }
}

void EvrIOChannel::add_info() {
  if (_ninfo < MaxInfos-1) {
    _dettype.update();
    _detid  .update();
    _devtype.update();
    _detid  .update();
    _detinfo[_ninfo] = Pds::DetInfo(-1,
				    _dettype.value, _detid.value,
				    _devtype.value, _devid.value);
    _detnames->addItem(Pds::DetInfo::name(_detinfo[_ninfo]));
    _ninfo++;
  }
}

void EvrIOChannel::remove_info() {
  int i = _detnames->currentIndex();
  if (i >= 0) {
    _detnames->removeItem(i);
    while(i < int(_ninfo)) {
      _detinfo[i] = _detinfo[i+1];
      i++;
    }
    _ninfo--;
  }
}

void EvrIOChannel::insert(Pds::LinkedList<Parameter>& pList) {
  pList.insert(&_label);
  if (_label.allowEdit()) {
    pList.insert(&_dettype);
    pList.insert(&_detid);
    pList.insert(&_devtype);
    pList.insert(&_devid);
  }
}
void EvrIOChannel::pull(const Pds::EvrData::IOChannel& c) {
  strncpy(_label.value, c.name(), Pds::EvrData::IOChannel::NameLength);
  _label.value[Pds::EvrData::IOChannel::NameLength] = 0;
  _ninfo = c.ninfo();
  _detnames->clear();
  for(unsigned i=0; i<c.ninfo(); i++) {
    _detnames->addItem(Pds::DetInfo::name(c.infos()[i]));
    _detinfo[i] = c.infos()[i];
  }
}
void EvrIOChannel::push(Pds::EvrData::IOChannel& c) const {
  *new(&c) Pds::EvrData::IOChannel(_label.value, _ninfo, _detinfo);
}

#include "Parameters.icc"
