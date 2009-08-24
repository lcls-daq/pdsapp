#include "pdsapp/config/Path.hh"
#include "pdsapp/config/PdsDefs.hh"

#include <sys/stat.h>

static const mode_t _fmode = S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP | S_IRWXU;

using namespace Pds_ConfigDb;

Path::Path(const string& path) :
  _path(path)
{
}

Path::~Path()
{
}

string Path::expt   () const { return _path + "/db/expt"; }
string Path::devices() const { return _path + "/db/devices"; }
string Path::device (const string& dev) const { return _path + "/db/devices." + dev; }

void Path::create()
{
  //  mode_t mode = S_IRWXU | S_IRWXG;
  mode_t mode = _fmode;
  mkdir(_path.c_str(),mode);
  char buff[128];
  sprintf(buff,"%s/db"  ,_path.c_str());  mkdir(buff,mode);
  sprintf(buff,"%s/desc",_path.c_str());  mkdir(buff,mode);
  sprintf(buff,"%s/keys",_path.c_str());  mkdir(buff,mode);
  sprintf(buff,"%s/xtc" ,_path.c_str());  mkdir(buff,mode);
}

bool Path::is_valid() const
{
  struct stat s;
  if (stat(_path.c_str(),&s)) return false;

  char buff[128];
  sprintf(buff,"%s/db"  ,_path.c_str());  if (stat(buff,&s)) return false;
  sprintf(buff,"%s/desc",_path.c_str());  if (stat(buff,&s)) return false;
  sprintf(buff,"%s/keys",_path.c_str());  if (stat(buff,&s)) return false;
  sprintf(buff,"%s/xtc" ,_path.c_str());  if (stat(buff,&s)) return false;
  return true;
}

string Path::key_path(const string& device, const string& key) const
{ return _path+"/xtc/"+device+"/"+key; }

string Path::key_path(const string& key) const
{ return _path+"/keys/"+key; }

string Path::data_path(const string& device,
		       const UTypeName& type) const
{
  string path = _path + "/xtc/" + PdsDefs::qtypeName(type);
  struct stat s;
  if (stat(path.c_str(),&s)) {
    //    mode_t mode = S_IRWXU | S_IRWXG;
    mode_t mode = _fmode;
    mkdir(path.c_str(),mode);
  }
  return path;
}

string Path::desc_path(const string& device,
		       const UTypeName& type) const
{
  string path = _path + "/desc/" + PdsDefs::qtypeName(type);
  struct stat s;
  if (stat(path.c_str(),&s)) {
    //    mode_t mode = S_IRWXU | S_IRWXG;
    mode_t mode = _fmode;
    mkdir(path.c_str(),mode);
  }
  return path;
}

#include <glob.h>
#include <libgen.h>

list<string> Path::xtc_files(const string& device,
			     const UTypeName& type) const
{
  list<string> l;
  glob_t g;
  string path = data_path(device,type)+"/*.xtc";
  glob(path.c_str(), 0, 0, &g);
  for(unsigned i=0; i<g.gl_pathc; i++)
    l.push_back(string(basename(g.gl_pathv[i])));
  globfree(&g);
  return l;
}

