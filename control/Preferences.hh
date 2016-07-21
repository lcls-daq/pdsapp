#ifndef Pds_Preferences_hh
#define Pds_Preferences_hh

#include <QtCore/QList>
#include <QtCore/QString>

#include <cstdio>

namespace Pds {
  class Preferences {
  public:
    Preferences(const char* title,
                unsigned    platform,
                const char* mode);
    ~Preferences();
  public:
    void read (QList<QString>&);
    void read (QList<QString>&,
	       QList<bool>&,
	       const char*);
    void read (QList<QString>&, 
	       QList<int>&, 
	       QList<bool>&,
	       const char*);
    void write(const QString&);
    void write(const QString&,
	       const char*);
    void write(const QString&, 
	       int,
	       const char*);
  private:
    FILE* _f;
  };
};

#endif
