#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>

#include "pds/service/Timer.hh"
#include "pds/service/Task.hh"

#include "pds/mon/MonCds.hh"
#include "pds/mon/MonGroup.hh"
#include "pds/mon/MonPort.hh"
#include "pds/mon/MonServerManager.hh"
#include "pds/mon/MonEntryTH1F.hh"
#include "pds/mon/MonEntryTH2F.hh"
#include "pds/mon/MonEntryImage.hh"
#include "pds/mon/MonEntryProf.hh"
#include "pdsdata/xtc/ClockTime.hh"

using namespace Pds;

static void build(MonCds& cds)
{
  const unsigned char ngroups = 4;
  const char* dirs[ngroups] = {"Svt:Crate0:Slot0",
			       "Svt:Crate0:Slot1",
			       "Svt:Crate1:Slot0",
			       "Svt:Crate1:Slot1"};
  const unsigned char nentries = 3;
  const char* names[nentries] = {"Throttle",
				 "Posttime",
				 "Depth"};
  const char* title[nentries] = {"Throttle [counts]",
				 "Posttime [us]",
				 "Depth [counts]"};

  for (unsigned short d=0; d<ngroups; d++) {
    MonGroup* group = new MonGroup(dirs[d]);
    cds.add(group);
    for (unsigned short n=0; n<nentries; n++) {
      MonDescTH1F desc(names[n], title[n], "", 10, 0, 5);
      MonEntryTH1F* entry = new MonEntryTH1F(desc);
      group->add(entry);
      if (n == 0) {
	entry->desc().xwarnings(1.25, 1.75);
      }
    }
  }

  {
    MonGroup* group = new MonGroup("Test many");
    cds.add(group);
    unsigned many = 8;
    for (unsigned short n=0; n<many; n++) {
      char name[128];
      char xtitle[128];
      char ytitle[128];
      sprintf(name, "Test %d", n);
      sprintf(xtitle, "Xtitle %d", n);
      sprintf(ytitle, "Ytitle %d", n);
      MonDescTH1F desc(name, xtitle, ytitle, 10, 0, 5);
      MonEntry* entry = new MonEntryTH1F(desc);
      group->add(entry);
    }
  }

  {
    MonGroup* group = new MonGroup("Profile test");
    cds.add(group);
    MonDescProf desc("Occupancy", "Azimuth", "Occ [%]", 8, 0, 4,
			 "A:B:C:D:E:F:G:H:I:L:");
    MonEntry* entry = new MonEntryProf(desc);
    group->add(entry);
    entry->desc().ywarnings(4.25, 5.75);
  }

  {
    MonGroup* group = new MonGroup("2D test");
    cds.add(group);
    MonDescTH2F desc("ZX vtx", "X position [mm]", "Z position [mm]",
			 10, 0, 5, 10, 0, 10);
    MonEntry* entry = new MonEntryTH2F(desc);
    group->add(entry);
    entry->desc().xwarnings(1.25, 1.75);
    entry->desc().ywarnings(4.25, 6.75);
  }

  {
    MonGroup* group = new MonGroup("Image test");
    cds.add(group);
    MonDescImage desc("Image", 512, 512, 2, 2);
    MonEntry* entry = new MonEntryImage(desc);
    group->add(entry);
    entry->desc().xwarnings(1.25, 1.75);
    entry->desc().ywarnings(4.25, 6.75);
  }
}

static float randomnumber(float low, float hig)
{
  float res = low + rand()*(hig-low)/RAND_MAX;
  return res;
}

static void randomgss(float& x, float& y)
{
  float r = sqrt(-2*log(randomnumber(0.,1.)));
  float f = randomnumber(0.,2*M_PI);
  x = r*cos(f);
  y = r*sin(f);
}

class MonServerTimerTest : public Timer {
public:
  MonServerTimerTest(MonCds& cds) :
    _cds(cds),
    _thread(new Task(TaskObject("ServerTimerTest")))
  {}

  ~MonServerTimerTest() {_thread->destroy();}


private:
  void expired() 
  {
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME,&tv);
    ClockTime now(tv.tv_sec,tv.tv_nsec);
    unsigned short ngroups = _cds.ngroups();
    for (unsigned short d=0; d<ngroups; d++) {
      MonGroup* group = const_cast<MonGroup*>(_cds.group(d));
      unsigned short nentries = group->nentries();
      for (unsigned short n=0; n<nentries; n++) {
	MonEntry* entry = group->entry(n);
	switch (entry->desc().type()) {
	case MonDescEntry::TH1F:
	  {
	    MonEntryTH1F* e = dynamic_cast<MonEntryTH1F*>(entry);
 	    e->addcontent(1, (unsigned)floorf(randomnumber(0, 10)));
	  }
	  break;
	case MonDescEntry::TH2F:
	  {
	    MonEntryTH2F* e = dynamic_cast<MonEntryTH2F*>(entry);
	    float x,y;
	    randomgss(x,y);
	    int xb = int(5+2*x);
	    int yb = int(5+2*y);
	    if (xb>=0 && xb<10 &&
		yb>=0 && yb<10)
	      e->addcontent(1, xb, yb);
	  }
	  break;
	case MonDescEntry::Image:
	  {
	    //
	    //  Make an atomic change to the image
	    //
	    _cds.payload_sem().take();
	    MonEntryImage* e = dynamic_cast<MonEntryImage*>(entry);
	    if (randomnumber(0,1)<0.5) {
	      for(unsigned iy=0; iy<512; iy++)
		for(unsigned ix=0; ix<512; ix++)
		  e->addcontent(ix, ix, iy);
	    }
	    else {
	      for(unsigned iy=0; iy<512; iy++)
		for(unsigned ix=0; ix<512; ix++)
		  e->addcontent(iy, ix, iy);
	    }
	    _cds.payload_sem().give();
	  }
	  break;
	case MonDescEntry::Prof:
	  {
	    MonEntryProf* e = dynamic_cast<MonEntryProf*>(entry);
	    assert(e != 0);
 	    unsigned bin = (unsigned)floorf(randomnumber(0, 8));
 	    double y = randomnumber(0, 10);
 	    e->addy(y, bin);
	  }
	  break;
	default:
	  break;
	}
	entry->time(now);
      }
    }
  }
  Task*    task      () {return _thread;}
  unsigned duration  () const {return 100;}
  unsigned repetitive() const {return 1;}

private:
  MonCds& _cds;
  Task* _thread;
};

int main(int argc, char **argv) 
{
  MonServerManager manager(MonPort::Test);
  MonCds& cds = manager.cds();
  build(cds);
  
  int error = manager.serve();
  if (error) {
    printf("*** MonServerManager cannot serve: %s\n", strerror(error));
    manager.dontserve();
    return 0;
  } else {
    printf("Serving %d entries:\n", cds.totalentries());
    for (unsigned g=0; g<cds.ngroups(); g++) {
      const MonGroup* group = cds.group(g);
      printf("%s group\n", group->desc().name());
      for (unsigned e=0; e<group->nentries(); e++) {
	const MonEntry* entry = group->entry(e);
	printf("  [%2d] %s\n", e+1, entry->desc().name());
      }
    }
  }

  MonServerTimerTest timertest(cds);
  timertest.start();
  
  do {
    fprintf(stdout, "Command [Enable, Disable, EOF=quit] :");
    fflush(stdout);
    const int maxlen=128;
    char line[maxlen];
    char* result = fgets(line, maxlen, stdin);
    if (!result) {
      fprintf(stdout, "\nExiting\n");
      break;
    } else if (strlen(result) == 2) {
      switch (result[0]) {
      case 'e':
      case 'E':
	manager.enable();
	break;
      case 'd':
      case 'D':
	manager.disable();
	break;
      }
    }
  } while (1);
  
  timertest.cancel();
  manager.dontserve();

  return 0;
}
