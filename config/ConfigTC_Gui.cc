#include "ConfigTC_Gui.hh"
#include "ConfigTC_Dialog.hh"
#include "ConfigTC_Serializer.hh"

#include <QtGui/QListWidget>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QVBoxLayout>

using namespace ConfigGui;

ConfigTC_Gui::ConfigTC_Gui() :
  QWidget(0)
{
    QVBoxLayout* typeLayout = new QVBoxLayout(this);
    QLabel*      typeLabel  = new QLabel("Configuration Types",this);
    _types                  = new QListWidget(this);
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(_types);
    setLayout(typeLayout);
    connect(_types, SIGNAL(itemClicked(QListWidgetItem*)), 
	    this, SLOT(launchSerializer(QListWidgetItem*)));
}

ConfigTC_Gui::~ConfigTC_Gui() 
{
}

void ConfigTC_Gui::launchSerializer(QListWidgetItem*)
{
  int i = _types->currentRow();
  Dialog* d = new Dialog(this, *_serializers[i]);
  d->exec();
}

void ConfigTC_Gui::addSerializer(Serializer* s) 
{
  _serializers[_types->count()] = s;
  _types->addItem(s->label); 
}
