#include "pdsapp/config/ListUi.hh"

#include "pdsapp/config/DeviceEntry.hh"
#include "pdsapp/config/Dialog.hh"
#include "pdsapp/config/Parameters.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QListWidgetItem>

#include <sys/stat.h>
#include <glob.h>
#include <libgen.h>

#include <fstream>
using std::ifstream;

using namespace Pds_ConfigDb;

ListUi::ListUi(const Path& path) :
  QWidget(0),
  _path(path)
{
  QHBoxLayout* layout = new QHBoxLayout;
  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Key",this));
    layout1->addWidget(_keylist = new QListWidget(this));
    _keylist->setMinimumWidth(400);
    layout->addLayout(layout1); }
  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Devices",this));
    layout1->addWidget(_devlist = new QListWidget(this));
    layout->addLayout(layout1); }
  { QVBoxLayout* layout1 = new QVBoxLayout;
    layout1->addWidget(new QLabel("Configuration Type",this));
    layout1->addWidget(_xtclist = new QListWidget(this));
    layout->addLayout(layout1); }
  setLayout(layout);

  // populate key list
  string kname = "[0-9]*";
  string kpath = _path.key_path(kname);
  glob_t g;
  glob(kpath.c_str(),0,0,&g);
  for(unsigned k=0; k<g.gl_pathc; k++) {
    struct stat64 s;
    stat64(g.gl_pathv[k],&s);
    QString entry(basename(g.gl_pathv[k]));
    entry += "  [" + QString(ctime(&s.st_mtime)).remove('\n') + "]";

    string info_path(g.gl_pathv[k]);
    info_path += "/Info";
    ifstream fi(info_path.c_str());
    if (fi.good()) {
      char buff[16];
      fi.getline(buff,16);
      entry += "  [" + QString(buff) + "]";
    }
    *new QListWidgetItem(entry,_keylist);
  }
  globfree(&g);

  connect(_keylist, SIGNAL(itemSelectionChanged()), this, SLOT(update_device_list()));
  connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_xtc_list()));
  connect(_xtclist, SIGNAL(itemSelectionChanged()), this, SLOT(view_xtc()));
}

ListUi::~ListUi()
{
}

void ListUi::update_device_list()
{
  bool ok = disconnect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_xtc_list()));
  _devlist->clear();
  _devices .clear();
  QListWidgetItem* item = _keylist->currentItem();
  if (item) {
    string kname = string(qPrintable(item->text().split(' ')[0])) + "/[0-9]*";
    string kpath = _path.key_path(kname);
    glob_t g;
    glob(kpath.c_str(),0,0,&g);
    for(unsigned k=0; k<g.gl_pathc; k++) {
      const char* src = basename(g.gl_pathv[k]);
      unsigned phy = strtoul(src,NULL,16);
      if (strlen(src)==1) {
	*new QListWidgetItem(Pds::Level::name((Pds::Level::Type)phy),_devlist);
      }
      else if (strlen(src)==8) {
	DeviceEntry entry(phy);
	*new QListWidgetItem(Pds::DetInfo::name(reinterpret_cast<const Pds::DetInfo&>(entry)),_devlist);
      }
      else {
	printf("config source %s does not compute.  Skipping.\n",src);
	continue;
      }
      _devices.push_back(string(g.gl_pathv[k]));
    }
    globfree(&g);
  }
  if (ok) connect(_devlist, SIGNAL(itemSelectionChanged()), this, SLOT(update_xtc_list()));
  update_xtc_list();
}

void ListUi::update_xtc_list()
{
  bool ok = disconnect(_xtclist, SIGNAL(itemSelectionChanged()), this, SLOT(view_xtc()));
  _xtclist->clear();
  _types   .clear();
  QListWidgetItem* item = _devlist->currentItem();
  if (item) {
    string kpath = _devices[_devlist->currentRow()] + "/*";
    glob_t g;
    glob(kpath.c_str(),0,0,&g);
    for(unsigned k=0; k<g.gl_pathc; k++) {
      unsigned typ = strtoul(basename(g.gl_pathv[k]),NULL,16);
      *new QListWidgetItem(Pds::TypeId::name(reinterpret_cast<const Pds::TypeId&>(typ).id()),_xtclist);
      _types.push_back(string(g.gl_pathv[k]));
    }
    globfree(&g);
  }
  if (ok) connect(_xtclist, SIGNAL(itemSelectionChanged()), this, SLOT(view_xtc()));
}

void ListUi::view_xtc()
{
  char path[128];
  strcpy(path,_types[_xtclist->currentRow()].c_str());
  QString qpath(dirname(path));

  strcpy(path,_types[_xtclist->currentRow()].c_str());
  unsigned typ = strtoul(basename(path),NULL,16);
  const Pds::TypeId& t = reinterpret_cast<Pds::TypeId&>(typ);

  QString qfile(_types[_xtclist->currentRow()].c_str());

  Parameter::allowEdit(false);
  Dialog* d = new Dialog(_xtclist, *_dict.lookup(t), qpath, qpath, qfile, false);
  //  d->exec();
  //  delete d;
  d->setAttribute(::Qt::WA_DeleteOnClose, true);
  d->show();
}
