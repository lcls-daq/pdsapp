#include "pds/evgr/EvgrBoardInfo.hh"
#include "pds/evgr/EvgrManager.hh"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Pds;

int main(int argc, char** argv) {

  EvgrBoardInfo<Evg>& egInfo = *new EvgrBoardInfo<Evg>("/dev/ega3");
  EvgrBoardInfo<Evr>& erInfo = *new EvgrBoardInfo<Evr>("/dev/erb3");
  new EvgrManager(egInfo,erInfo);
  while (1) sleep(10);
}
