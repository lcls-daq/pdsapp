#include "Dialog.hh"
#include "Serializer.hh"

#include <QtGui/QVBoxLayout>
#include <QtGui/QFileDialog>
#include <QtGui/QInputDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QKeyEvent>

#include <sys/stat.h>
#include <libgen.h>
#include <errno.h>

//#define EDIT_CYCLES

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

  class KeyPressEater : public QObject {
    //    Q_OBJECT
  protected:
    bool eventFilter(QObject *obj, QEvent *event);
  };
  
  bool KeyPressEater::eventFilter(QObject *obj, QEvent *event)
  {
    if (event->type() == QEvent::KeyPress) {
      QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
      if (keyEvent->key() == ::Qt::Key_Return ||
	  keyEvent->key() == ::Qt::Key_Enter)
	return true;
    }
    // standard event processing
    return QObject::eventFilter(obj, event);
  }
};

using namespace Pds_ConfigDb;

static const QString nopath(".");
static const QString nochoice;
static KeyPressEater keyPressEater;

Dialog::Dialog(QWidget* parent,
               Serializer& s,
               const void* p,
               unsigned    sz) :
  QDialog(parent),
  _s(s),
  _read_dir (nochoice),
  _write_dir(nochoice)
{
  layout();

  append(p,sz);
}

Dialog::Dialog(QWidget* parent,
         Serializer& s,
         const QString& file) :
  QDialog(parent),
  _s(s),
  _read_dir (nochoice),
  _write_dir(nochoice),
  _file     (file)
{
  layout();

  append(file);
}

Dialog::Dialog(QWidget* parent,
         Serializer& s,
         const QString& read_dir,
         const QString& write_dir,
         const QString& file) :
  QDialog(parent),
  _s(s),
  _read_dir (read_dir),
  _write_dir(write_dir),
  _file     (file)
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
  _read_dir (read_dir),
  _write_dir(write_dir),
  _file     ("example.xtc")
{
  layout();

  // Create the new cycle
  Cycle* cycle = new Cycle(_s.dataSize());
  _s.writeParameters(cycle->buffer);
  _cycles.insert(_cycles.begin(),cycle);

  _current = 0;
#ifdef EDIT_CYCLES
  _cycleBox->addItem(QString("%1").arg(_cycles.size()-1));
  _cycleBox->setCurrentIndex(_current);
#endif
}

Dialog::~Dialog() 
{
  for(unsigned i=0; i<_cycles.size(); i++)
    delete _cycles[i];
}

void Dialog::layout()
{
  QVBoxLayout* layout = new QVBoxLayout(this);
#ifdef EDIT_CYCLES
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
#endif

  _s.initialize(this, layout);

  QHBoxLayout* blayout = new QHBoxLayout;
#ifdef EDIT_CYCLES
  QPushButton* bReplace = new QPushButton("Read\nReplace",this);
  QPushButton* bAppend  = new QPushButton("Read\nAppend",this);
  blayout->addWidget(bReplace);
  blayout->addWidget(bAppend);
  if (Parameter::allowEdit()) {
    bReplace->setEnabled(true);
    bAppend ->setEnabled(true); 
    connect(bReplace, SIGNAL(clicked()), this, SLOT(replace ()));
    connect(bAppend , SIGNAL(clicked()), this, SLOT(append  ()));
  }
  else {
    bReplace->setEnabled(false);
    bAppend ->setEnabled(false);
  }
#endif
  QPushButton* bWrite = new QPushButton("Save",this);
  QPushButton* bReturn = new QPushButton("Cancel",this);
  blayout->addWidget(bWrite);
  blayout->addWidget(bReturn);
  if (Parameter::allowEdit()) {
    bWrite  ->setEnabled(true);
    connect(bWrite  , SIGNAL(clicked()), this, SLOT(write   ()));
  }
  else {
    bWrite  ->setEnabled(false);
  }
  connect(bReturn, SIGNAL(clicked()), this, SLOT(reject()));

  layout->addLayout(blayout);
  setLayout(layout);

  installEventFilter(&keyPressEater);
}

void Dialog::replace()
{
  QString file = QFileDialog::getOpenFileName(this,"File to read from:",
                _read_dir, "(current) *.xtc;; (all) *");
  if (file.isNull())
    return;

  for(unsigned i=0; i<_cycles.size(); i++)
    delete _cycles[i];
  _cycles.clear();
#ifdef EDIT_CYCLES
  _cycleBox->clear();
#endif
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

  // Validate
  if (!_s.validate())
    return;

#ifdef EDIT_CYCLES
  int i = _cycleBox->currentIndex();
#else
  int i = 0;
#endif
  Cycle* cycle = new Cycle(_s.dataSize());
  _s.writeParameters(cycle->buffer);
  if (_cycles.size()) {
    delete _cycles[i];
    _cycles[i] = cycle;
  }
  else
    _cycles.insert(_cycles.begin(),cycle);

  const int bufsize = 128;
  char* buff = new char[bufsize];

  if (!_write_dir.isNull()) {
    QString filet = _file.mid(_file.lastIndexOf("/")+1);

    bool ok;
    QString file = QInputDialog::getText(this,"File to write to:","Filename:",
           QLineEdit::Normal,filet,&ok);
    if (!ok)
      return;
    if (file.isEmpty())
      return;

    _file=file;
    QString fullname = _write_dir + "/" + file;
    strcpy(buff,qPrintable(fullname));
  }
  else {
    strcpy(buff,qPrintable(_file));
  }

  FILE* output = fopen(buff,"w");
  if (output) {
    for(unsigned k=0; k<_cycles.size(); k++)
      fwrite(_cycles[k]->buffer, _cycles[k]->size, 1, output);
    fclose(output);
  }
  else {
    char msg[256];
    sprintf(msg,"Error opening %s : %s",buff,strerror(errno));
    printf("%s\n",msg);
    QMessageBox::critical(this,"Save Error",msg);
  }
  delete[] buff;
  accept();
}

void Dialog::insert_cycle()
{
  // Save the current cycle
  _s.update();
#ifdef EDIT_CYCLES
  int i = _cycleBox->currentIndex();
#else
  int i = 0;
#endif
  delete _cycles[i];
  Cycle* cycle = new Cycle(_s.dataSize());
  _s.writeParameters(cycle->buffer);
  _cycles[i] = cycle;

  // Create the new cycle
  cycle = new Cycle(_s.dataSize());
  _s.writeParameters(cycle->buffer);
  _cycles.insert(_cycles.begin()+i+1,cycle);

  _current = i+1;
#ifdef EDIT_CYCLES
  _cycleBox->addItem(QString("%1").arg(_cycles.size()-1));
  _cycleBox->setCurrentIndex(_current);
#endif
}

void Dialog::remove_cycle()
{
  unsigned i = _cycleBox->currentIndex();
  delete _cycles[i];
  _cycles.erase(_cycles.begin()+i);

  _current = i<_cycles.size() ? i : _cycles.size()-1;
#ifdef EDIT_CYCLES
  _cycleBox->removeItem(_cycles.size());

  _cycleBox->setCurrentIndex(_current);
#endif
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
  struct stat64 file_stat;
  if (stat64(qPrintable(file),&file_stat)) {
    char msg[256];
    sprintf(msg,"System error opening %s : %s",qPrintable(file),strerror(errno));
    printf("%s\n",msg);
    QMessageBox::critical(this,"Load Error",msg);
    return;
  }
  char* buff = new char[file_stat.st_size];
  FILE* input = fopen(qPrintable(file),"r");
  fread(buff, file_stat.st_size, 1, input);
  fclose(input);

  append(buff, file_stat.st_size);

  delete[] buff;
}

void Dialog::append(const void* p, unsigned sz)
{
  char* b = reinterpret_cast<char*>(const_cast<void*>(p));
  char* e = b + sz;
  while(b < e) {
    //    Cycle* cycle = new Cycle(_s.readParameters(b));
    int len = _s.readParameters(b);
    Cycle* cycle = new Cycle(_s.dataSize());
    b += len;
    _s.writeParameters(cycle->buffer);
    _cycles.push_back(cycle);
#ifdef EDIT_CYCLES
    _cycleBox->addItem(QString("%1").arg(_cycles.size()-1));
#endif
  }
  _current = 0;
#ifdef EDIT_CYCLES
  _cycleBox->setCurrentIndex(_current);
#endif
  _s.readParameters(_cycles[_current]->buffer);
  
  _s.flush();
}

void Dialog::showEvent(QShowEvent*)
{
  _s.flush();
}
