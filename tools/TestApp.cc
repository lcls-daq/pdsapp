#include "pdsapp/tools/TestApp.hh"

using namespace Pds;

TestApp::TestApp() {}

TestApp::~TestApp() {}

Transition* TestApp::transitions(Transition* tr) 
{
  return tr;
}

InDatagram* TestApp::events(InDatagram* dg) 
{
  return dg;
}
