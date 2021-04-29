#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/Validators.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QFileDialog>
#include <QtGui/QCheckBox>
#include <QtGui/QPushButton>

#include <fstream>
#include <stdint.h>
#include <string.h>

using namespace Pds_ConfigDb;

const char* Enums::Bool_Names[] = { "False", "True", NULL };
const char* Enums::Polarity_Names[] = { "Pos", "Neg", NULL };
const char* Enums::Enabled_Names[]  = { "Enable", "Disable", NULL };
const char* Enums::Disabled_Names[] = { "Disable", "Enable", NULL };
const char* Enums::OnOff_Names[] = { "Off", "On", NULL };

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
  _input   (0),
  _size    (size),
  _display (0)
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
  if (allowEdit()) {
    if (_input)
      _input->setText(value);
  } else {
    if (_display)
      _display->setText(value);
  }
}

void     TextParameter::enable(bool v)
{
  if (_input)
    _input->setReadOnly(!(allowEdit() && v));
}

QWidget* TextParameter::widget() 
{
  return allowEdit() ? 
    static_cast<QWidget*>(_input) : 
    static_cast<QWidget*>(_display); 
}

TextParameter& TextParameter::operator=(const TextParameter& o)
{
  for(unsigned i=0; i<MaxSize; i++)
    value[i] = o.value[i];
  _size = o._size;
  flush();
  return *this;
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
  version(0),
  _filter(0),
  _maxsize(maxsize)
{
  value = new char[_maxsize];
}

TextFileParameter::TextFileParameter(const char* label, unsigned maxsize, const char* filter) :
  ParameterFile(label),
  size(0),
  version(0),
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
  _display->setText(QString("%1 (%2) v%3")
                    .arg(_label)
                    .arg(size)
                    .arg(version));
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

    version++;

    _display->setText(QString("%1 (%2) v%3")
                      .arg(_label)
                      .arg(size)
                      .arg(version));
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
  version++;
}

void TextFileParameter::set_value(const char* text, unsigned new_version)
{
  strncpy(value, text, _maxsize);
  size = strlen(value);
  version = new_version;
}

unsigned TextFileParameter::length() const
{
  return size + 1;
}

CheckValue::CheckValue(const char* label,
		       bool checked) :
  Parameter(label),
  value    (checked),
  _input   (0)
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
  if (_input)
    _input->setChecked(value);
}

void CheckValue::enable(bool v)
{
  _input->setEnabled(allowEdit()&v);
}

CheckValue& CheckValue::operator=(const CheckValue& o)
{
  value = o.value;
  flush();
  return *this;
}

template <class T>
NumericInt<T>::NumericInt() :
  _input(0),
  _display(0)
{}

template <class T>
NumericInt<T>::NumericInt(const char* label, T val, T vlo, T vhi, IntMode mo, double sca) :
  Parameter(label),
  value(val),
  mode (mo),
  scale(sca),
  _input(0),
  _display(0)
{
  range[0]=vlo;
  range[1]=vhi;
}

template <class T>
NumericInt<T>::~NumericInt() 
{
}

template <class T>
QLayout* NumericInt<T>::initialize(QWidget* parent)  
{
  QHBoxLayout* layout = new QHBoxLayout;
  if (_label)
    layout->addWidget(new QLabel(_label));             
  if (allowEdit()) {
    layout->addWidget(_input = new QLineEdit(parent));  
    _input->setReadOnly(!allowEdit());
    flush();
    switch(mode) {
    case Decimal:
      _input->setValidator(new IntValidator( *this, *_input, range[0], range[1]));
      break;
    case Hex:
      _input->setValidator(new HexValidator( *this, *_input, range[0], range[1]));
      break;
    default:
      _input->setValidator(new DoubleValidator( *this, *_input, range[0]*scale, range[1]*scale));
    }
  }
  else {
    switch(mode) {
    case Decimal: _display = new QLabel(QString::number(value,10)); break;
    case Hex:     _display = new QLabel(QString::number(value,16)); break;
    default:      _display = new QLabel(QString::number(double(value)*scale)); break;
    }
    _display->setFrameShape(QFrame::Box);
    layout->addWidget(_display);
  }
  layout->setContentsMargins(0,0,0,0);               
  return layout;                                     
}

template <class T>
void NumericInt<T>::update()
{
  if (allowEdit()) {
    bool ok;
    switch(mode) {
    case Decimal:  value = _input->text().toInt(); break;
    case Hex:      value = _input->text().toULongLong(&ok,16); break;
    default:       value = int(_input->text().toDouble(&ok)/scale + 0.5); break;
    }
  }
}

template <class T>
void NumericInt<T>::flush()
{
  QString v;
  switch(mode) {
    case Decimal: v = QString::number(value,10); break;
    case Hex:	  v = QString::number(value,16); break;
    default:      v = QString::number(double(value)*scale); break;
  }

  if (allowEdit()) {
    if (_input)
      _input->setText(v);
  } else {
    if (_display)
      _display->setText(v);
  }
}

template <class T>
void NumericInt<T>::enable(bool v)
{
  if (allowEdit())
    _input->setReadOnly(!v);
}

template <class T>
bool NumericInt<T>::connect(ParameterSet& set)
{
  return QObject::connect(_input, SIGNAL(editingFinished()),
	 	          set._qset, SLOT(membersChanged()));
}

template <class T>
unsigned NumericInt<T>::count()
{
  return value;
}

template <class T>
QWidget* NumericInt<T>::widget()
{
  return allowEdit() ? static_cast<QWidget*>(_input) : static_cast<QWidget*>(_display);
}

template <class T>
NumericInt<T>& NumericInt<T>::operator=(const NumericInt<T>& o)
{
  value = o.value;
  mode = o.mode;
  scale = o.scale;
  range[0] = o.range[0];
  range[1] = o.range[1];
  flush();
  return *this;
}

template <class T>
NumericFloat<T>::NumericFloat() :
  _input(0),
  _display(0)
{}

template <class T>
NumericFloat<T>::NumericFloat(const char* label, T val, T vlo, T vhi) :
  Parameter(label),
  value(val),
  _input(0),
  _display(0)
{
  range[0] = vlo;
  range[1] = vhi;
}

template <class T>
NumericFloat<T>::~NumericFloat() 
{
}

template <class T>
QLayout* NumericFloat<T>::initialize(QWidget* parent)  
{
  QHBoxLayout* layout = new QHBoxLayout;
  if (_label)
    layout->addWidget(new QLabel(_label));             
  if (allowEdit()) {
    layout->addWidget(_input = new QLineEdit(parent));  
    _input->setReadOnly(!allowEdit());
    flush();
    _input->setValidator( new DoubleValidator( *this, *_input,
	  				       range[0], range[1]) );
  }
  else {
    _display = new QLabel(QString::number(value));
    _display->setFrameShape(QFrame::Box);
    layout->addWidget(_display);
  }
  layout->setContentsMargins(0,0,0,0);               
  return layout;                                     
}

template <class T>
void NumericFloat<T>::update()
{
  if (allowEdit())
    value = _input->text().toDouble();
}

template <class T>
void NumericFloat<T>::flush()
{
  if (allowEdit()) {
    if(_input)
      _input->setText(QString::number(value));
  } else {
    if(_display)
      _display->setText(QString::number(value));
  }
}

template <class T>
void NumericFloat<T>::enable(bool v)
{
  if (allowEdit())
    _input->setReadOnly(!v);
}

template <class T>
NumericFloat<T>& NumericFloat<T>::operator=(const NumericFloat<T>& o)
{
  value = o.value;
  range[0] = o.range[0];
  range[1] = o.range[1];
  flush();
  return *this;
}

template <class T>
Poly<T>::Poly(const char* label) :
  ParameterFile(label),
  _display(0) {}

template <class T>
Poly<T>::~Poly() { delete _dialog; }

template <class T>
QLayout* Poly<T>::initialize(QWidget*)
{
  QGridLayout* l = new QGridLayout;
  l->addWidget(_display = new QLabel(QString("%1 (%2)")
				     .arg(_label)
				     .arg(value.size())),
	       0,0,1,2,Qt::AlignHCenter);
  l->addWidget(_import = new QPushButton("Import"),1,0);
  l->addWidget(_export = new QPushButton("Export"),1,1);

  _dialog = new PolyDialog(*this);

  QObject::connect(_import, SIGNAL(clicked()), _dialog, SLOT(mport()));
  QObject::connect(_export, SIGNAL(clicked()), _dialog, SLOT(xport()));

  return l;
}

template <class T>
void Poly<T>::update() {}

template <class T>
void Poly<T>::flush() 
{
  if (_display)
    _display->setText(QString("%1 (%2)")
                      .arg(_label)
                      .arg(value.size()));
}

template <class T>
void Poly<T>::enable(bool v)
{
  _import->setEnabled(v);
  _export->setEnabled(v);
}

template <class T>
void Poly<T>::mport(const QString& fname)
{
  std::ifstream f(qPrintable(fname));
  if (f.good()) {
    std::vector<T> a;
    T v;
    while(1) {
      f >> v; 
      if (!f.good()) break;
      a.push_back(v); }
    value = a;
    _display->setText(QString("%1 (%2)")
                      .arg(_label)
                      .arg(value.size()));
  }
}

template <class T>
void Poly<T>::xport(const QString& fname) const
{
  std::ofstream f(qPrintable(fname));
  if (f.good()) {
    for(unsigned i=0; i<value.size(); i++)
      f << value[i] << ' ';
  }
}

template <class T>
Poly<T>& Poly<T>::operator=(const Poly<T>& o)
{
  for(unsigned i=0; i<value.size(); i++)
    value[i] = o.value[i];
  flush();
  return *this;
}


template class NumericInt<unsigned char>;
template class NumericInt<char>;
template class NumericInt<unsigned short>;
template class NumericInt<short>;
template class NumericInt<unsigned>;
template class NumericInt<uint64_t>;
template class NumericInt<int>;
template class NumericFloat<float>;
template class NumericFloat<double>;
template class Poly<double>;
