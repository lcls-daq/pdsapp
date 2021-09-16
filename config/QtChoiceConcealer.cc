#include "pdsapp/config/QtChoiceConcealer.hh"

#include <QtGui/QLayout>
#include <QtGui/QWidget>

using namespace Pds_ConfigDb;

QtChoiceConcealer::QtChoiceConcealer() {}

QtChoiceConcealer::~QtChoiceConcealer() {}

QLayout* QtChoiceConcealer::add(QLayout* l, int m) 
{
  _layouts.push_back(std::make_pair(l, m));
  return l;
}

QWidget* QtChoiceConcealer::add(QWidget* l, int m) 
{
  _widgets.push_back(std::make_pair(l, m));
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

void QtChoiceConcealer::show(int choice)
{
  for(unsigned j=0; j<_layouts.size(); j++)
    setChildrenVisible(_layouts[j].first,1<<choice & _layouts[j].second);
  for(unsigned j=0; j<_widgets.size(); j++)
    _widgets[j].first->setVisible(1<<choice & _widgets[j].second);
}

void QtChoiceConcealer::hide(int choice)
{
  show(~choice);
}
