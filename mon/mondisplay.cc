#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "pds/service/Task.hh"
#include "MonMain.hh"

#include "pds/mon/MonPort.hh"

void printHelp(const char* program);

using namespace Pds;

int main(int argc, char **argv) 
{

  const char* config = 0;
  const char* hosts[MonPort::NTypes];
  memset(hosts, 0, MonPort::NTypes*sizeof(const char*));
  {
    int c;
    while ((c = getopt(argc, argv, "m:v:d:x:f:")) != -1) {
      switch (c) {
      case 'm':
	hosts[MonPort::Mon] = optarg;
	break;
      case 'v':
	hosts[MonPort::Vmon] = optarg;
	break;
      case 'd':
	hosts[MonPort::Dhp] = optarg;
	break;
      case 'x':
	hosts[MonPort::Vtx] = optarg;
	break;
      case 'f':
	config = optarg;
	break;
      default:
	printHelp(argv[0]);
	return 0;
      }
    }
  }
  unsigned p;
  for (p=0; p<MonPort::NTypes; p++) if (hosts[p]) break;
  if (p == MonPort::NTypes) {
    printHelp(argv[0]);
    return 0;
  }

  Task* workTask = new Task(TaskObject("MonMain"));
  MonMain dspl(workTask, hosts, config);
}


void printHelp(const char* program)
{
  printf("usage: %s [-m <monserver>] [-v <vmonserver>] [-d <dhpserver>]"
	 " [-x <vtxserver>] [-f <configfile>]\n"
	 "       (at least one server must be specified)\n", program);
}
