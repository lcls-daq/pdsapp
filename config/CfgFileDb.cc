#include "CfgFileDb.hh"

#include "pds/config/CfgRequest.hh"

#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

using namespace Pds;

static const int STRLEN_PLATFORM = 3;
static const int STRLEN_ENV = 8;
static const int STRLEN_SRC = 8;
static const int STRLEN_ID  = 8;
static const int STRLEN_LEVEL = 1;

CfgFileDb::CfgFileDb(const char* directory,
		     unsigned    platform) :
  _path( new char[ strlen(directory) + STRLEN_PLATFORM + STRLEN_ENV + STRLEN_SRC + STRLEN_ID + 4 ] )
{
  //  sprintf(_path,"%s/%0*x/",directory,STRLEN_PLATFORM,platform);
  sprintf(_path,"%s/",directory);
  _offset = strlen(_path);
}

CfgFileDb::~CfgFileDb() 
{
  delete[] _path;
}

int CfgFileDb::open (const CfgRequest& request, int mode)
{
  //  Lookup the file corresponding to the request
  const Src& src = request.src();
  if (src.level() == Level::Source)
    sprintf(_path+_offset,"%0*x/%0*x/%0*x",
	    STRLEN_ENV,
	    request.transition().env(),
	    STRLEN_SRC,
	    src.phy(),
	    STRLEN_ID,
	    request.id().value());
  else
    sprintf(_path+_offset,"%0*x/%0*x/%0*x",
	    STRLEN_ENV,
	    request.transition().env(),
	    STRLEN_LEVEL,
	    src.level(),
	    STRLEN_ID,
	    request.id().value());
  
  return ::open(_path, mode);
}
 
int CfgFileDb::close(int fd)
{
  close(fd);
  return 0;
}
