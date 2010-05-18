#include "pdsapp/config/Serializer.hh"

#include <QtGui/QBoxLayout>

using namespace Pds_ConfigDb;

Serializer::Serializer(const char* l) : label(l), path(0) {}

Serializer::~Serializer() {}

void Serializer::initialize(QWidget* parent, QBoxLayout* layout)
{
  Parameter* p = pList.forward();
  while( p != pList.empty() ) {
    layout->addLayout(p->initialize(parent));
    p = p->forward();
  }
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
