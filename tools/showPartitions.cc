#include "pds/collection/CollectionManager.hh"

#include "pds/management/Query.hh"
#include "pds/management/SourceLevel.hh"
#include "pds/service/Semaphore.hh"
#include "pds/collection/CollectionPorts.hh"

#include <string.h>

using namespace Pds;

static const unsigned MaxPayload = sizeof(PartitionAllocation);
static const unsigned ConnectTimeOut = 250; // 1/4 second

class Observer : public CollectionManager {
public:
  Observer() :
    CollectionManager(Level::Observer, 0, MaxPayload, ConnectTimeOut, 0),
    _sem(Semaphore::EMPTY),
    _allocations(new Allocation[SourceLevel::MaxPartitions()])
  {}
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

  void dump_partitions() const {
    printf("            Control   | Platform | Partition  |   Node\n"
	   "        ip     / pid  |          | id/name    | level/ pid /   ip\n"
	   "----------------------+----------+------------+-------------\n");
    for(unsigned k=0; k<SourceLevel::MaxPartitions(); k++) {
      const Allocation& a = _allocations[k];
      if (a.nnodes()==0) {
	printf("%*s     ---      %02d/%7s\n", 
	       21," ", a.partitionid(), a.partition());
      }
      else {
	for(unsigned i=0; i<a.nnodes(); i++)
	  if (a.node(i)->level() == Level::Control) {
	    const Node& n = *a.node(i);
	    unsigned ip = n.ip();
	    char ipbuff[32];
	    sprintf(ipbuff,"%d.%d.%d.%d/%05d",
		    (ip>>24), (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff, n.pid());
	    printf("%*s%s     %03d      %02d/%7s", 
		   int(21-strlen(ipbuff))," ",ipbuff, n.platform(), a.partitionid(), a.partition());
	    for(unsigned m=0; m<a.nnodes(); m++) {
	      const Node& p=*a.node(m);
	      ip = p.ip();
	      printf("%s       %1d/%05d/%d.%d.%d.%d\n", 
		     m==0 ? "":"                                             ",
		     p.level(), p.pid(),
		     (ip>>24),(ip>>16)&0xff,(ip>>8)&0xff,ip&0xff);
	    }
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

int main()
{
  Observer o;
  o.start();
  if (o.connect()) {
    o.get_partitions();
    o.dump_partitions();
  }
  else {
    printf("Unable to connect to source\n");
  }
}
