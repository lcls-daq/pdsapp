#include "pdsapp/config/Serializer.hh"

#include <QtGui/QBoxLayout>
#include <QtGui/QWidget>
#include <QtCore/QString>
#include <new>
#include <cassert>

using namespace Pds_ConfigDb;

Serializer::Serializer(const char* l) : label(l), path(0) {}

Serializer::~Serializer() {}

void Serializer::initialize(QWidget* parent, QBoxLayout* layout)
{
  assert(this != NULL);
  Parameter* p = pList.forward();
  if (fixedSize()) {
    layout->setSizeConstraint(QLayout::SetFixedSize);
  }
  while( p != pList.empty() ) {
    layout->addLayout(p->initialize(parent));
    p = p->forward();
  }
  parent->setWindowTitle(_name);
}

void Serializer::flush ()
{
  Parameter* p = pList.forward();
  while( p != pList.empty() ) {
    p->flush();
    p = p->forward();
  }
}

void Serializer::update()
{
  Parameter* p = pList.forward();
  while( p != pList.empty() ) {
    p->update();
    p = p->forward();
  }
}

void Serializer::setPath(const Path& p) { path = &p; }

void Serializer::name(const char* n)
{
  _name = QString(n);
}
