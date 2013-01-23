#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/PdsDefs.hh"
#include "pds/config/CfgPath.hh"

#include <sys/stat.h>
#include <fstream>
using std::ifstream;
using std::ofstream;
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <glob.h>
#include <libgen.h>

#define DBUG

namespace Pds_ConfigDb {
  class TimeProfile {
  public:
    TimeProfile() : _n(0) { clock_gettime(CLOCK_REALTIME,&_tp); }
    ~TimeProfile() {
      printf(" == NFS access periods ==\n");
      for(unsigned i=0; i<_n; i++)
	printf("%f%c",_times[i],(i%10)==9 ? '\n':',');
      printf("\n");
    }
    void interval(const char* s1, const char* s2) {
      timespec tp;
      clock_gettime(CLOCK_REALTIME,&tp);
      double dt = double(tp.tv_sec - _tp.tv_sec)+1.e-9*(double(tp.tv_nsec)-double(_tp.tv_nsec));
      _tp = tp;

      const double threshold = 0.1;
      if (dt > threshold)
	printf("%s::%s = %f seconds\n",s1,s2,dt);

      if (_n<MAX_N)
	_times[_n++] = dt;
    }
    void interval(const std::string s1, const char* s2) { interval(s1.c_str(),s2); }
  private:
    timespec _tp;
    enum { MAX_N=100 };
    double   _times[MAX_N];
    unsigned _n;
  };
};

using namespace Pds_ConfigDb;

const mode_t _fmode = S_IROTH | S_IXOTH | S_IRGRP | S_IXGRP | S_IRWXU;

Experiment::Experiment(const Path& path) :
  _path(path)
{
}

void Experiment::read()
{
  const int line_size=1024;
  char buff[line_size];

  string path = _path.expt();
  cout << "Reading table from path " << path << endl;
  _table = Table(path);
  _devices.clear();

  path = _path.devices();
  ifstream f(path.c_str());
  while(f.good()) {
    unsigned sz=line_size;
    f.getline(buff,sz);
    char* p = strtok(buff,"\t");
    if (!p) break;
    string name(p);
    path = _path.device(name);
    list<DeviceEntry> args;
    while( (p=strtok(NULL,"\t")) )
      args.push_back(DeviceEntry(string(p)));
    _devices.push_back(Device(path,name,args));
  }
}

void Experiment::write() const
{
  _table.write(_path.expt());
  string fp = _path.devices();
  ofstream f(fp.c_str());
  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++) {
    f << iter->name();
    const list<DeviceEntry>& slist = iter->src_list();
    for(list<DeviceEntry>::const_iterator siter=slist.begin(); siter!=slist.end(); siter++) {
      f << "\t" << siter->id();
    }
    f << std::endl;
  }

  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++)
    iter->table().write(_path.device(iter->name()));
}

Experiment* Experiment::branch(const string& p) const
{
  //  Create the directory structure
  Path newpath(p);
  newpath.create();

  //  Create the device dependent structures
  Experiment* newdb = new Experiment(newpath);
  for(list<Device>::const_iterator it=devices().begin(); it!=devices().end(); it++) {
    newdb->add_device((*it).name(),(*it).src_list());
    const list<TableEntry>& t = (*it).table().entries();
    //  Load the device data files
    for(list<TableEntry>::const_iterator ait=t.begin(); ait!=t.end(); ait++) {
      for(list<FileEntry>::const_iterator fit=(*ait).entries().begin(); fit!=(*ait).entries().end(); fit++) {
  Pds_ConfigDb::UTypeName utype((*fit).name());
  string file_path = (*it).xtcpath(_path.base(), utype, (*fit).entry());
  newdb->import_data((*it).name(),
         utype,
         file_path,
         string(""));
      }
    }
  }
  delete newdb;

  //  Load the device and alias tables
  newdb = new Experiment(_path);
  newdb->read();
  newdb->_path = newpath;
  newdb->write();

  newdb->update_keys();
  return newdb;
}

Device* Experiment::device(const string& name)
{
  for(list<Device>::iterator iter=_devices.begin(); iter!=_devices.end(); iter++)
    if (iter->name() == name)
      return &(*iter);
  return 0;
}

void Experiment::add_device(const string& name,
          const list<DeviceEntry>& slist)
{
  cout << "Adding device " << name << endl;
  for(list<DeviceEntry>::const_iterator iter = slist.begin(); iter!=slist.end(); iter++)
    cout << ' ' << iter->id();
  cout << endl;

  Device device("",name,slist);
  _devices.push_back(device);
  //  mode_t mode = S_IRWXU | S_IRWXG;
  mode_t mode = _fmode;
  mkdir(device.keypath(_path.base(),"").c_str(),mode);
  cout << "Added dir " << device.keypath(_path.base(),"") << endl;
}

void Experiment::remove_device(const Device& device)
{
  cout << "Removing device " << device.name() << endl;
  _devices.remove(device);
}

void Experiment::import_data(const string& device,
           const UTypeName& type,
           const string& file,
           const string& desc)
{
  if (PdsDefs::typeId(type)==0) {
    cerr << type << " not registered as valid configuration data type" << endl;
    return;
  }

  mode_t mode = _fmode;
  const char* base = basename(const_cast<char*>(file.c_str()));
  string dst = _path.data_path(device,type)+"/"+base;
  struct stat64 s;
  if (!stat64(dst.c_str(),&s)) {
    cerr << dst << " already exists." << endl
   << "Rename the source file and try again." << endl;
    return;
  }
  const char* dir = dirname(const_cast<char*>(dst.c_str()));
  if (stat64(dir,&s)) {
    mkdir(dir,mode);
  }

  char buff[128];
  sprintf(buff,"cp %s %s",file.c_str(),dst.c_str());
  system(buff);

  dst = _path.desc_path(device,type)+"/"+base;
  dir = dirname(const_cast<char*>(dst.c_str()));
  if (stat64(dir,&s)) {
    mkdir(dir,mode);
  }
  ofstream f(dst.c_str());
  f << desc << std::endl;
}

bool Experiment::update_key(const TableEntry& entry)
{
#ifdef DBUG
  TimeProfile profile;
#endif
  //
  //  First, check if each device's configuration is valid in the db.
  //  This means that the device's configuration is up-to-date (current datatype versions)
  //
  unsigned valid=0;
  for(list<FileEntry>::const_iterator iter=entry.entries().begin();
      iter != entry.entries().end(); iter++) {
    if (device(iter->name())->validate_key(iter->entry(),_path.base()))
      valid++;
    else
      printf("%s/%s not valid\n",iter->name().c_str(),iter->entry().c_str());
#ifdef DBUG
    profile.interval(iter->name(),"validate_key");
#endif
  }
  if (valid != entry.entries().size()) {
    cerr << "Cannot update " << entry.name() << '[' << entry.key() << ']' << endl;
    return false;
  }

  //
  //  Next, check if the devices' file representation are consistent with the db.
  //  If not, create them and indicate that a new top key is needed.
  //
  int changed=0;
  for(list<FileEntry>::const_iterator iter=entry.entries().begin();
      iter != entry.entries().end(); iter++)
    if (device(iter->name())->update_key(iter->entry(),_path.base())) {
      changed++;
      //      printf("%s/%s changed\n",iter->name().c_str(),iter->entry().c_str());

#ifdef DBUG
      profile.interval(iter->name(),"update_key");
#endif
    }

  mode_t mode = _fmode;
  //  mode_t mode = S_IRWXU | S_IRWXG;

  //
  //  Check that the top key file representation is valid.
  //
  const int line_size=128;
  char buff[line_size];
  string kpath = _path.key_path(entry.key());
  unsigned invalid = entry.entries().size();   // number of expected devices

  glob_t g;
  string gpath = kpath + "/[0-9,a-f]*";
  glob(gpath.c_str(),0,0,&g);
  if (invalid < g.gl_pathc) invalid = g.gl_pathc; // number of devices included
  globfree(&g);

#ifdef DBUG
  printf("Check %d/%d devices\n",invalid,entry.entries().size());

  profile.interval("glob",gpath.c_str());
#endif

  struct stat64 s;
  if (!stat64(kpath.c_str(),&s)) {   // the key exists
#ifdef DBUG
    profile.interval("stat",kpath.c_str());
#endif
    //  check each device
    for(list<FileEntry>::const_iterator iter=entry.entries().begin();
  iter !=entry.entries().end(); iter++) {
      const Device* d = device(iter->name());
      string dpath = "../" + iter->name() + "/" + d->table().get_top_entry(iter->entry())->key();
      const list<DeviceEntry>& slist = d->src_list();
      //  Check each <src> entry for the device
      for(list<DeviceEntry>::const_iterator siter = slist.begin();
    siter != slist.end(); siter++) {
  string spath = kpath + "/" + Pds::CfgPath::src_key(*siter);
  if (!stat64(spath.c_str(),&s)) {
#ifdef DBUG
          profile.interval("stat",spath.c_str());
#endif
    int sz=readlink(spath.c_str(),buff,line_size);
#ifdef DBUG
          profile.interval("readlink",spath.c_str());
#endif
    if (sz>0) {
      buff[sz] = 0;
      if (!strcmp(buff,dpath.c_str()))
        --invalid;
#ifdef DBUG
      else
        printf("link(%s) incorrect %s != %s\n",
         spath.c_str(),
         buff,
         dpath.c_str());
#endif
    }
  }
#ifdef DBUG
        else
          profile.interval("stat",spath.c_str());
#endif
      }
    }
  }
  else {
#ifdef DBUG
    profile.interval("stat",kpath.c_str());
#endif
    printf("The top key file (%s) doesn't yet exist\n", kpath.c_str());
  }

#ifdef DBUG
  printf("Still %d devices invalid\n",invalid);
#endif

  if (invalid) {
    sprintf(buff,"%08x",next_key());
    string key(buff);
    TableEntry te = TableEntry(entry.name(),key,entry.entries());
    kpath = _path.key_path(key);
    mkdir(kpath.c_str(),mode);
#ifdef DBUG
    profile.interval("mkdir",kpath.c_str());
#endif
    for(list<FileEntry>::const_iterator iter=te.entries().begin();
  iter!=te.entries().end(); iter++) {
      Device* d = device(iter->name());
      string dpath = "../" + iter->name() + "/" + d->table().get_top_entry(iter->entry())->key();
      const list<DeviceEntry>& slist = d->src_list();
      for(list<DeviceEntry>::const_iterator siter=slist.begin();
    siter != slist.end(); siter++) {
  string spath = kpath + "/" + Pds::CfgPath::src_key(*siter);
  symlink(dpath.c_str(),spath.c_str());
#ifdef DBUG
        profile.interval("symlink",dpath.c_str());
#endif
      }
    }
    _table.set_top_entry(te);
    cout << "Assigned new key " << te.key() << " to " << te.name() << endl;

    string spath = kpath + "/Info";
    ofstream of(spath.c_str());
    of << te.name() << endl;
  }

  return invalid;
}

unsigned Experiment::next_key() const
{
  string kpath = _path.key_path(string("[0-9]*"));
  glob_t g;
  glob(kpath.c_str(),0,0,&g);
  unsigned key = unsigned(g.gl_pathc);
  globfree(&g);
  return key;
}

void Experiment::update_keys()
{
  for(list<TableEntry>::iterator iter=_table.entries().begin();
      iter!=_table.entries().end(); iter++)
    update_key(*iter);
}

int Experiment::current_key(const string& alias) const
{
  const TableEntry* e = _table.get_top_entry(alias.c_str());
  return e ? strtoul(e->key().c_str(),NULL,16) : -1;
}

void Experiment::dump() const
{
  cout << "Experiment " << endl;
  _table.dump(_path.expt());
  cout << endl << _path.devices() << endl;
  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++) {
    cout << iter->name();
    for(list<DeviceEntry>::const_iterator diter=iter->src_list().begin();
  diter!=iter->src_list().end(); diter++)
      cout << '\t' << diter->id();
    cout << endl;
  }
  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++) {
    iter->table().dump(_path.device(iter->name()));
  }
}
