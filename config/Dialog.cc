#include "Dialog.hh"
#include "Serializer.hh"

#include <QtGui/QVBoxLayout>
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
               const QString&      name,
               const void* p,
               unsigned    sz,
               bool        edit) :
  QDialog (parent),
  _s      (s),
  _name   (name),
  _payload(0)
{
  layout(edit);
  append(p,sz);
}

Dialog::Dialog(QWidget* parent,
               Serializer& s,
               const QString&      name,
               bool        edit) :
  QDialog (parent),
  _s      (s),
  _name   (name),
  _payload(0)
{
  layout(edit);
  _payload_sz = _s.dataSize();
  _payload    = new char[_payload_sz];
  _s.writeParameters(_payload);
}

Dialog::~Dialog() 
{
  delete &_s;
  if (_payload)
    delete[] _payload;
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

void Dialog::write()
{
  _s.update();

  // Validate
  if (!_s.validate())
    return;

  if (_payload)
    delete[] _payload;

  _payload_sz = _s.dataSize();
  _payload = new char[_payload_sz];
  _s.writeParameters(_payload);

  accept();
}

void Dialog::append(const void* p, unsigned sz)
{
  _s.readParameters(const_cast<void*>(p));

  if (_payload) 
    delete[] _payload;

  _payload_sz = _s.dataSize();
  _payload = new char[_payload_sz];

  _s.writeParameters(_payload);

  _s.readParameters(_payload);
  
  _s.flush();
}

void Dialog::showEvent(QShowEvent*)
{
  _s.flush();
}
