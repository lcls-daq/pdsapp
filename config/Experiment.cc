#include "pdsapp/config/Experiment.hh"

#include <sys/stat.h>
#include <fstream>
using std::ifstream;
using std::ofstream;

using namespace Pds_ConfigDb;

Experiment::Experiment(const string& path) :
  _path(path)
{
}

bool Experiment::is_valid() const
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

void Experiment::create()
{
  mode_t mode = S_IRWXU | S_IRWXG;
  mkdir(_path.c_str(),mode);
  char buff[128];
  sprintf(buff,"%s/db"  ,_path.c_str());  mkdir(buff,mode);
  sprintf(buff,"%s/desc",_path.c_str());  mkdir(buff,mode);
  sprintf(buff,"%s/keys",_path.c_str());  mkdir(buff,mode);
  sprintf(buff,"%s/xtc" ,_path.c_str());  mkdir(buff,mode);
}

void Experiment::read()
{
  const int line_size=128;
  char buff[line_size];
  string path;
  path = _path + "/db/expt";

  printf("Reading table from path %s\n",path.c_str());

  _table = Table(path);
  _devices.clear();
  path = _path + "/db/devices";
  ifstream f(path.c_str());
  while(f.good()) {
    unsigned sz=line_size;
    f.getline(buff,sz);
    char* p = strtok(buff,"\t");
    if (!p) break;
    string name(p);
    path = _path + "/db/devices." + name;
    list<DeviceEntry> args;
    while( (p=strtok(NULL,"\t")) )
      args.push_back(DeviceEntry(string(p)));
    _devices.push_back(Device(path,name,args));
  }
}
        
void Experiment::write() const
{
  _table.write(_path+"/db/expt");
  string fp = _path + "/db/devices";
  ofstream f(fp.c_str());
  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++) {
    f << iter->name();
    const list<DeviceEntry>& slist = iter->src_list();
    for(list<DeviceEntry>::const_iterator siter=slist.begin(); siter!=slist.end(); siter++)
      f << "\t" << siter->id();
    f << std::endl;
  }

  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++)
    iter->table().write(_path+"/db/devices."+iter->name());
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
  printf("Adding device %s :",name.c_str());
  for(list<DeviceEntry>::const_iterator iter = slist.begin(); iter!=slist.end(); iter++)
    printf(" %s",iter->id().c_str());
  printf("\n");

  Device device("",name,slist);
  _devices.push_back(device);
  mode_t mode = S_IRWXU | S_IRWXG;
  mkdir(device.keypath(_path,"").c_str(),mode);
  printf("Added dir %s\n",device.keypath(_path,"").c_str());
}

string Experiment::key_path(const string& device, const string& key) const
{
  return _path+"/xtc/"+device+"/"+key;
}

string Experiment::data_path(const string& device,
			     const string& type) const
{
  return _path + "/xtc/"+type;
}

string Experiment::desc_path(const string& device,
			     const string& type) const
{
  return _path + "/desc/" + type;
}

#include <glob.h>
#include <libgen.h>

list<string> Experiment::xtc_files(const string& device,
				   const string& type) const
{
  list<string> l;
  glob_t g;
  string path = data_path(device,type)+"/*";
  glob(path.c_str(), 0, 0, &g);
  for(unsigned i=0; i<g.gl_pathc; i++)
    l.push_back(string(basename(g.gl_pathv[i])));
  globfree(&g);
  return l;
}

void Experiment::import_data(const string& device,
			     const string& type,
			     const string& file,
			     const string& desc)
{
  mode_t mode = S_IRWXU | S_IRWXG;
  const char* base = basename(const_cast<char*>(file.c_str()));
  string dst = data_path(device,type)+"/"+base;
  struct stat s;
  if (!stat(dst.c_str(),&s)) {
    printf("%s already exists.\nRename the source file and try again.\n",dst.c_str());
    return;
  }
  const char* dir = dirname(const_cast<char*>(dst.c_str()));
  if (stat(dir,&s)) {
    mkdir(dir,mode);
  }

  char buff[128];
  sprintf(buff,"cp %s %s",file.c_str(),dst.c_str());
  system(buff);

  dst = desc_path(device,type)+"/"+base;
  dir = dirname(const_cast<char*>(dst.c_str()));
  if (stat(dir,&s)) {
    mkdir(dir,mode);
  }
  ofstream f(dst.c_str());
  f << desc << std::endl;
}

bool Experiment::validate_key(const TableEntry& entry)
{
  mode_t mode = S_IRWXU | S_IRWXG;
  int changed=0;
  for(list<FileEntry>::const_iterator iter=entry.entries().begin();
      iter != entry.entries().end(); iter++)
    changed += device(iter->name())->validate_key(iter->entry(),_path);

  const int line_size=128;
  char buff[line_size];
  string kpath = _path + "/keys/" + entry.key();
  int invalid = entry.entries().size();
  struct stat s;
  if (!stat(kpath.c_str(),&s)) {
    for(list<FileEntry>::const_iterator iter=entry.entries().begin();
	iter !=entry.entries().end(); iter++) {
      const Device* d = device(iter->name());
      string dpath = string("../") + iter->name() + "/" + d->table().get_top_entry(iter->entry())->key();
      const list<DeviceEntry>& slist = d->src_list();
      int invalid_device = slist.size();
      for(list<DeviceEntry>::const_iterator siter = slist.begin();
	  siter != slist.end(); siter++) {
	string spath = kpath + "/" + siter->id();
	unsigned sz=line_size;
	if (!stat(spath.c_str(),&s) &&
	    readlink(spath.c_str(),buff,sz) &&
	    string(buff)==dpath)
	  --invalid_device;
      }
      if (invalid_device==0)
	--invalid;
    }
  }
  
  if (invalid) {
    string p = _path + "/keys/[0-9]*";
    glob_t g;
    glob(p.c_str(),0,0,&g);
    sprintf(buff,"%08x",g.gl_pathc);
    globfree(&g);
    string key(buff);
    TableEntry te = TableEntry(entry.name(),key,entry.entries());
    p = _path + "/keys/" + key;
    mkdir(p.c_str(),mode);
    for(list<FileEntry>::const_iterator iter=te.entries().begin();
	iter!=te.entries().end(); iter++) {
      Device* d = device(iter->name());
      string dpath = "../" + iter->name() + "/" + d->table().get_top_entry(iter->entry())->key();
      for(list<DeviceEntry>::const_iterator diter=d->src_list().begin();
	  diter!=d->src_list().end(); diter++) {
	string spath = p + "/" + diter->id();
	symlink(dpath.c_str(),spath.c_str());
      }
    }
    _table.set_top_entry(te);
    printf("Assigned new key %s to %s\n",te.key().c_str(),te.name().c_str());
  }

  return invalid;
}

void Experiment::update_keys()
{
  for(list<TableEntry>::iterator iter=_table.entries().begin();
      iter!=_table.entries().end(); iter++)
    validate_key(*iter);
}

void Experiment::dump() const
{  
  printf("Experiment %s\n",_path.c_str());
  _table.dump(_path+"/db/expt");
  printf("\n%s/db/devices\n",_path.c_str());
  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++) {
    printf("%s",iter->name().c_str());
    for(list<DeviceEntry>::const_iterator diter=iter->src_list().begin();
	diter!=iter->src_list().end(); diter++)
      printf("\t%s",diter->id().c_str());
    printf("\n");
  }
  for(list<Device>::const_iterator iter=_devices.begin(); iter!=_devices.end(); iter++) {
    iter->table().dump(_path+"/db/devices."+iter->name());
  }
}
