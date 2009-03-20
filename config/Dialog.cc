#include "Dialog.hh"
#include "Serializer.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QPushButton>

#include <sys/stat.h>

using namespace Pds_ConfigDb;

Dialog::Dialog(QWidget* parent,
	       Serializer& s,
	       const QString& read_dir,
	       const QString& write_dir) :
  QDialog(parent),
  _s(s),
  _read_dir(read_dir),
  _write_dir(write_dir)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  Pds::LinkedList<Parameter>& pList(s.pList);
  Parameter* p = pList.forward();
  while( p != pList.empty() ) {
    layout->addLayout(p->initialize(this));
    p = p->forward();
  }
  QHBoxLayout* blayout = new QHBoxLayout;
  QPushButton* bRead  = new QPushButton("Copy",this);
  QPushButton* bWrite = new QPushButton("Save",this);
  QPushButton* bReturn = new QPushButton("Cancel",this);
  blayout->addWidget(bRead);
  blayout->addWidget(bWrite);
  blayout->addWidget(bReturn);
  connect(bRead  , SIGNAL(clicked()), this, SLOT(read()));
  connect(bWrite , SIGNAL(clicked()), this, SLOT(write()));
  connect(bReturn, SIGNAL(clicked()), this, SLOT(reject()));

  layout->addLayout(blayout);
  setLayout(layout);
}

Dialog::~Dialog() {}

void Dialog::read()
{
  QString file = QFileDialog::getOpenFileName(this,"File to read from:",
					      _read_dir, "*.xtc");
  if (file.isNull())
    return;
  struct stat file_stat;
  if (stat(qPrintable(file),&file_stat)) return;
  _file = file;
  char* buff = new char[file_stat.st_size];
  FILE* input = fopen(qPrintable(file),"r");
  fread(buff, file_stat.st_size, 1, input);
  fclose(input);
  _s.readParameters(buff);
  delete[] buff;

  Pds::LinkedList<Parameter>& pList(_s.pList);
  Parameter* p = pList.forward();
  while( p != pList.empty() ) {
    p->flush();
    p = p->forward();
  }
}

void Dialog::write()
{
  Pds::LinkedList<Parameter>& pList(_s.pList);
  Parameter* p = pList.forward();
  while( p != pList.empty() ) {
    p->update();
    p = p->forward();
  }
//   QString file = QFileDialog::getSaveFileName(this,"File to write to:",
// 					      _write_dir,"*.xtc");
  QFileDialog  d(this,"File name:");
  d.setFilter("*.xtc");
  QDir directory(_write_dir);
  directory.setFilter( QDir::Files );
  d.setDirectory(directory);
  d.setAcceptMode(QFileDialog::AcceptSave);
  if (d.exec()) {
    QString file = d.selectedFiles().at(0);
    if (file.isNull())
      return;
    const int bufsize = 0x1000;
    char* buff = new char[bufsize];
    strcpy(buff,qPrintable(file));
    _file=QString(basename(buff));
    int siz = _s.writeParameters(buff);
    FILE* output = fopen(qPrintable(file),"w");
    fwrite(buff, siz, 1, output);
    fclose(output);
    delete[] buff;
    accept();
  }
}

