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
  layout(false);

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
  layout(true);

  append(file);
}

Dialog::Dialog(QWidget* parent,
	       Serializer& s,
	       const QString& read_dir,
	       const QString& write_dir,
	       const QString& file,
	       bool           edit) :
  QDialog(parent),
  _s(s),
  _read_dir (read_dir),
  _write_dir(write_dir),
  _file     (file)
{
  layout(edit);

  append(file);
}

Dialog::Dialog(QWidget* parent,
	       Serializer& s,
	       const QString& read_dir,
	       const QString& write_dir,
	       bool           edit) :
  QDialog(parent),
  _s(s),
  _read_dir (read_dir),
  _write_dir(write_dir),
  _file     ("example.xtc")
{
  layout(edit);

  // Create the new cycle
  Cycle* cycle = new Cycle(_s.dataSize());
  _s.writeParameters(cycle->buffer);
  _cycles.insert(_cycles.begin(),cycle);

  _current = 0;
}

Dialog::~Dialog() 
{
  delete &_s;
  for(unsigned i=0; i<_cycles.size(); i++)
    delete _cycles[i];
}

void Dialog::layout(bool edit)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  Parameter::allowEdit(edit);
  _s.initialize(this, layout);

  QHBoxLayout* blayout = new QHBoxLayout;
  QPushButton* bWrite = new QPushButton("Save",this);
  QPushButton* bReturn = new QPushButton("Cancel",this);
  blayout->addWidget(bWrite);
  blayout->addWidget(bReturn);
  if (edit) {
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

  int i = 0;

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
  }
  _current = 0;

  _s.readParameters(_cycles[_current]->buffer);
  
  _s.flush();
}

void Dialog::showEvent(QShowEvent*)
{
  _s.flush();
}
