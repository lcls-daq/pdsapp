#include "CfgServer.hh"
#include "CfgFileDb.hh"
#include "pds/config/CfgPorts.hh"
#include "pds/service/Task.hh"

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

namespace Pds {

  class FileServer : public CfgServer {
  public:
    FileServer(const char* directory,
	       unsigned    platform,
	       unsigned    max_size,
	       unsigned    nclients) :
      CfgServer (platform, max_size, nclients),
      _database (directory, platform)
    {
    }
    ~FileServer() {}
  public:
    int fetch(void* where, const CfgRequest& request)
    {
      int fd;
      if ( (fd=_database.open(request,O_RDONLY)) < 0)
	return fd;

      //  Determine size
      off_t len = lseek(fd, 0, SEEK_END);
      lseek(fd, 0, SEEK_SET);

      //  Copy contents to "where" and return size
      int length = read(fd, where, len);
      if (length != len)
	printf("server: read failed: %s\n",strerror(errno));

      _database.close(fd);
      
      return length;
    }
  private:
    CfgFileDb _database;
  };

}

using namespace Pds;

int main(int argc, char** argv) {

  // parse the command line for our boot parameters
  bool lInsert=false;
  bool lList  =false;
  bool lHelp  =false;
  unsigned platform = 0;
  const char* directory = 0;
  unsigned key = 0;
  unsigned src = 0;
  const char* fname = 0;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "ilp:d:k:s:f:")) != EOF ) {
    switch(c) {
    case 'i': lInsert = true; break;
    case 'l': lList = true; break;
    case 'p': platform = strtoul(optarg, NULL, 0); break;
    case 'd': directory = optarg; break;
    case 'k': key = strtoul(optarg, NULL, 0); break;
    case 's': src = strtoul(optarg, NULL, 0); break;
    case 'f': fname = optarg; break;
    default:  lHelp = true; break;
  }

  if (!platform) {
    printf("%s: platform required\n",argv[0]);
    print_help(argv[0]);
    return 0;
  }

  if (!directory) {
    printf("%s: base directory required\n",argv[0]);
    print_help(argv[0]);
    return 0;
  }
  
  if (lInsert && (key==0 || src==0 || fname==0)) {
    printf("%s: insert requires <key>, <src>, and <filename>\n",argv[0]);
    print_help(argv[0]);
    return 0;
  }
      
  if (lList && key==0 && src==0) {
    printf("%s: list requires <key> or <src>\n",argv[0]);
    print_help(argv[0]);
    return 0;
  }

  if (lHelp) {
    print_help(argv[0]);
    return 0;
  }

  if (lInsert) {
  }

  FileServer server(directory,
		    platform,
		    max_size,
		    nclients);

  task->mainLoop();

  return 0;
}
