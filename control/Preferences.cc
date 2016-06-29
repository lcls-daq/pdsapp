#include "pdsapp/control/Preferences.hh"

#include <QtCore/QStringList>

#include <sstream>
#include <string>

#include <stdlib.h>
#include <errno.h>

using namespace Pds;

static const unsigned NODE_BUFF_SIZE=256;

Preferences::Preferences(const char* title,
                         unsigned    platform,
                         const char* mode)
{
  std::stringstream o;
  char* home = getenv("HOME");
  if (home)
    o << home << '/';
  o << "." << title << " for platform " << platform;

  _f = fopen(o.str().c_str(),mode);
  if (_f) {
      //printf("Opened %s in %s mode %p\n",o.str().c_str(),mode, this);
  }
  else {
    std::string msg("Failed to open ");
    msg += o.str();
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
  if (_f)
    fprintf(_f,"%s\n",qPrintable(s));
}

void Preferences::write(const QString& s,
			const char*    b)
{
  if (_f)
    fprintf(_f,"%s;%s\n",qPrintable(s),b);
}

void Preferences::write(const QString& s, 
			int            i,
			const char*    b)
{
  if (_f)
    fprintf(_f,"%s;%d;%s\n",qPrintable(s),i,b);
}

////////////////////
void Preferences::write(const QString& s, 
			int            i,
			const char*    b,
			const char*    k)
{
  if (_f)
    fprintf(_f,"%s;%d;%s;%s\n",qPrintable(s),i,b,k);
}
//////////////
