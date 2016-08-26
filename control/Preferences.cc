#include "pdsapp/control/Preferences.hh"
#include "pds/service/Routine.hh"
#include "pds/service/Task.hh"

#include <QtCore/QStringList>

#include <sstream>
#include <string>
#include <map>

#include <stdlib.h>
#include <errno.h>

typedef std::map<Pds::Preferences*,FILE*> fmap_t;
static fmap_t _fmap;

namespace Pds {
  class NfsOpen : public Routine {
  public:
    NfsOpen(Preferences&       parent,
            const std::string& path) :
      _parent(parent),
      _path  (path) {}
  public:
    void routine() { _fmap[&_parent] = fopen(_path.c_str(),"w"); delete this; }
  private:
    Preferences& _parent;
    std::string  _path;
  };

  class NfsClose : public Routine {
  public:
    NfsClose(Preferences&       parent) :
      _parent(parent) {}
  public:
    void routine() { 
      fmap_t::iterator it = _fmap.find(&_parent); 
      if (it != _fmap.end()) {
        fclose(it->second);
        _fmap.erase(it);
      }
      delete this;
    }
  private:
    Preferences& _parent;
  };

  class NfsWrite : public Routine {
  public:
    NfsWrite(Preferences& parent,
             const std::string& line) :
      _parent(parent),
      _line  (line) {}
  public:
    void routine() { 
      fmap_t::iterator it = _fmap.find(&_parent);
      if (it != _fmap.end())
        fprintf(it->second,_line.c_str());
      delete this;
    }
  private:
    Preferences& _parent;
    std::string  _line;
  };
};

using namespace Pds;

static Task* _task = 0;

static const unsigned NODE_BUFF_SIZE=256;

Preferences::Preferences(const char* title,
                         unsigned    platform,
                         const char* mode) :
  _f  (0),
  _nfs(0)
{
  //  Try the /tmp directory first (reduce NFS accesses)
  {
    std::stringstream o;
    o << "/tmp/";
    o << "." << title << " for platform " << platform;

    _f = fopen(o.str().c_str(),mode);
    printf("%s open %s\n", (_f ? "Successful":"Failed to"), o.str().c_str());
  }
  //  Try the home directory on NFS
  {
    std::stringstream o;
    char* home = getenv("HOME");
    if (home)
      o << home << '/';
    o << "." << title << " for platform " << platform;
    if (_f) {
      if (mode[0]=='w') {
        if (!_task) {
          printf("Starting Nfs task\n");
          _task = new Task(TaskObject("prefnfs"));
        }
        _task->call(new NfsOpen(*this,o.str()));
      }
    }
    else
      _f = fopen(o.str().c_str(),mode); 
  }

  if (!_f) {
    std::string msg("Failed to open preferences for ");
    msg += std::string(title);
    perror(msg.c_str());
  }
}

Preferences::~Preferences() { if (_f) fclose(_f); }

void Preferences::read (QList<QString>& s)
{
  if (_f) {
    char *buff = (char *)malloc(NODE_BUFF_SIZE);  // use malloc w/ getline
    if (buff == (char *)NULL) {
      printf("%s: malloc(%d) failed, errno=%d\n", __PRETTY_FUNCTION__, NODE_BUFF_SIZE, errno);
      return;
    }

    char* lptr=buff;
    size_t linesz = NODE_BUFF_SIZE;         // initialize for getline
    
    while(getline(&lptr,&linesz,_f)!=-1) {
      QString p(lptr);
      p.chop(1);  // remove new-line
      s.push_back(p);
    }
    free(buff);
  }      
}

void Preferences::read (QList<QString>& s, 
			QList<bool>&    b,
			const char*     cTruth)
{
  if (_f) {
    char *buff = (char *)malloc(NODE_BUFF_SIZE);  // use malloc w/ getline
    if (buff == (char *)NULL) {
      printf("%s: malloc(%d) failed, errno=%d\n", __PRETTY_FUNCTION__, NODE_BUFF_SIZE, errno);
      return;
    }

    char* lptr=buff;
    size_t linesz = NODE_BUFF_SIZE;         // initialize for getline
    
    while(getline(&lptr,&linesz,_f)!=-1) {
      QString p(lptr);
      p.chop(1);  // remove new-line
      p.replace('\t','\n');        

      QStringList ls = p.split(';');

      switch (ls.size()) {
      case 1:
	s.push_back(ls[0]);
	b.push_back(false);
	break;
      case 2:
	s.push_back(ls[0]);
	b.push_back(ls[1].compare(cTruth,::Qt::CaseInsensitive)==0);
	break;
      default:
	break;
      }
    }
    free(buff);
  }
}

void Preferences::read (QList<QString>& s,
			QList<int>&     i, 
			QList<bool>&    b,
			const char*     cTruth)
{
  if (_f) {
    char *buff = (char *)malloc(NODE_BUFF_SIZE);  // use malloc w/ getline
    if (buff == (char *)NULL) {
      printf("%s: malloc(%d) failed, errno=%d\n", __PRETTY_FUNCTION__, NODE_BUFF_SIZE, errno);
      return;
    }

    char* lptr=buff;
    size_t linesz = NODE_BUFF_SIZE;         // initialize for getline
    
    while(getline(&lptr,&linesz,_f)!=-1) {
      QString p(lptr);
      p.chop(1);  // remove new-line
      p.replace('\t','\n');        

      QStringList ls = p.split(';');

      switch (ls.size()) {
      case 1:
	s.push_back(ls[0]);
	i.push_back(0);
	b.push_back(false);
	break;
      case 2:
	s.push_back(ls[0]);
	i.push_back(ls[1].toInt());
	b.push_back(false);
	break;
      case 3:
	s.push_back(ls[0]);
	i.push_back(ls[1].toInt());
	b.push_back(ls[2].compare(cTruth,::Qt::CaseInsensitive)==0);
	break;
      default:
	break;
      }
    }
    free(buff);
  }
}

///////////////////
void Preferences::read (QList<QString>& s,
			QList<int>&     i, 
			QList<bool>&    b,
			const char*     bTruth,
			QList<bool>&    k,
			const char*     kTruth)
{
  if (_f) {
    char *buff = (char *)malloc(NODE_BUFF_SIZE);  // use malloc w/ getline
    if (buff == (char *)NULL) {
      printf("%s: malloc(%d) failed, errno=%d\n", __PRETTY_FUNCTION__, NODE_BUFF_SIZE, errno);
      return;
    }

    char* lptr=buff;
    size_t linesz = NODE_BUFF_SIZE;         // initialize for getline
    while(getline(&lptr,&linesz,_f)!=-1) {
      QString p(lptr);
      p.chop(1);  // remove new-line
      p.replace('\t','\n');        

      QStringList ls = p.split(';');

      switch (ls.size()) {
      case 1:
	s.push_back(ls[0]);
	i.push_back(0);
	b.push_back(false);
        k.push_back(false);
	break;
      case 2:
	s.push_back(ls[0]);
	i.push_back(ls[1].toInt());
	b.push_back(false);
	k.push_back(false);
	break;
      case 3:
	s.push_back(ls[0]);
	i.push_back(ls[1].toInt());
	b.push_back(ls[2].compare(bTruth,::Qt::CaseInsensitive)==0);
	k.push_back(true);
        break;
      case 4:
	s.push_back(ls[0]);
	i.push_back(ls[1].toInt());
	b.push_back(ls[2].compare(bTruth,::Qt::CaseInsensitive)==0);
        k.push_back(ls[3].compare(kTruth,::Qt::CaseInsensitive)==0);
	break;
      default:
	break;
      }
    }
    free(buff);
  }
}
//////////////////

void Preferences::write(const QString& s)
{ 
  std::ostringstream o;
  o << qPrintable(s) << "\n";
  _write(o.str()); 
}

void Preferences::write(const QString& s,
			const char*    b)
{ 
  std::ostringstream o;
  o << qPrintable(s) << ";" << b << "\n";
  _write(o.str()); 
}

void Preferences::write(const QString& s, 
			int            i,
			const char*    b)
{ 
  std::ostringstream o;
  o << qPrintable(s) << ";" << i << ";" << b << "\n";
  _write(o.str()); 
}

////////////////////
void Preferences::write(const QString& s, 
			int            i,
			const char*    b,
			const char*    k)
{ 
  std::ostringstream o;
  o << qPrintable(s) << ";" << i << ";" << b << ";" << k << "\n";
  _write(o.str()); 
}
//////////////

void Preferences::_write(const std::string& line)
{
  if (_f) {
    fprintf(_f, line.c_str());
    _task->call(new NfsWrite(*this,line));
  }
}

void Preferences::_nfsOpen (const char* path)
{
  _nfs = fopen(path,"w");
}

void Preferences::_nfsWrite(const char* line)
{
  if (_nfs)
    fprintf(_nfs, line);
}

void Preferences::_nfsClose()
{
  fclose(_nfs);
  _nfs = 0;
}
