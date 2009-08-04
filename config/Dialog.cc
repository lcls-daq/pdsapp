#include "Dialog.hh"
#include "Serializer.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QInputDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>

#include <sys/stat.h>
#include <libgen.h>

namespace Pds_ConfigDb {

  class Cycle {
  public:
    Cycle(int sz) : 
      buffer(new char[sz]),
      size  (sz)
    {}
    ~Cycle()
    { delete[] buffer; }
  public:
    char* buffer;
    int   size;
  };
};

using namespace Pds_ConfigDb;

static const QString nopath(".");

Dialog::Dialog(QWidget* parent,
	       Serializer& s,
	       const QString& file) :
  QDialog(parent),
  _s(s),
  _read_dir(nopath),
  _write_dir(nopath)
{
  layout();

  append(file);
}

Dialog::Dialog(QWidget* parent,
	       Serializer& s,
	       const QString& read_dir,
	       const QString& write_dir) :
  QDialog(parent),
  _s(s),
  _read_dir(read_dir),
  _write_dir(write_dir)
{
  layout();

  // Create the new cycle
  Cycle* cycle = new Cycle(_s.dataSize());
  _s.writeParameters(cycle->buffer);
  _cycles.insert(_cycles.begin(),cycle);

  _cycleBox->addItem(QString("%1").arg(_cycles.size()-1));
  _cycleBox->setCurrentIndex(_current = 0);
}

Dialog::~Dialog() 
{
  for(unsigned i=0; i<_cycles.size(); i++)
    delete _cycles[i];
}

void Dialog::layout()
{
  QVBoxLayout* layout = new QVBoxLayout(this);
  QPushButton* bInsertCycle = new QPushButton("Insert",this);
  QPushButton* bRemoveCycle = new QPushButton("Remove",this);
  _cycleBox                 = new QComboBox  (this);

  { QGroupBox* calibGroup = new QGroupBox("Calibration Cycle",this);
    QHBoxLayout* layout1 = new QHBoxLayout;
    layout1->addWidget(bInsertCycle);
    layout1->addWidget(bRemoveCycle);
    layout1->addWidget(_cycleBox);
    calibGroup->setLayout(layout1);
    layout->addWidget(calibGroup); }

  if (Parameter::allowEdit()) {
    connect(bInsertCycle, SIGNAL(clicked()), this, SLOT(insert_cycle()));
    connect(bRemoveCycle, SIGNAL(clicked()), this, SLOT(remove_cycle()));
  }
  else {
    bInsertCycle->setEnabled(false);
    bRemoveCycle->setEnabled(false);
  }
  connect(_cycleBox, SIGNAL(activated(int)), this, SLOT(set_cycle(int)));

  _s.initialize(this, layout);

  QHBoxLayout* blayout = new QHBoxLayout;
  QPushButton* bReplace = new QPushButton("Read\nReplace",this);
  QPushButton* bAppend  = new QPushButton("Read\nAppend",this);
  QPushButton* bWrite = new QPushButton("Save",this);
  QPushButton* bReturn = new QPushButton("Cancel",this);
  blayout->addWidget(bReplace);
  blayout->addWidget(bAppend);
  blayout->addWidget(bWrite);
  blayout->addWidget(bReturn);
  if (Parameter::allowEdit()) {
    bReplace->setEnabled(true);
    bAppend ->setEnabled(true);
    bWrite  ->setEnabled(true);
    connect(bReplace, SIGNAL(clicked()), this, SLOT(replace ()));
    connect(bAppend , SIGNAL(clicked()), this, SLOT(append  ()));
    connect(bWrite  , SIGNAL(clicked()), this, SLOT(write   ()));
  }
  else {
    bReplace->setEnabled(false);
    bAppend ->setEnabled(false);
    bWrite  ->setEnabled(false);
  }
  connect(bReturn, SIGNAL(clicked()), this, SLOT(reject()));

  layout->addLayout(blayout);
  setLayout(layout);
}

void Dialog::replace()
{
  QString file = QFileDialog::getOpenFileName(this,"File to read from:",
					      _read_dir, "*.xtc");
  if (file.isNull())
    return;

  for(unsigned i=0; i<_cycles.size(); i++)
    delete _cycles[i];
  _cycles.clear();
  _cycleBox->clear();

  append(file);
}

void Dialog::append()
{
  QString file = QFileDialog::getOpenFileName(this,"File to read from:",
					      _read_dir, "*.xtc");
  if (file.isNull())
    return;

  append(file);
}

void Dialog::write()
{
  // Save the current cycle
  _s.update();
  int i = _cycleBox->currentIndex();
  delete _cycles[i];
  Cycle* cycle = new Cycle(_s.dataSize());
  _s.writeParameters(cycle->buffer);
  _cycles[i] = cycle;

  bool ok;
  QString file = QInputDialog::getText(this,"File to write to:","Filename:",
				       QLineEdit::Normal,"example.xtc",&ok);
  if (!ok)
    return;
  if (file.isEmpty())
    return;

  const int bufsize = 128;
  char* buff = new char[bufsize];

  QString fullname = _write_dir + "/" + file;
  strcpy(buff,qPrintable(fullname));
  struct stat s;
  if (!stat(buff,&s)) {
    QMessageBox::warning(this, "Save error", "Chosen filename already exists");
    delete[] buff;
    return;
  }

  _file=file;
  FILE* output = fopen(buff,"w");
  for(unsigned k=0; k<_cycles.size(); k++)
    fwrite(_cycles[k]->buffer, _cycles[k]->size, 1, output);
  fclose(output);
  delete[] buff;
  accept();
}

void Dialog::insert_cycle()
{
  // Save the current cycle
  _s.update();
  int i = _cycleBox->currentIndex();
  delete _cycles[i];
  Cycle* cycle = new Cycle(_s.dataSize());
  _s.writeParameters(cycle->buffer);
  _cycles[i] = cycle;

  // Create the new cycle
  cycle = new Cycle(_s.dataSize());
  _s.writeParameters(cycle->buffer);
  _cycles.insert(_cycles.begin()+i+1,cycle);

  _cycleBox->addItem(QString("%1").arg(_cycles.size()-1));
  _cycleBox->setCurrentIndex(_current = i+1);
}

void Dialog::remove_cycle()
{
  unsigned i = _cycleBox->currentIndex();
  delete _cycles[i];
  _cycles.erase(_cycles.begin()+i);

  _cycleBox->removeItem(_cycles.size());

  _cycleBox->setCurrentIndex(_current = i<_cycles.size() ? i : _cycles.size()-1);
  _s.readParameters(_cycles[_current]->buffer);
  _s.flush();
}

void Dialog::set_cycle(int index)
{
  if (Parameter::allowEdit()) {
    // Save the current cycle
    _s.update();
    int i = _current;
    delete _cycles[i];
    Cycle* cycle = new Cycle(_s.dataSize());
    _s.writeParameters(cycle->buffer);
    _cycles[i] = cycle;
  }

  _s.readParameters(_cycles[_current=index]->buffer);
  _s.flush();
}


void Dialog::append(const QString& file)
{
  // perform the read
  struct stat file_stat;
  if (stat(qPrintable(file),&file_stat)) return;
  char* buff = new char[file_stat.st_size];
  FILE* input = fopen(qPrintable(file),"r");
  fread(buff, file_stat.st_size, 1, input);
  fclose(input);

  char* b = buff;
  char* e = buff + file_stat.st_size;
  while(b < e) {
    Cycle* cycle = new Cycle(_s.readParameters(b));
    b += cycle->size;
    _s.writeParameters(cycle->buffer);
    _cycles.push_back(cycle);
    _cycleBox->addItem(QString("%1").arg(_cycles.size()-1));
  }
  _cycleBox->setCurrentIndex(_current = 0);
  _s.readParameters(_cycles[_current]->buffer);
  _s.flush();

  delete[] buff;
}
