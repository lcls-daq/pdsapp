#include "pdsapp/config/Parameters.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

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
