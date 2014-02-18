#include "pdsapp/control/SequencerSync.hh"

#include <stdio.h>

using namespace Pds;

static char _buff[64];
static const char* pvname(const char* format, const char* base, unsigned id)
{
  sprintf(_buff,format,base,id);
  return _buff;
}

static const char* ecs_ioc = "ECS:SYS0";

SequencerSync::SequencerSync(unsigned id) :
  _seq_cntl_writer  (pvname("%s:%d:PLYCTL",ecs_ioc,id))
{
}

void SequencerSync::start()
{
  *reinterpret_cast<int*>(_seq_cntl_writer  .data()) = 1;
  _seq_cntl_writer  .put();
  ca_flush_io();

  printf("SequencerSync start\n");
}

void SequencerSync::stop()
{
  *reinterpret_cast<int*>(_seq_cntl_writer  .data()) = 0;
  _seq_cntl_writer  .put();
  ca_flush_io();

  printf("SequencerSync stop\n");
}
