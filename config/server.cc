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
  const char* directory = 0;
  unsigned platform = 0;
  unsigned max_size = 4*1024;
  unsigned nclients = 16;

  extern char* optarg;
  int c;
  while ( (c=getopt( argc, argv, "p:d:s:c:")) != EOF ) {
    switch(c) {
    case 'd':
      directory = optarg;
      break;
    case 'c':
      nclients = strtoul(optarg, NULL, 0);
      break;
    case 's':
      max_size = strtoul(optarg, NULL, 0);
      break;
    case 'p':
      platform = strtoul(optarg, NULL, 0);
      break;
    }
  }

  if (!platform) {
    printf("%s: platform required\n",argv[0]);
    return 0;
  }

  if (!directory) {
    printf("%s: base directory required\n",argv[0]);
    return 0;
  }

  Task* task = new Task(Task::MakeThisATask);

  FileServer server(directory,
		    platform,
		    max_size,
		    nclients);

  task->mainLoop();

  return 0;
}
