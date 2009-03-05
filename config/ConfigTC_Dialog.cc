#include "ConfigTC_Dialog.hh"
#include "ConfigTC_Serializer.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QPushButton>

#include <sys/stat.h>

using namespace ConfigGui;

Dialog::Dialog(QWidget* parent,
	       Serializer& s) :
  QDialog(parent),
  _s(s)
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  Pds::LinkedList<Parameter>& pList(s.pList);
  Parameter* p = pList.forward();
  while( p != pList.empty() ) {
    layout->addLayout(p->initialize(this));
    p = p->forward();
  }
  QHBoxLayout* blayout = new QHBoxLayout;
  QPushButton* bRead  = new QPushButton("Read",this);
  QPushButton* bWrite = new QPushButton("Write",this);
  QPushButton* bReturn = new QPushButton("Return",this);
  blayout->addWidget(bRead);
  blayout->addWidget(bWrite);
  blayout->addWidget(bReturn);
  connect(bRead  , SIGNAL(clicked()), this, SLOT(read()));
  connect(bWrite , SIGNAL(clicked()), this, SLOT(write()));
  connect(bReturn, SIGNAL(clicked()), this, SLOT(accept()));

  layout->addLayout(blayout);
  setLayout(layout);
}

Dialog::~Dialog() {}

void Dialog::read()
{
  QString file = QFileDialog::getOpenFileName(this,"File to read from:",
					      ".","*.xtc");
  if (file.isNull())
    return;
  struct stat file_stat;
  stat(qPrintable(file),&file_stat);
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
  QString file = QFileDialog::getSaveFileName(this,"File to write to:",
					      ".","*.xtc");
  if (file.isNull())
    return;
  char* buff = new char[0x1000];
  int siz = _s.writeParameters(buff);
  FILE* output = fopen(qPrintable(file),"w");
  fwrite(buff, siz, 1, output);
  fclose(output);
  delete[] buff;
}
