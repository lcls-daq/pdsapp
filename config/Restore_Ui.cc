#include "pdsapp/config/Restore_Ui.hh"

#include "pdsapp/config/Device.hh"
#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/GlobalCfg.hh"
#include "pdsapp/config/Parameters.hh"

#include "pds/config/DbClient.hh"
#include "pds/config/PdsDefs.hh"
#include "pds/config/DeviceEntry.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>

#include <fstream>
using std::ifstream;

using namespace Pds_ConfigDb;

static bool comp(const XtcEntryT& t1,
		 const XtcEntryT& t2)
{
  if (t1.time == t2.time) {
    if (t1.xtc.type_id.value() == t2.xtc.type_id.value())
      return t1.xtc.name < t2.xtc.name;
    return t1.xtc.type_id.value() < t2.xtc.type_id.value();
  }
  return t1.time < t2.time;
}

static bool eqcomp(const XtcEntryT& t1,
		 const XtcEntryT& t2)
{
  return (t1.time == t2.time) &&
    (t1.xtc.type_id.value() == t2.xtc.type_id.value()) &&
    (t1.xtc.name == t2.xtc.name);
}

Restore_Ui::Restore_Ui(QWidget* parent,
		       DbClient& db,
		       const Device& device,
		       Pds::TypeId type_id,
		       const std::string& name) :
  QDialog (parent),
  _db     (db),
  _name   (name),
  _lenbuff(0x100000),
  _xtcbuff(new char[_lenbuff])
{
  QHBoxLayout* layout = new QHBoxLayout;
  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Date",this));
    layout1->addWidget(_xtclist = new QListWidget(this));
    _xtclist->setMinimumWidth(400);
    layout->addLayout(layout1); }
  setLayout(layout);

  for(std::list<DeviceEntry>::const_iterator it=device.src_list().begin();
      it!=device.src_list().end(); it++) {
    std::list<XtcEntryT> x = db.getXTC(it->value(),type_id.value());
    _xtc.merge(x,comp);
  }

  _xtc.unique(eqcomp);

  for(std::list<XtcEntryT>::reverse_iterator it=_xtc.rbegin();
      it!=_xtc.rend(); it++) {
    QString entry = QString("%1").arg(QString(it->stime.c_str()));
    *new QListWidgetItem(entry,_xtclist);
  }

  connect(_xtclist, SIGNAL(itemSelectionChanged()), this, SLOT(view_xtc()));
}

Restore_Ui::~Restore_Ui()
{
  delete[] _xtcbuff;
}

void Restore_Ui::view_xtc()
{
  int i = _xtclist->currentRow();
  std::list<XtcEntryT>::reverse_iterator it=_xtc.rbegin();
  while(i--)
    it++;

  XtcEntryT t(*it);

  int len=_db.getXTC(t);
  if (len > int(_lenbuff)) {
    delete[] _xtcbuff;
    _xtcbuff = new char[_lenbuff = len];
  }

  int sz = _db.getXTC(t,_xtcbuff,_lenbuff);
  if (sz<0 || sz!=len) {
    printf("Error fetching [%d/%d bytes]\n",sz,len);
    return;
  }

  Parameter::allowEdit(true);
  QString qchoice(_name.c_str());
  Dialog* d = new Dialog(_xtclist, 
                         *_dict.lookup(t.xtc.type_id),
                         qchoice, _xtcbuff, len, true);
  if (d->exec()==QDialog::Accepted) {
    t.xtc.name = qPrintable(d->name());
    _db.begin();
    _db.setXTC(t.xtc, d->payload(), d->payload_size());
    _db.commit();
    
    if (GlobalCfg::instance().contains(t.xtc.type_id)) {
      char* b = new char[d->payload_size()];
      memcpy(b, d->payload(), d->payload_size());
      GlobalCfg::instance().cache(t.xtc.type_id,b);
    }
    delete d;
    accept();
  }
  else {
    delete d;
  }
}
