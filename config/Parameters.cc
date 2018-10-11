#include "pdsapp/config/Parameters.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QFileDialog>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>

#include <string.h>

using namespace Pds_ConfigDb;

const char* Enums::Bool_Names[] = { "False", "True", NULL };
const char* Enums::Polarity_Names[] = { "Pos", "Neg", NULL };
const char* Enums::Enabled_Names[]  = { "Enable", "Disable", NULL };
const char* Enums::Disabled_Names[] = { "Disable", "Enable", NULL };

static bool _edit = false;
static bool _read = false;

Parameter::Parameter() : _label(0), _allowEdit(_edit) {}
Parameter::Parameter(const char* l) : _label(l), _allowEdit(_edit) {}

void Parameter::allowEdit(bool edit) { _edit = edit; }
bool Parameter::allowEdit() const { return _allowEdit; }

void Parameter::readFromData(bool r) { _read = r; }
bool Parameter::readFromData() { return _read; }

TextParameter::TextParameter(const char* label, const char* val, unsigned size) :
  Parameter(label),
  _size    (size)
{
  memset (value, 0, size);
  strncpy(value, val, size);
}


TextParameter::~TextParameter() 
{
}

QLayout* TextParameter::initialize(QWidget* parent)
{
  QHBoxLayout* layout = new QHBoxLayout;
  layout->addWidget(new QLabel(_label));
  if (allowEdit()) {
    layout->addWidget(_input = new QLineEdit(parent));
    _input->setMaxLength(_size);
    flush();
  }
  else {
    _display = new QLabel(value);
    _display->setFrameShape(QFrame::Box);
    layout->addWidget(_display);
  }
  layout->setContentsMargins(0,0,0,0);
  return layout;
}

void     TextParameter::update()
{
  if (allowEdit())
    strncpy(value, qPrintable(_input->text()), _size);
}

void     TextParameter::flush ()
{
  if (allowEdit())
    _input->setText(value);
  else
    _display->setText(value);
}

void     TextParameter::enable(bool v)
{
  _input->setReadOnly(!(allowEdit() && v));
}

QWidget* TextParameter::widget() 
{
  return allowEdit() ? 
    static_cast<QWidget*>(_input) : 
    static_cast<QWidget*>(_display); 
}

ParameterFile::ParameterFile(const char* p) :
  Parameter(p) {}

PolyDialog::PolyDialog(ParameterFile& p) :
  _p(p),
  _d(new QFileDialog)
{
}

FilterDialog::FilterDialog(ParameterFile& p) :
  _p(p),
  _d(new QFileDialog)
{
}

FilterDialog::FilterDialog(ParameterFile& p, const QString& filter, const QString& caption, const QString& directory) :
  _p(p),
  _d(new QFileDialog(0, caption, directory, filter))
{
}

TextFileParameter::TextFileParameter(const char* label, unsigned maxsize) :
  ParameterFile(label),
  size(0),
  _filter(0),
  _maxsize(maxsize)
{
  value = new char[_maxsize];
}

TextFileParameter::TextFileParameter(const char* label, unsigned maxsize, const char* filter) :
  ParameterFile(label),
  size(0),
  _filter(filter),
  _maxsize(maxsize)
{
  value = new char[_maxsize];
}

TextFileParameter::~TextFileParameter()
{
  delete _dialog;
  delete[] value;
}

QLayout* TextFileParameter::initialize(QWidget*)
{
  QGridLayout* l = new QGridLayout;
  l->addWidget(_display = new QLabel(QString("%1 (%2)")
             .arg(_label)
             .arg(size)),
         0,0,1,2,Qt::AlignHCenter);
  l->addWidget(_import = new QPushButton("Import"),1,0);
  l->addWidget(_export = new QPushButton("Export"),1,1);

  if (_filter)
    _dialog = new FilterDialog(*this, _filter);
  else
    _dialog = new FilterDialog(*this);

  QObject::connect(_import, SIGNAL(clicked()), _dialog, SLOT(mport()));
  QObject::connect(_export, SIGNAL(clicked()), _dialog, SLOT(xport()));

  return l;
}

void TextFileParameter::update() {}

void TextFileParameter::flush()
{
  _display->setText(QString("%1 (%2)")
                    .arg(_label)
                    .arg(size));
}

void TextFileParameter::enable(bool v)
{
  _import->setEnabled(v);
  _export->setEnabled(v);
}

void TextFileParameter::mport(const QString& fname)
{
  FILE* f = fopen(qPrintable(fname), "r");
  if (f) {
    ssize_t ret = 0;
    size_t bytes_read = 0;
    do {
      bytes_read += (unsigned) ret;
      ret = ::fread(value+bytes_read, sizeof(char), _maxsize-bytes_read, f);
    } while(ret > 0);

    if (ret < 0) {
      value[0] = 0;
      size = 0;
    } else {
      size = bytes_read;
      if (size < _maxsize) {
        value[size] = 0;
      } else {
        value[_maxsize - 1] = 0;
      }
    }

    _display->setText(QString("%1 (%2)")
                      .arg(_label)
                      .arg(size));
  }
}

void TextFileParameter::xport(const QString& fname) const
{
  FILE* f = fopen(qPrintable(fname), "w");
  if (f) {
    ::fwrite(value, sizeof(char), size, f);
  }
}

void TextFileParameter::set_value(const char* text)
{
  strncpy(value, text, _maxsize);
  size = strlen(value);
}

unsigned TextFileParameter::length() const
{
  return size + 1;
}

CheckValue::CheckValue(const char* label,
		       bool checked) :
  Parameter(label),
  value    (checked)
{
}

CheckValue::~CheckValue()
{}

QLayout* CheckValue::initialize(QWidget*)
{
  QHBoxLayout* h = new QHBoxLayout;
  h->addWidget(_input = new QCheckBox(_label));
  flush();
  _input->setEnabled(allowEdit());
  return h;
}

void CheckValue::update()
{
  value = _input->isChecked();
}

void CheckValue::flush()
{
  _input->setChecked(value);
}

void CheckValue::enable(bool v)
{
  _input->setEnabled(allowEdit()&v);
}
