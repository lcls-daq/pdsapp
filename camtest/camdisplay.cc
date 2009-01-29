#include "pds/service/Task.hh"
#include "pds/collection/Arp.hh"
#include "pds/management/EventCallback.hh"
#include "pds/management/EventLevel.hh"

#include "pds/utility/Appliance.hh"
#include "pds/client/XtcIterator.hh"
#include "pds/xtc/InDatagramIterator.hh"
#include "pds/xtc/ZcpDatagramIterator.hh"
#include "pds/service/GenericPool.hh"
#include "pds/utility/SetOfStreams.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pds/camera/Frame.hh"
#include "pds/camera/TwoDGaussian.hh"
#include "FrameDisplay.hh"

#include <time.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

using namespace Pds;

class CamDisplay : public Appliance, XtcIterator {
  enum { MaxFrameSize = 0x200000 + sizeof(Frame) };
public:
  CamDisplay(unsigned detectorId,
	     int rate) : 
    _detectorId(detectorId),
    _step_nsec(1000000000/rate),
    _frame_buffer(new char[MaxFrameSize]),
    _iter(sizeof(ZcpDatagramIterator),1),
    _display(new FrameDisplay)
  {	
    clock_gettime(CLOCK_REALTIME, &_tp);
    _next(_tp);
  }
  ~CamDisplay()
  {
    delete   _display;
    delete[] _frame_buffer;
    delete[] _hdr_buffer;
  }

  int process(const Xtc& xtc,
	      InDatagramIterator* iter)
  {
    if (xtc.contains.id()==TypeId::Id_Xtc)
      return iterate(xtc,iter);

    int advance = 0;
    if (xtc.src.phy() == _detectorId) {
      if (xtc.contains.id() == TypeId::Id_Frame || 
	  xtc.contains.id() == TypeId::Id_TwoDGaussianAndFrame) {
	Frame& frame  = *(Frame*)_frame_buffer;
	advance += iter->copy(&frame, sizeof(Frame));
	char* buff = reinterpret_cast<char*>(&frame+1);
	char* data = (char*)iter->read_contiguous(frame.extent, buff);
	advance += frame.extent;
	{  // convert to 8-bit
	  unsigned short* ind = reinterpret_cast<unsigned short*>(data);
	  unsigned char* outd = reinterpret_cast<unsigned char*> (buff);
	  frame.extent >>= 1;
	  unsigned char* end  = outd + frame.extent;
	  while( outd < end ) {
	    *outd = (*ind >> 4) & 0xff;
	    outd++;
	    ind++;
	  }
	}

	if (xtc.contains.id() == TypeId::Id_TwoDGaussianAndFrame) {
	  TwoDGaussian fex;
	  advance += iter->copy(&fex, sizeof(fex));
	  _display->show_frame(frame,fex);
	  printf("fex: %g %g  %g %g %g\n",
		 fex._xmean, fex._ymean, fex._major_axis_width, fex._minor_axis_width, fex._major_axis_tilt);
	}
	else {
	  _display->show_frame(frame);
	}
      }
    }
    return advance;
  }

  Transition* transitions(Transition* in) { return in; }
  InDatagram* occurrences(InDatagram* in) { return in; }
  InDatagram* events     (InDatagram* in)
  {
    if (in->datagram().seq.service() == TransitionId::L1Accept) {
      timespec tp;
      clock_gettime(CLOCK_REALTIME, &tp);
      if ((tp.tv_sec  > _tp.tv_sec) ||
	  (tp.tv_sec == _tp.tv_sec && tp.tv_nsec < _tp.tv_nsec)) {
	_next(tp);
	InDatagramIterator* in_iter = in->iterator(&_iter);
	iterate(in->datagram().xtc,in_iter);
	delete in_iter;
      }
    }
    return in; 
  }
private:
  void _next(const timespec& tp) {
    _tp.tv_sec  = tp.tv_sec;
    _tp.tv_nsec = tp.tv_nsec + _step_nsec;
    if (_tp.tv_nsec > 1000000000) {
      _tp.tv_sec++;
      _tp.tv_nsec -= 1000000000;
    }
  }
private:
  unsigned _detectorId;
  int      _step_nsec;
  timespec _tp;
  char*    _hdr_buffer;
  char*    _frame_buffer;
  GenericPool _iter;
  FrameDisplay* _display;
};

class MyCallback : public EventCallback {
public:
  MyCallback(Task* task, unsigned detid, int rate) : 
    _task(task), _display(new CamDisplay(detid,rate)) {}
  ~MyCallback() {}

  void attached (SetOfStreams& streams) 
  {
    Stream* frmk = streams.stream(StreamParams::FrameWork);
    _display->connect(frmk->inlet());
  }
  void failed   (Reason reason) { _task->destroy(); delete this; }
  void dissolved(const Node& who) { _task->destroy(); delete this; }
private:
  Task*       _task;
  CamDisplay* _display;
};

int main(int argc, char** argv) {

  int display_rate=1;
  unsigned platform=-1UL;
  unsigned detector_id=0;
  const char* arpsuidprocess = 0;
  int c;
  while ((c = getopt(argc, argv, "d:p:r:")) != -1) {
    errno = 0;
    char* endPtr;
    switch (c) {
    case 'p':
      platform = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) platform = -1UL;
      break;
    case 'a':
      arpsuidprocess = optarg;
      break;
    case 'd':
      detector_id = strtoul(optarg, &endPtr, 0);
      if (errno != 0 || endPtr == optarg) detector_id = 0;
    case 'r':
      display_rate = atoi(optarg);
      break;
    }
  }

  if (platform == -1UL) {
    printf("Platform required\n");
    printf("Usage: %s -p <platform> -d <detector_id> -r <display rate> [-a <arp process id>]\n",
	   argv[0]);
    return 1;
  }

  Arp* arp=0;
  if (arpsuidprocess) {
    arp = new Arp(arpsuidprocess);
    if (arp->error()) {
      char message[128];
      sprintf(message, "failed to create Arp for `%s': %s", 
	      arpsuidprocess, strerror(arp->error()));
      printf("%s: %s\n", argv[0], message);
      delete arp;
      return 0;
    }
  }

  unsigned detector_src = DetInfo(0, DetInfo::AmoXes, detector_id, DetInfo::Opal1000, 0).phy();
  Task* task = new Task(Task::MakeThisATask);
  MyCallback* display = new MyCallback(task, detector_src, display_rate);
  EventLevel* event = new EventLevel(platform,
				     *display,
				     arp);
  if (event->attach())
    task->mainLoop();

  event->detach();
  delete display;
  delete event;
  if (arp) delete arp;
  return 0;
}

