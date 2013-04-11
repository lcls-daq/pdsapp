#include "pdsapp/config/XtcFileServer.hh"

#include <QtGui/QFileDialog>
#include <QtGui/QPlastiqueStyle>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>

#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cassert>
#include <sys/stat.h>

using namespace Pds_ConfigDb;

static QString itoa(int i) { return QString::number(i); }

static void setStatus(const QString status) {
  cout << qPrintable(status) << endl;
}

void XtcFileServer::updateDirLabel() {
  _dirLabel->setText("Folder: " + _curdir);
}

void XtcFileServer::updateRunCombo() {
  _runCombo->clear();
  _runCombo->addItems(_runList);
}

void XtcFileServer::getPathsForRun(QStringList& list, QString run) {
  if (_curdir.isEmpty() || run.isEmpty()) {
    setStatus("getPathsForRun: no paths to add for directory " + _curdir + " run " + run);
    return;
  }

  QString pattern = _curdir + "/xtc/*-r" + run + "-s*.xtc";
  glob_t g;
  glob(qPrintable(pattern), 0, 0, &g);

  for (unsigned i = 0; i < g.gl_pathc; i++) {
    char *path = g.gl_pathv[i];
    struct ::stat64 st;
    if (::stat64(path, &st) == -1) {
      perror(path);
      continue;
    }
    if (st.st_size == 0) {
      setStatus(QString("Ignoring empty file ") + path);
      continue;
    }
    list << path;
  }
  globfree(&g);
  list.sort();
}

static void _connect(const QObject* sender, const char* signal, const QObject* receiver, const char* method, ::Qt::ConnectionType type = ::Qt::AutoConnection) {
  if (! QObject::connect(sender, signal, receiver, method, type)) {
    cerr << "connect(" << sender << ", " << signal << ", " << receiver << ", " << method << ", " << type << ") failed" << endl;
    exit(1);
  }
}

XtcFileServer::XtcFileServer(const char* curdir) :
  QGroupBox (0),
  _curdir(curdir),
  _dirSelect(new QPushButton("Change")),
  _dirLabel(new QLabel),
  _runCombo(new QComboBox)
{
  QVBoxLayout* l = new QVBoxLayout;

  QHBoxLayout* hboxA = new QHBoxLayout;
  hboxA->addWidget(_dirLabel);
  hboxA->addWidget(_dirSelect);
  l->addLayout(hboxA);
  updateDirLabel();

  // This forces combobox to use scrollbar, which is essential
  // when it contains a very large number of items.
  _runCombo->setStyle(new QPlastiqueStyle());

  QHBoxLayout* hbox1 = new QHBoxLayout;
  hbox1->addStretch();
  hbox1->addWidget(new QLabel("Run:"));
  hbox1->addStretch();
  hbox1->addWidget(_runCombo);
  hbox1->addStretch();
  l->addLayout(hbox1);

  setLayout(l);
  show();

  _connect(_dirSelect, SIGNAL(clicked()), this, SLOT(selectDir()));
  _connect(_runCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(selectRun(int)));
  _connect(this, SIGNAL(_updateDirLabel()), this, SLOT(updateDirLabel()));
  _connect(this, SIGNAL(_updateRunCombo()), this, SLOT(updateRunCombo()));

  setDir(QString(_curdir));
  //  configure_run();
}

XtcFileServer::~XtcFileServer()
{
}

void XtcFileServer::selectDir()
{
  setStatus("Selecting directory...");
  setDir(QFileDialog::getExistingDirectory(0, "Select Directory", _curdir, 0));
}

void XtcFileServer::setDir(QString dir)
{
  if (dir == "") {
    return;
  }
  while (dir.endsWith("/")) {
    dir.chop(1);
  }
  if (dir.endsWith("/xtc")) {
    dir.chop(4);
  }
  _curdir.clear();
  _curdir.append(dir);
  emit _updateDirLabel();


  // Collect all the runs in this dir.
  _runList.clear();
  glob_t g;
  QString gpath = _curdir + "/xtc/*-r*-s*.xtc";
  setStatus("Looking for runs under " + gpath + "...");
  glob(qPrintable(gpath), 0, 0, &g);
  for (unsigned i = 0; i < g.gl_pathc; i++) {
    char *path = g.gl_pathv[i];
    QString s(basename(path));
    int p = s.indexOf("-r");
    if (p >= 0) {
      QString run = s.mid(p+2, s.indexOf("-s")-p-2);
      if (! _runList.contains(run)) {
        _runList << run;
      }
    }
  }
  globfree(&g);
  _runList.sort();
  setStatus("Found " + itoa(_runList.size()) + " runs under " + _curdir);
  emit _updateRunCombo();
}

void XtcFileServer::selectRun(int index)
{
  QString runName = _runCombo->currentText();

  QStringList files;
  setStatus("Fetching files for run " + runName + "...");
  getPathsForRun(files, runName);
  if (files.empty()) {
    setStatus("Found no files paths for run " + runName);
    return;
  }
  setStatus("Fetched " + itoa(files.size()) + " paths for run " + runName);

  emit file_selected(files.first());
}
