#include "pdsapp/tools/MonReqServer.hh"

using namespace Pds;

MonReqServer::MonReqServer() {}

Transition* MonReqServer::transitions(Transition* tr)
{
  printf("MonReqServer transition %s\n",TransitionId::name(tr->id()));
  return tr;
}

InDatagram* MonReqServer::events     (InDatagram* dg)
{
  return dg;
}
