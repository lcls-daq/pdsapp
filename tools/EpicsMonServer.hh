#ifndef EpicsMonServer_hh
#define EpicsMonServer_hh

#include <string>
#include <vector>

namespace Pds {

  class MyMonitorServer;

  class EpicsMonServer {
  public:
    EpicsMonServer(const char* tag);
    ~EpicsMonServer();
  public:
    void configure(const std::vector<std::string>&);
    void event    (const std::vector<double>&);
  public:
    void unconfigure();
  private:
    MyMonitorServer* _srv;
    std::vector<std::string> _names;
  };

};

#endif
