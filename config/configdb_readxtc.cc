#include "pdsapp/config/Xtc_Ui.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/XtcFileIterator.hh"

#include <QtGui/QApplication>

#include <stdlib.h>
#include <fcntl.h>

using namespace Pds_ConfigDb;

int main(int argc, char** argv)
{
  if (argc<2 || strcmp(argv[1],"-h")==0 || strcmp(argv[1],"--help")==0) {
    printf("Usage: %s <xtc_file> [cycle]\n",argv[0]);
  }

  int fd = ::open(argv[1],O_LARGEFILE,O_RDONLY);
  if (fd == -1) {
    char buff[256];
    sprintf(buff,"Error opening file %s\n",argv[1]);
    perror(buff);
    return false;
  }

  Pds::XtcFileIterator iter(fd,0x2000000);
  Pds::Dgram* dg;
  
  int cycle = argc>2 ? atoi(argv[2]) : 0;
  while((dg = iter.next())) {
    if (dg->seq.service()==Pds::TransitionId::Configure && argc==2)
      break;
    if (dg->seq.service()==Pds::TransitionId::BeginCalibCycle && cycle--==0)
      break;
  }
  
  if (!dg) {
    printf("Configure transition not found!\n");
    return -1;
  }

  QApplication app(argc, argv);

  Xtc_Ui* ui = new Xtc_Ui((QWidget*)0,*dg);
  ui->show();
  app.exec();
}
