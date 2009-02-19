#ifndef Pds_MonPATH_HH
#define Pds_MonPATH_HH

namespace Pds {

  class MonPath {
  public:
    MonPath(const char* name, char separator=':');

    const char* nextdir();
    void rewind();

    static unsigned split(const char* name, char* buf, const char** ptr, char separator);
    static unsigned split(const char* name, const char** ptr, unsigned howmany);

  private:
    enum {NameSize=256};
    char _name[NameSize];
    const char* _current;
    const char* _last;
  };
};

#endif

