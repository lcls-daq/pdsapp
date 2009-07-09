#include "pdsapp/config/Parameters.hh"

#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

#include <string.h>

using namespace Pds_ConfigDb;

const char* Enums::Bool_Names[] = { "False", "True", NULL };
const char* Enums::Polarity_Names[] = { "Pos", "Neg", NULL };
const char* Enums::Enabled_Names[] = { "Enable", "Disable", NULL };

static bool _edit = false;

void Parameter::allowEdit(bool edit) { _edit = edit; }
bool Parameter::allowEdit() { return _edit; }

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
  layout->addWidget(_input = new QLineEdit(parent));
  _input->setReadOnly(!allowEdit());
  _input->setMaxLength(_size);
  flush();
  layout->setContentsMargins(0,0,0,0);
  return layout;
}

void     TextParameter::update()
{
  strncpy(value, qPrintable(_input->text()), _size);
}

void     TextParameter::flush ()
{
  _input->setText(value);
}
