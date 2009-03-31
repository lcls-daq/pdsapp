#include "pdsapp/config/DetInfoDialog_Ui.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <QtGui/QComboBox>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGroupBox>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>


using namespace Pds_ConfigDb;

DetInfoDialog_Ui::DetInfoDialog_Ui(QWidget* parent,
				   const list<Pds::DetInfo>& slist) :
  QDialog(parent),
  _list(slist)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QPushButton* _addbutton;
  QPushButton* _rembutton;

  QHBoxLayout* hlayout = new QHBoxLayout;
  { QGroupBox* group1 = new QGroupBox("Detector Info", this);
    QVBoxLayout* layout1 = new QVBoxLayout(group1);
    QGridLayout* layout1a = new QGridLayout;
    { QLabel* dlabel = new QLabel("Detector", group1);
      dlabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
      layout1a->addWidget(dlabel,0,0,1,1);
      layout1a->addWidget(_detlist = new QComboBox(this),0,1,1,1);
      QLabel* ilabel = new QLabel("Id", group1);
      ilabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
      layout1a->addWidget(ilabel,0,2,1,1);
      layout1a->addWidget(_detedit = new QLineEdit(this),0,3,1,1); }
    { QLabel* dlabel = new QLabel("Device", group1);
      dlabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
      layout1a->addWidget(dlabel,1,0,1,1);
      layout1a->addWidget(_devlist = new QComboBox(this),1,1,1,1);
      QLabel* ilabel = new QLabel("Id", group1);
      ilabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
      layout1a->addWidget(ilabel,1,2,1,1);
      layout1a->addWidget(_devedit = new QLineEdit(this),1,3,1,1); }
    layout1->addLayout(layout1a);
    layout1->addWidget(_addbutton = new QPushButton("Add", group1));
    group1->setLayout(layout1);
    hlayout->addWidget(group1); }
  
  { QGroupBox* group2 = new QGroupBox("List", this);
    QVBoxLayout* layout2 = new QVBoxLayout(group2);
    layout2->addWidget(_srclist = new QListWidget(group2));
    layout2->addWidget(_rembutton = new QPushButton("Remove", group2));
    group2->setLayout(layout2);
    hlayout->addWidget(group2); }

  layout->addLayout(hlayout);
  
  QDialogButtonBox* buttons = new QDialogButtonBox(this);
  buttons->setOrientation(Qt::Horizontal);
  buttons->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);
  layout->addWidget(buttons);

  for(unsigned i=0; i<Pds::DetInfo::NumDetector; i++)
    _detlist->addItem(Pds::DetInfo::name(Pds::DetInfo::Detector(i)));

  for(unsigned i=0; i<Pds::DetInfo::NumDevice; i++)
    _devlist->addItem(Pds::DetInfo::name(Pds::DetInfo::Device(i)));

  _detedit->setValidator(new QIntValidator(0,0xff,_detedit));
  _devedit->setValidator(new QIntValidator(0,0xff,_devedit));

  _srclist->clear();
  const QString sep("/");
  for(list<Pds::DetInfo>::iterator iter = _list.begin(); iter != _list.end(); iter++) {
    Pds::DetInfo& info = *iter;
    QString item;
    item += Pds::DetInfo::name(info.detector());
    item += sep + QString::number(info.detId()) + sep;
    item += Pds::DetInfo::name(info.device());
    item += sep + QString::number(info.devId());
    _srclist->addItem(item);
  }

  connect(_addbutton, SIGNAL(clicked()), this, SLOT(add()));
  connect(_rembutton, SIGNAL(clicked()), this, SLOT(remove()));

  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
}

DetInfoDialog_Ui::~DetInfoDialog_Ui()
{
}

void DetInfoDialog_Ui::add()
{
  { Pds::DetInfo::Detector dettype = (Pds::DetInfo::Detector)_detlist->currentIndex();
    Pds::DetInfo::Device   devtype = (Pds::DetInfo::Device  )_devlist->currentIndex();
    bool detok, devok;
    int  detid = _detedit->text().toInt(&detok);
    int  devid = _devedit->text().toInt(&devok);
    Pds::DetInfo info(0, dettype, detid, devtype, devid);
    _list.push_back(info);
    _list.unique(); }
  
  _srclist->clear();
  const QString sep("/");
  for(list<Pds::DetInfo>::iterator iter = _list.begin(); iter != _list.end(); iter++) {
    Pds::DetInfo& info = *iter;
    QString item;
    item += Pds::DetInfo::name(info.detector());
    item += sep + QString::number(info.detId()) + sep;
    item += Pds::DetInfo::name(info.device());
    item += sep + QString::number(info.devId());
    _srclist->addItem(item);
  }
}

void DetInfoDialog_Ui::remove()
{
  int idx = _srclist->currentRow();
  _srclist->takeItem(idx);
  list<Pds::DetInfo>::iterator iter = _list.begin();
  while(idx--) iter++;
  _list.erase(iter);
}

