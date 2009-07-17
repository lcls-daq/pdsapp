#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/camera/FrameV1.hh"
#include "pdsdata/pnCCD/fformat.h"
#include "pds/service/GenericPool.hh"
#include "pds/xtc/Datagram.hh"

Datagram* outdg;
int       Filedes = -1;           // file descriptor
unsigned char fileHeaderBuffer[1024];
fileHeaderType *FileHdr = (fileHeaderType*)fileHeaderBuffer;

class pnCCDconf : public Xtc {
public:
  pnCCDconf() : Xtc( TypeId(TypeId::Id_pnCCDconfig,FileHdr->version), DetInfo(0xfeed,DetInfo::Camp,8,DetInfo::pnCCD,5) ) {
    memcpy(_data, (char*)FileHdr, 1024);
    alloc(sizeof(*this)-sizeof(Xtc));
  }
private:
  unsigned char _data[1024];
};

class pnCcdRceConf : public Xtc {
public:
  pnCcdRceConf() : Xtc(TypeId(TypeId::Id_Xtc,Version),ProcInfo(Level::Segment, 0xbead, 0x7f000001)) { 
    alloc(sizeof(*this)-sizeof(Xtc));
    printf(" .%d. ", p.extent);
  }
public:
  pnCCDconf p;
private:
  enum {Version=1};
};


class pnCCDdata : public Xtc {
public:
  pnCCDdata() : Xtc( TypeId(TypeId::Id_pnCCDframe,Version), DetInfo(0xfeed,DetInfo::Camp,8,DetInfo::pnCCD,5) ) {
    if ((read(Filedes, _data, Size)) != Size) perror("pnCCDdata");
    alloc(sizeof(*this)-sizeof(Xtc));
  }
private:
  enum {Size=((256*128*sizeof(short)) + sizeof(frameHeaderType))};
  enum {Version=6};
  unsigned char _data[Size];
};

class pnCcdRce : public Xtc {
public:
  pnCcdRce() : Xtc(TypeId(TypeId::Id_Xtc,Version),ProcInfo(Level::Segment, 0xbead, 0x7f000001)) { 
    alloc(sizeof(*this)-sizeof(Xtc));
    printf(" .%d. ", p.extent);
  }
public:
  pnCCDdata p;
private:
  enum {Version=1};
};

class myLevelIter : public XtcIterator {
public:
  enum {Stop, Continue};
  myLevelIter(Xtc* xtc, unsigned depth) : XtcIterator(xtc), _depth(depth), _higherXtc(xtc) {}

  int process(Xtc* xtc) {
    printf(" ^%d ", xtc->extent);
    unsigned i=_depth; while (i--) printf("--");
    Level::Type level = xtc->src.level();
    printf(" %d %s level: %d ",_depth, Level::name(level), level);
    if (level==Level::Event) {
      // copy over the "Event" xtc header
      Xtc* eventXtc = new((char*)&(outdg->xtc)) Xtc(xtc->contains,xtc->src,xtc->damage);
      // copy over the existing payload (includes existing segment and source level)
      void* payload = eventXtc->alloc(xtc->sizeofPayload());
      memcpy(payload,xtc->payload(),xtc->sizeofPayload());
      // add the new segment xtc at the end
      if (outdg->seq.service() == TransitionId::L1Accept) {
	pnCcdRce* r = new(eventXtc) pnCcdRce();
	printf(" ~%d\n", r->extent);
      }
      else if (outdg->seq.service() == TransitionId::Configure) {
	pnCcdRceConf* c = new(eventXtc) pnCcdRceConf();
	printf(" ~%d\n", c->extent);
      }
    }
    return Continue;
  }
private:
  unsigned _depth;
  Xtc*     _higherXtc;
};

void usage(char* progname) {
  fprintf(stderr,"Usage: %s [-h] -f <filename> -p <pnccdFilename> -o <outFilename>\n", progname);
}

int main(int argc, char* argv[]) {
  int c;
  char* xtcname=0;
  char* Filename=0;
  char* outFilename=0;

  while ((c = getopt(argc, argv, "hf:p:o:")) != -1) {
    switch (c) {
    case 'h':
      usage(argv[0]);
      break;
    case 'f':
      xtcname = optarg;
      break;
    case 'p':
      Filename = optarg;
      break;
    case 'o':
      outFilename = optarg;
      break;
    default:
      usage(argv[0]);
      exit(1);
    }
  }
  
  if (!xtcname || !Filename || !outFilename) {
    usage(argv[0]);
    exit(1);
  }

  FILE* file = fopen(xtcname,"r");
  if (!file) {
    char s[120];
    sprintf(s, "Unable to open XTC file %s ", xtcname);
    perror(s);
    exit(2);
  }

  FILE* outFile = fopen(outFilename,"w");
  if (!outFile) {
    char s[120];
    sprintf(s, "Unable to open output file %s ", outFilename);
    perror(s);
    exit(2);
  }

  if ((Filedes = open(Filename, O_RDONLY)) == -1) {
    char s[120];
    sprintf(s, "Unable to open pnCCD file %s ", Filename);
    perror(s);
    exit(2);
  }

  read(Filedes, (char*)FileHdr, 1024);
  printf("pnCCD File Header:\n");
  printf("\tmyLength  %d, fhLength %d, nCCDs %d, width %d, maxHeight %d, version %d\n\t dataSetID %s\n", 
		FileHdr->myLength, FileHdr->fhLength, FileHdr->nCCDs, FileHdr->width, 
		FileHdr->maxHeight, FileHdr->version, FileHdr->dataSetID);
  if (FileHdr->version > 5) {
    printf("\tthe_width %d, the_maxHeight %d\n", FileHdr->the_width, FileHdr->the_maxHeight);
  }

  GenericPool* pool = new GenericPool(0x400000,1);

  XtcFileIterator iter(file,0x400000);
  Dgram* indg;
  while ((indg = iter.next())) {
    printf("%s transition: time 0x%x/0x%x, payloadSize %d\n",TransitionId::name(indg->seq.service()),
           indg->seq.stamp().fiducials(),indg->seq.stamp().ticks(),indg->xtc.sizeofPayload());

    if ((indg->seq.service()!=TransitionId::L1Accept) && (indg->seq.service()!=TransitionId::Configure)) {
      printf("*** non-l1\n");
      //  fwrite indg and indg->payload;
      fwrite((char*)indg,sizeof(Dgram)+indg->xtc.sizeofPayload(),1,outFile);
    } else {
      outdg = new(pool) Datagram(*indg);
      myLevelIter iter(&(indg->xtc),0);
      iter.iterate();
      //  fwrite outdg and outdg->payload;
      fwrite((char*)outdg,sizeof(Dgram)+outdg->xtc.sizeofPayload(),1,outFile);
      printf("*** l1 v%d ^%d\n", indg->xtc.extent,  outdg->xtc.extent);
      delete outdg;
    }
  }
  fclose(file);
  fclose(outFile);
  close(Filedes);
  return 0;
}
