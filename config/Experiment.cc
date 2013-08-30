#include "pdsapp/config/Experiment.hh"
#include "pdsapp/config/PdsDefs.hh"
#include "pdsapp/config/XtcTable.hh"
#include "pdsapp/config/XML.hh"
#include "pds/config/CfgPath.hh"

#include <sys/stat.h>
#include <fstream>
using std::ifstream;
using std::ofstream;
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <string>

#include <glob.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

//#define DBUG

static double _log_threshold = -1;

static int _symlink(const char* dst, const char* src) {
  int r = symlink(dst,src);
  if (r<0) {
    char buff[256];
    sprintf(buff,"symlink %s -> %s",src,dst);
    perror(buff);
  }
  return r;
}

static void _handle_no_lock(const char* s)
{
  std::string _no_lock_exception(s);
  printf("_handle_no_lock:%s\n",s);
  throw _no_lock_exception;
}

namespace Pds_ConfigDb {
  class TimeProfile {
  public:
    TimeProfile() : _n(0) { clock_gettime(CLOCK_REALTIME,&_tp); }
    ~TimeProfile() {
      if (_log_threshold>=0) {
	printf(" == NFS access periods ==\n");
	for(unsigned i=0; i<_n; i++) {
	  printf(",%f",_times[i]);
	  if ((i%10)==9) printf("\n");
	}
	printf("\n == \n");
      }
    }
    void interval(const char* s1, const char* s2) {
      if (_log_threshold >=0) {
	timespec tp;
	clock_gettime(CLOCK_REALTIME,&tp);
	double dt = double(tp.tv_sec - _tp.tv_sec)+1.e-9*(double(tp.tv_nsec)-double(_tp.tv_nsec));
	_tp = tp;

	if (dt > _log_threshold)
	  printf("NFS_alert-%s::%s = %f seconds\n",s1,s2,dt);

	if (_n<MAX_N)
	  _times[_n++] = dt;
      }
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

Experiment::Experiment(const Path& path, Option lock) :
  _path(path),
  _lock(lock)
{
  string fname = _path.expt() + ".xml";
  _f = fopen(fname.c_str(),_lock==Lock ? "r+":"r");
  if (!_f) {
    perror("fopen expt.xml");
  }
  else if (_lock==Lock) {  // lock it
    struct flock flk;
    flk.l_type   = F_WRLCK;
    flk.l_whence = SEEK_SET;
    flk.l_start  = 0;
    flk.l_len    = 0;
    if (fcntl(fileno(_f), F_SETLK, &flk)<0) {
      perror("Experiment fcntl F_SETLK");
      if (fcntl(fileno(_f), F_GETLK, &flk)<0) 
        perror("Experiment fcntl F_GETLK");
      else if (flk.l_type == F_UNLCK)
        printf("F_GETLK: lock is available\n");
      else {
        char buff[256];
        if (flk.l_pid == 0)
          _handle_no_lock("Lock held by remote process");
        else {
          sprintf(buff,"Lock held by process %d\n", flk.l_pid);
          _handle_no_lock(buff);
        }
      }
    }
  }
}

Experiment::~Experiment()
{
  if (_f)
    fclose(_f);
}

void Experiment::load(const char*& p)
{
  _devices.clear();

  XML_iterate_open(p,tag)
    if      (tag.name == "_table") {
      _table = Table();
      _table.load(p); 
    }
    else if (tag.name == "_devices") {
      Device d;
      d.load(p);
      _devices.push_back(d);
    }
    else if (tag.name == "_time_db")
      _time_db  = XML::IO::extract_i(p);
    else if (tag.name == "_time_key")
      _time_key = XML::IO::extract_i(p);
  XML_iterate_close(Experiment,tag);
}

void Experiment::save(char*& p) const
{
  XML_insert(p, "Table"   , "_table", _table.save(p));
  XML_insert(p, "time_t"  , "_time_db" , XML::IO::insert(p, (unsigned)_time_db));
  XML_insert(p, "time_t"  , "_time_key", XML::IO::insert(p, (unsigned)_time_key));
  for(list<Device>::const_iterator it=_devices.begin(); it!=_devices.end(); it++) {
    XML_insert(p, "Device", "_devices", it->save(p) );
  }
}

void Experiment::read() 
{
  FILE* f = _f;
  if (!f) {
    perror("fopen expt.xml");
    read_file();
  }
  else {
    fseek(f,0,SEEK_SET);
    struct stat64 s;
    if (fstat64(fileno(f),&s))
      perror("fstat64 expt.xml");
    else {
      const char* TRAILER = "</Document>";
      char* buff = new char[s.st_size+strlen(TRAILER)+1];
      if (fread(buff, 1, s.st_size, f) != size_t(s.st_size))
        perror("fread expt.xml");
      else {
        strcpy(buff+s.st_size,TRAILER);
        const char* p = buff;
        load(p);
      }
      delete[] buff;
    }
  }
}

void Experiment::write() const
{
  time_t time_db = _time_db;

  FILE* f = _f;
  if (!f || _lock==NoLock)
    _handle_no_lock("write");
  else {
    fseek(f,0,SEEK_SET);
    const int MAX_SIZE = 0x100000;
    char* buff = new char[MAX_SIZE];
    char* p = buff;

    _time_db = time(0);
    save(p);

    if (p > buff+MAX_SIZE) {
      perror("save overwrites array");
      _time_db = time_db;
    } 
    else 
      fwrite(buff, 1, p-buff, f);

    delete[] buff;
  }
}

void Experiment::read_file()
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

  _table._next_key = next_key_file();

  struct stat64 s;
  if (!stat64(path.c_str(),&s))
    _time_db = s.st_mtime;

  if (!stat64(_path.key_path(_table._next_key-1).c_str(),&s))
    _time_key = s.st_mtime;
}

void Experiment::write_file() const
{
  if (_lock==NoLock) 
    _handle_no_lock("write_file");

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
  newdb->update_keys();
  newdb->write();

  return newdb;
}

Device* Experiment::device(const string& name)
{
  if (_lock==NoLock)
    _handle_no_lock("device");

  for(list<Device>::iterator iter=_devices.begin(); iter!=_devices.end(); iter++)
    if (iter->name() == name)
      return &(*iter);
  return 0;
}

const Device* Experiment::device(const string& name) const
{
  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++)
    if (iter->name() == name)
      return &(*iter);
  return 0;
}

void Experiment::add_device(const string& name,
                            const list<DeviceEntry>& slist)
{
  if (_lock==NoLock) 
    _handle_no_lock("add_device");

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
  if (_lock==NoLock) 
    _handle_no_lock("remove_device");

  for(list<TableEntry>::iterator alias=_table.entries().begin();
      alias!=_table.entries().end(); alias++)
    for(list<FileEntry>::const_iterator iter=alias->entries().begin();
        iter != alias->entries().end(); iter++) {
      FileEntry e(*iter);
      if (e.name() == device.name()) {
        alias->remove(e);
        break;
      }
    }

  _devices.remove(device);
}

void Experiment::import_data(const string& device,
                             const UTypeName& type,
                             const string& file,
                             const string& desc)
{
  if (_lock==NoLock) 
    _handle_no_lock("import_data");

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

  char buff[256];
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

bool Experiment::update_key_file(const TableEntry& entry)
{
  if (_lock==NoLock) 
    _handle_no_lock("update_key_file");

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
    if (device(iter->name())->validate_key_file(iter->entry(),_path.base()))
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
    if (device(iter->name())->update_key_file(iter->entry(),_path.base())) {
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
  printf("Check %d/%d devices\n",invalid,int(entry.entries().size()));
 
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
    sprintf(buff,"%08x",next_key_file());
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
  _symlink(dpath.c_str(),spath.c_str());
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

unsigned Experiment::next_key_file() const
{
  string kpath = _path.key_path(string("[0-9]*"));
  glob_t g;
  glob(kpath.c_str(),0,0,&g);
  unsigned key = unsigned(g.gl_pathc);
  globfree(&g);
  return key;
}

unsigned Experiment::next_key() const { return _table._next_key; }

void Experiment::update_keys()
{
  if (_lock==NoLock) 
    _handle_no_lock("update_keys");

#ifdef DBUG
  printf("expt:update_keys %u\n",unsigned(_time_key));
#endif

  //  Flag all xtc files that have been modified since the last update
  XtcTable xtc(_path.base());
  xtc.update(_path,_time_key);

#ifdef DBUG
  xtc.dump();
#endif

  //  Generate device keys
  for(list<Device>::iterator it=_devices.begin(); it!=_devices.end(); it++)
    it->update_keys(_path,xtc,_time_key);

  //  Generate experiment keys
  for(list<TableEntry>::iterator iter=_table.entries().begin();
      iter!=_table.entries().end(); iter++) {
    bool changed = false;
    for(list<FileEntry>::const_iterator it=iter->entries().begin(); it!=iter->entries().end(); it++) {
      const Device* dev = device(it->name());
      if (!dev) {
        printf("No device %s found for key %s\n",it->name().c_str(),iter->name().c_str()); 
        continue;
      }
      
      const TableEntry* te = dev->table().get_top_entry(it->entry());
      if (!te) {
        printf("No alias %s/%s found for key %s\n",it->name().c_str(),it->entry().c_str(),iter->name().c_str()); 
        continue;
      }

      changed |= te->updated();
#ifdef DBUG
      if (te->updated())
        printf("expt:%s dev:%s:%s %s\n", iter->name().c_str(), it->name().c_str(), it->entry().c_str(),
               te->updated() ? "changed" : "unchanged");
#endif
    }

    if (changed) {
      // make the key
      mode_t mode = _fmode;
      string kpath = _path.key_path(_table._next_key);
#ifdef DBUG
      printf("mkdir %s\n",kpath.c_str());
#endif
      mkdir(kpath.c_str(),mode);
      iter->update(_table._next_key++);

      for(list<FileEntry>::const_iterator it=iter->entries().begin(); it!=iter->entries().end(); it++) {
        const Device* dev = device(it->name());
        if (!dev) continue;
        const TableEntry* te = dev->table().get_top_entry(it->entry());
        if (!te) continue;

        ostringstream o;
        o << "../" << it->name() << "/" << te->key();

        for(list<DeviceEntry>::const_iterator src=dev->src_list().begin(); src!=dev->src_list().end(); src++) {
          ostringstream t;
          t << kpath << "/" << src->path();
#ifdef DBUG
          printf("symlink %s -> %s\n",t.str().c_str(), o.str().c_str());
#endif
          _symlink(o.str().c_str(),t.str().c_str());
        }
      }
    }
  }

  _time_key = time(0);
  //  write();

  xtc.write(_path.base());
}

int Experiment::current_key(const string& alias) const
{
  const TableEntry* e = _table.get_top_entry(alias.c_str());
  return e ? strtoul(e->key().c_str(),NULL,16) : -1;
}

//
//  Clone an existing key
//
unsigned Experiment::clone(const string& alias)
{
  if (_lock==NoLock)
    _handle_no_lock("clone");

  TableEntry* iter = const_cast<TableEntry*>(_table.get_top_entry(alias));
  if (!iter)
    throw std::string("alias not found");

  unsigned key = _table._next_key++;
  write();  // save current state

  // make the key
  mode_t mode = _fmode;
  string kpath = _path.key_path(key);
#ifdef DBUG
  printf("mkdir %s\n",kpath.c_str());
#endif
  mkdir(kpath.c_str(),mode);
  iter->update(key);

  for(list<FileEntry>::const_iterator it=iter->entries().begin(); it!=iter->entries().end(); it++) {
    const Device* dev = device(it->name());
    if (!dev) continue;
    const TableEntry* te = dev->table().get_top_entry(it->entry());
    if (!te) continue;
    
    ostringstream o;
    o << "../" << it->name() << "/" << te->key();
    
    for(list<DeviceEntry>::const_iterator src=dev->src_list().begin(); src!=dev->src_list().end(); src++) {
      ostringstream t;
      t << kpath << "/" << src->path();
#ifdef DBUG
      printf("symlink %s -> %s\n",t.str().c_str(), o.str().c_str());
#endif
      _symlink(o.str().c_str(),t.str().c_str());
    }
  }

  read();  // erase changes in memory

  return key;
}

void Experiment::substitute(unsigned           key, 
                            const string&      devname,
                            const Pds::TypeId& type, 
                            const char*        payload,
                            size_t             sz) const
{
  if (_lock==NoLock) 
    _handle_no_lock("substitute");

  const Device* dev = device(devname);
  if (!dev) return;

  for(std::list<DeviceEntry>::const_iterator it=dev->src_list().begin(); it!=dev->src_list().end(); it++) {
    substitute(key, *it, type, payload, sz);
  }
}

void Experiment::substitute(unsigned           key, 
                            const Pds::Src&    src,
                            const Pds::TypeId& type, 
                            const char*        payload,
                            size_t             sz) const
{
  if (_lock==NoLock)
    _handle_no_lock("substitute_");

  string kpath = _path.key_path(key);

  DeviceEntry entry(src);
  ostringstream t;
  t << kpath << '/' << entry.path();
    
  size_t buflen = 128;
  char buff[128];
  int len;
  if ((len=readlink(t.str().c_str(),buff,buflen))>=0) {
      
    // remove the link
#ifdef DBUG
    printf("unlink %s\n",t.str().c_str());
#endif
    if (unlink(t.str().c_str())<0) {
      perror(t.str().c_str());
      return;
    }
      
    // create the directory
#ifdef DBUG
    printf("mkdir %s\n",t.str().c_str());
#endif
    if (mkdir(t.str().c_str(), _fmode)<0) {
      perror(t.str().c_str());
      return;
    }
      
    ostringstream s;
    buff[len] = 0;
    s << kpath << '/' << buff << "/*";
      
    glob_t gl;
    glob(s.str().c_str(), GLOB_NOSORT, NULL, &gl);
    for(unsigned i=0; i<gl.gl_pathc; i++) {
      ostringstream q;
      q << t.str() << '/' << basename(gl.gl_pathv[i]);
#ifdef DBUG
      printf("symlink %s -> %s\n",q.str().c_str(), gl.gl_pathv[i]);
#endif
      if (_symlink(gl.gl_pathv[i], q.str().c_str())<0)
        return;
    }
    globfree(&gl);
  }

  struct stat64 s;
  if (stat64(t.str().c_str(),&s)<0) {
    // create the directory
#ifdef DBUG
    printf("mkdir %s\n",t.str().c_str());
#endif
    if (mkdir(t.str().c_str(), _fmode)<0) {
      perror(t.str().c_str());
      return;
    }
  }
    
  t << '/' << std::hex << setw(8) << setfill('0') << type.value();
  if ((len=readlink(t.str().c_str(),buff,buflen))>=0) {
    // remove the link
#ifdef DBUG
    printf("unlink %s\n",t.str().c_str());
#endif
    if (unlink(t.str().c_str())<0) {
      perror(buff);
      return;
    }
  }

  FILE* f = fopen(t.str().c_str(),"w");
#ifdef DBUG
  printf("fopen %s\n",t.str().c_str());
#endif  
  if (!f) {
    perror(t.str().c_str());
    return;
  }
#ifdef DBUG
  printf("fwrite %p  sz 0x%x\n",payload,sz);
#endif
  fwrite(payload, sz, 1, f);
  fclose(f);
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

void Experiment::log_threshold(double v) { _log_threshold=v; }
