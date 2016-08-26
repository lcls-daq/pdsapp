#ifndef Pds_Preferences_hh
#define Pds_Preferences_hh

#include <QtCore/QList>
#include <QtCore/QString>

#include <cstdio>

namespace Pds {
  class Preferences {
  public:
    Preferences(const char* partition,
                const char* title,
                unsigned    platform,
                const char* mode);
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
    void read (QList<QString>&, 
	       QList<int>&, 
	       QList<bool>&,
	       const char*,
	       QList<bool>&,
	       const char*);

    void write(const QString&);
    void write(const QString&,
	       const char*);
    void write(const QString&, 
	       int,
	       const char*);
    void write(const QString&, 
	       int,
	       const char*,
               const char*);
  private:
    void _write   (const std::string&);
    void _nfsOpen (const char*);
    void _nfsWrite(const char*); 
    void _nfsClose();
 private:
    FILE* _f;
    FILE* _nfs;
    friend class NfsOpen;
    friend class NfsClose;
    friend class NfsWrite;
  };
};

#endif
