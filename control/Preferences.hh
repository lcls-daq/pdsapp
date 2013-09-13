#ifndef Pds_Preferences_hh
#define Pds_Preferences_hh

#include <cstdio>

namespace Pds {
  class Preferences {
  public:
    Preferences(const char* title,
                unsigned    platform,
                const char* mode);
    ~Preferences();
  public:
    FILE* file();
  private:
    FILE* _f;
  };
};

#endif
