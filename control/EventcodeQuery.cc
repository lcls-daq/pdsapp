#include "pdsapp/control/EventcodeQuery.hh"
#include "pdsapp/config/EventcodeTiming.hh"
#include <stdio.h>

using Pds_Epics::EpicsCA;

void Pds::EventcodeQuery::execute()
{
  new Pds::EventcodeQuery;
}

Pds::EventcodeQuery::EventcodeQuery() : EpicsCA("EVNT:SYS0:1:DELAY",0)
{
  printf("Created EventcodeQuery\n");
}

Pds::EventcodeQuery::~EventcodeQuery() {}

void Pds::EventcodeQuery::connected   (bool c)
{
  Pds_Epics::EpicsCA::connected(c);
  printf("%s %s connected\n",_channel.epicsName(),c ? "is":"is not");
  if (c) {
    _channel.get();
    ca_flush_io();
  }
}

void Pds::EventcodeQuery::getData(const void* p) 
{
  EpicsCA::getData(p);
  unsigned* ticks  =reinterpret_cast<unsigned*>(_pvdata);
  Pds_ConfigDb::EventcodeTiming::timeslot(ticks);

  printf("Set timeslot data:\n");
  for(unsigned i=0; i<256; i++)
    printf("%05d%c",ticks[i],(i%10)==9 ? '\n':' ');
  printf("\n");

  delete this;
}
