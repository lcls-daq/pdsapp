#include "pds/collection/CollectionManager.hh"

#include "pds/management/Query.hh"
#include "pds/management/SourceLevel.hh"
#include "pds/service/Semaphore.hh"
#include "pds/collection/CollectionPorts.hh"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

using namespace Pds;

static const unsigned MaxPayload = sizeof(PartitionAllocation);
static const unsigned ConnectTimeOut = 250; // 1/4 second

class Observer : public CollectionManager {
public:
  Observer() :
    CollectionManager(Level::Observer, 0, MaxPayload, ConnectTimeOut, 0),
    _sem(Semaphore::EMPTY),
    _allocations(new Allocation[SourceLevel::MaxPartitions()]) {}
  ~Observer() { delete[] _allocations; }
public:
  void get_partitions() {
    for(unsigned k=0; k<SourceLevel::MaxPartitions(); k++) {
      Allocation alloc("","",k);
      PartitionAllocation pa(alloc);
      Ins dst = CollectionPorts::platform();
      ucast(pa, dst);
      _sem.take();
    }
  }

  void kill_partition(unsigned partitionid) {
    for(unsigned k=0; k<SourceLevel::MaxPartitions(); k++) {
      const Allocation& a = _allocations[k];
      if (a.partitionid() == partitionid) {
	for(unsigned m=0; m<a.nnodes(); m++)
	  if (a.node(m)->level()==Level::Control) {
	    Message resign(Message::Resign, sizeof(Message));
	    _send(resign, CollectionPorts::platform(), *a.node(m));
	  }
      }
    }
  }

  void kill_partition(const char* partition) {
    for(unsigned k=0; k<SourceLevel::MaxPartitions(); k++) {
      const Allocation& a = _allocations[k];
      if (!strcmp(a.partition(), partition)) {
	for(unsigned m=0; m<a.nnodes(); m++)
	  if (a.node(m)->level()==Level::Control) {
	    Message resign(Message::Resign, sizeof(Message));
	    _send(resign, CollectionPorts::platform(), *a.node(m));
	  }
      }
    }
  }

private:
  virtual void message(const Node& hdr, const Message& msg) {
    if (hdr.level() == Level::Source &&
	msg.type () == Message::Query) {
      const Query& query = reinterpret_cast<const Query&>(msg);
      if (query.type() == Query::Partition) {
	const Allocation& alloc = reinterpret_cast<const PartitionAllocation&>(query).allocation();
	_allocations[alloc.partitionid()] = alloc;
	_sem.give();
      }
    }
  }
private:
  Semaphore  _sem;
  Allocation* _allocations;
};

#include <time.h>

void usage(const char* name)
{
  printf("usage: %s [-p partition_number] [-n partition_name]\n",name);
}

int main(int argc, char** argv)
{
  int partitionid = -1;
  const char* partition = 0;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "p:n:")) != EOF ) {
    switch(c) {
    case 'p': partitionid = strtoul(optarg, NULL, 0); break;
    case 'n': partition   = optarg; break;
    }
  }

  if (partitionid < 0 && partition==0) {
    usage(argv[0]);
    exit(1);
  }

  Observer o;
  o.start();
  if (o.connect()) {
    o.get_partitions();
    if (partitionid>=0)
      o.kill_partition(partitionid);
    if (partition!=0)
      o.kill_partition(partition);
  }
  else {
    printf("Unable to connect to source\n");
  }
}
