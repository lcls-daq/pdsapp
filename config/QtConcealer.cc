#include "pdsapp/config/QtConcealer.hh"

#include <QtGui/QLayout>
#include <QtGui/QWidget>

using namespace Pds_ConfigDb;

QtConcealer::QtConcealer() {}

QtConcealer::~QtConcealer() {}

QLayout* QtConcealer::add(QLayout* l) 
{
  _layouts.push_back(l);
  return l;
}

QWidget* QtConcealer::add(QWidget* l) 
{
  _widgets.push_back(l);
  return l;
}

static void setChildrenVisible(QLayout* l, bool v)
{
  for(int i=0; i<l->count(); i++) {
    QLayoutItem* item = l->itemAt(i);
    if (item->widget())
      item->widget()->setVisible(v);
    else if (item->layout())
      setChildrenVisible(item->layout(), v);
  }
}

void QtConcealer::show(bool v)
{
  for(unsigned j=0; j<_layouts.size(); j++)
    setChildrenVisible(_layouts[j],v);
  for(unsigned j=0; j<_widgets.size(); j++)
    _widgets[j]->setVisible(v);
}

void QtConcealer::hide(bool v)
{
  show(!v);
}
