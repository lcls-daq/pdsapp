#include "Dialog.hh"
#include "Serializer.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QInputDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

#include <sys/stat.h>
#include <libgen.h>

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
  bool ok;
  QString file = QInputDialog::getText(this,"File to write to:","Filename:",
				       QLineEdit::Normal,"example.xtc",&ok);
  if (!ok)
    return;
  if (file.isEmpty())
    return;

  const int bufsize = 0x1000;
  char* buff = new char[bufsize];

  { QString fullname = _write_dir + "/" + file;
    strcpy(buff,qPrintable(fullname));
    struct stat s;
    if (!stat(buff,&s)) {
      QMessageBox::warning(this, "Save error", "Chosen filename already exists");
      delete[] buff;
      return;
    }
  }

  strcpy(buff,qPrintable(file));
  _file=QString(basename(buff));
  int siz = _s.writeParameters(buff);
  FILE* output = fopen(qPrintable(file),"w");
  fwrite(buff, siz, 1, output);
  fclose(output);
  delete[] buff;
  accept();
}

