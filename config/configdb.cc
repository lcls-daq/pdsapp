#include "Experiment.hh"
#include "pds/config/PdsDefs.hh"
#include "pds/config/DbClient.hh"
#include "pds/config/CfgClientNfs.hh"
#include "pds/confignfs/Path.hh"
#include "pds/configsql/DbClient.hh"
#include "pds/utility/Transition.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <getopt.h>

extern int optind;

using namespace Pds_ConfigDb;

// Commands
int create_db  (int argc, char** argv);
int copy_db    (int argc, char** argv);
int branch_db  (int argc, char** argv);
int update_db  (int argc, char** argv);
int fetch_xtc  (int argc, char** argv);
int truncate_db(int argc, char** argv);
int dump_db    (int argc, char** argv);

typedef int command_fcn(int,char**);

struct command {
  const char* key;
  const char* options;
  const char* descr;
  command_fcn* fcn;
};

static struct command _commands[] =
  { { "--create",
      "--db <path>",
      "Create a new database",
      create_db },
    { "--copy",
      "--idb <ipath> --odb <opath>",
      "Copy an existing database (ipath) to a new database(opath) including history",
      copy_db },
    { "--branch",
      "--idb <ipath> --odb <opath>",
      "Copy an existing database (ipath) to a new database(opath) excluding history",
      branch_db },
    { "--update-keys",
      "--db <path>",
      "Update the run keys",
      update_db },
    { "--fetch-xtc",
      "--db <path> --source <64b source> --typeid <32b type> --key <run key>",
      "Fetch the xtc",
      fetch_xtc },
    { "--truncate-db",
      "--db <path> [--key <from-to>] [--time <from-to>]",
      "Truncate/delete the history",
      truncate_db },
    { "--dump-db",
      "--db <path>",
      "Dump the keys",
      dump_db },
    { NULL, NULL, NULL, NULL }
  };

void print_help(const char* p)
{
  printf("Usage: %s [options]\n",p);
  for(unsigned i=0; _commands[i].options; i++)
    printf("%s %s: %s\n",_commands[i].key,_commands[i].options,_commands[i].descr);
}

static void _copy_xtc(DbClient&,DbClient&,const XtcEntry&);

static bool parseUInt  (const char* arg, unsigned& v, int base=0)
{
  char* endptr;
  v = strtoul(arg,&endptr,base);
  return *endptr==0;
}

int main(int argc, char** argv)
{
  if (argc >= 4) {
    string cmd(argv[1]);
    for(unsigned i=0; _commands[i].options; i++) {
      if (cmd == std::string(_commands[i].key)) {
        int v = _commands[i].fcn(argc-1,&argv[1]);
        if (v>=0) return v;
      }
    }
  }
  print_help(argv[0]);
  return 1;
}

//
//  Create an empty database
//
int create_db(int argc, char** argv)
{
  static struct option _options[] = { {"db", 1, 0, 0},
                                      { 0, 0, 0, 0 } };

  const char* path = 0;

  char c;
  while( (c=getopt_long(argc, argv, "", _options, NULL)) != -1 ) {
    switch(c) {
    case 0: path = optarg; break;
    default: return -1;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    return -1;
  }

  if (!path) return -1;

  try {
    Sql::DbClient::open(path);
    printf("SQL database from authorization file %s exists\n",path);
  } catch(...) {
    if (DbClient::open(path))
      printf("NFS database at %s exists\n",path);
    else {
      printf("Creating NFS database at %s\n",path);
      Pds_ConfigDb::Nfs::Path nfspath(path);
      nfspath.create();
    }
  }
  return 0;
}

//
//  Copy a database with its history
//
int copy_db(int argc, char** argv)
{
  static struct option _options[] = { {"idb", 1, 0, 0},
                                      {"odb", 1, 0, 1},
                                      { 0, 0, 0, 0 } };

  const char *ipath=0, *opath=0;

  char c;
  while( (c=getopt_long(argc, argv, "", _options, NULL)) != -1 ) {
    switch(c) {
    case 0: ipath = optarg; break;
    case 1: opath = optarg; break;
    default: return -1;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    return -1;
  }

  if (!ipath || !opath) return -1;

  DbClient* newdb = 0;
  try {
    newdb = Sql::DbClient::open(opath);
    printf("Opened SQL database from authorization file %s\n",opath);
  } catch(...) {
    if (DbClient::open(opath)) {
      printf("NFS database at %s exists.  Delete and try again\n",opath);
      delete newdb;
      return 0;
    }
    else {
      printf("Creating NFS database at %s\n",opath);
      Pds_ConfigDb::Nfs::Path nfspath(opath);
      nfspath.create();
      newdb = DbClient::open(opath);
    }
  }

  Experiment db(ipath, Pds_ConfigDb::Experiment::NoLock);
  std::list<ExptAlias>  alist = db.path().getExptAliases();
  std::list<DeviceType> dlist = db.path().getDevices();

  newdb->begin();
  newdb->setExptAliases(alist);
  newdb->setDevices    (dlist);
  newdb->commit();

  std::list<Key> klist = db.path().getKeys();
  for(std::list<Key>::iterator it=klist.begin();
      it!=klist.end(); it++) {
    std::list<KeyEntry> entries = db.path().getKey(it->key);
    for(std::list<KeyEntry>::iterator eit=entries.begin();
        eit!=entries.end(); eit++)
      if (newdb->getXTC(eit->xtc)<0)
        _copy_xtc(db.path(),*newdb,eit->xtc);

    newdb->begin();
    newdb->setKey(*it, entries);
    newdb->commit();
  }

  for(std::list<DeviceType>::iterator it=dlist.begin();
      it!=dlist.end(); it++)
    for(std::list<DeviceEntries>::iterator eit=it->entries.begin();
        eit!=it->entries.end(); eit++)
      for(std::list<XtcEntry>::iterator xit=eit->entries.begin();
          xit!=eit->entries.end(); xit++)
        _copy_xtc(db.path(),*newdb,*xit);

  newdb->begin();
  newdb->updateKeys();
  newdb->commit();

  delete newdb;
  return 0;
}

//
//  Copy a database without its history
//
int branch_db(int argc, char** argv)
{
  static struct option _options[] = { {"idb", 1, 0, 0},
                                      {"odb", 1, 0, 1},
                                      { 0, 0, 0, 0 } };

  const char *ipath=0, *opath=0;

  char c;
  while( (c=getopt_long(argc, argv, "", _options, NULL)) != -1 ) {
    switch(c) {
    case 0: ipath = optarg; break;
    case 1: opath = optarg; break;
    default: return -1;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    return -1;
  }

  if (!ipath || !opath) return -1;

  DbClient* newdb = 0;
  try {
    newdb = Sql::DbClient::open(opath);
    printf("Opened SQL database from authorization file %s\n",opath);
  } catch(...) {
    if (DbClient::open(opath)) {
      printf("NFS database at %s exists.  Delete and try again\n",opath);
      delete newdb;
      return 0;
    }
    else {
      printf("Creating NFS database at %s\n",opath);
      Pds_ConfigDb::Nfs::Path nfspath(opath);
      nfspath.create();
      newdb = DbClient::open(opath);
    }
  }

  Experiment db(ipath,Experiment::NoLock);
  std::list<ExptAlias>  alist = db.path().getExptAliases();
  std::list<DeviceType> dlist = db.path().getDevices();

  for(std::list<DeviceType>::iterator it=dlist.begin();
      it!=dlist.end(); it++)
    for(std::list<DeviceEntries>::iterator eit=it->entries.begin();
        eit!=it->entries.end(); eit++)
      for(std::list<XtcEntry>::iterator xit=eit->entries.begin();
          xit!=eit->entries.end(); xit++)
        _copy_xtc(db.path(),*newdb,*xit);

  newdb->begin();
  newdb->setExptAliases(alist);
  newdb->setDevices    (dlist);
  newdb->commit();

  newdb->begin();
  newdb->updateKeys();
  newdb->commit();

  delete newdb;
  return 0;
}

//
//  Update the database runkeys
//
int update_db(int argc, char** argv)
{
  static struct option _options[] = { {"db", 1, 0, 0},
                                      { 0, 0, 0, 0 } };

  const char* path = 0;

  char c;
  while( (c=getopt_long(argc, argv, "", _options, NULL)) != -1 ) {
    switch(c) {
    case 0: path = optarg; break;
    default: return -1;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    return -1;
  }

  if (!path) return -1;

  DbClient* newdb = DbClient::open(path);

  newdb->begin();
  newdb->updateKeys();
  newdb->commit();

  delete newdb;
  return 0;
}

//
//  Fetch the xtc
//
int fetch_xtc(int argc, char** argv)
{
  static struct option _options[] = { {"db"     , 1, 0, 0},
                                      { "source", 1, 0, 1},
                                      { "typeid", 1, 0, 2},
                                      { "key"   , 1, 0, 3},
                                      { 0, 0, 0, 0 } };

  const char* path = 0;
  uint64_t    source = 0;
  uint32_t    type_id = 0;
  uint32_t    runkey = 0;

  char c;
  while( (c=getopt_long(argc, argv, "", _options, NULL)) != -1 ) {
    switch(c) {
    case 0: path    = optarg; break;
    case 1: source  = strtoull(optarg, NULL, 16); break;
    case 2: type_id = strtoul (optarg, NULL, 16); break;
    case 3: runkey  = strtoul (optarg, NULL, 0); break;
    default: return -1;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    return -1;
  }

  printf("fetch_xtc path [%s] source [%016llx] typeid [%08x] key [%08x]\n",
         path,(unsigned long long)source,type_id,runkey);

  if (!path) return -1;

  DeviceEntry src(source);
  unsigned maxSize = 0x1000000;
  char* p = new char[maxSize];

  Pds::CfgClientNfs cl(src);

  Pds::Allocation alloc("configdb",path,0);
  cl.initialize(alloc);

  Pds::Transition tr(Pds::TransitionId::Configure, Pds::Env(runkey));
  int sz = cl.fetch(tr,
                    reinterpret_cast<const Pds::TypeId&>(type_id),
                    p, maxSize);
  printf("XTC size is %d\n",sz);

  delete[] p;
  return 0;
}

//
//  Remove keys
//
int truncate_db(int argc, char** argv)
{
  static struct option _options[] = { { "db"  , 1, 0, 0},
                                      { "key" , 1, 0, 1},
                                      { "time", 1, 0, 2},
                                      { 0, 0, 0, 0 } };

  const char* path = 0;
  time_t      tfrom=0,tto=0;
  uint32_t    kfrom=0,kto=0;

  char* pDash;
  char c;
  bool bb1, bb2;
  while( (c=getopt_long(argc, argv, "", _options, NULL)) != -1 ) {
    switch(c) {
    case 0: path    = optarg; break;
    case 1:
      pDash = strchr(optarg, '-');
      if ((pDash == NULL) || (pDash != strrchr(optarg, '-'))) {
        printf("%s: option `--key' parsing error\n", argv[0]);
        return -1;
      }
      *pDash = '\0';
      bb1 = parseUInt(optarg, kfrom);
      bb2 = parseUInt(pDash+1, kto);
      if (!bb1 || !bb2) {
        printf("%s: option `--key' parsing error\n", argv[0]);
        return -1;
      }
      break;
    case 2:
      pDash = strchr(optarg, '-');
      if ((pDash == NULL) || (pDash != strrchr(optarg, '-'))) {
        printf("%s: option `--time' parsing error\n", argv[0]);
        return -1;
      }
      { struct tm tm_v;
        memset(&tm_v,0,sizeof(tm_v));
        char *cc1 = strptime(strtok(optarg,"-"),"%Y%m%d",&tm_v);
        tfrom   = mktime(&tm_v);
        char *cc2 = strptime(strtok(NULL,"-"),"%Y%m%d",&tm_v);
        if (!cc1 || !cc2 || *cc1 || *cc2) {
          printf("%s: option `--time' parsing error\n", argv[0]);
          return -1;
        }
        tto     = mktime(&tm_v);
      } break;
    default: return -1;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    return -1;
  }

  char buf1[256],buf2[256];
  ctime_r(&tfrom,buf1); *strchr(buf1,'\n')=0;
  ctime_r(&tto  ,buf2); *strchr(buf2,'\n')=0;
  printf("truncate_db path [%s] key [%d-%d] time [%s]-[%s]\n",
         path,kfrom,kto,buf1,buf2);

  if (!path) return -1;

  DbClient* db = DbClient::open(path);

  //
  //  Remove the top-level keys
  //
  std::list<KeyEntry> none;
  std::list<Key> klist = db->getKeys();
  for(std::list<Key>::iterator it=klist.begin();
      it!=klist.end(); it++) {
    if ((it->key  >= kfrom && it->key  < kto) ||
        (it->time >= tfrom && it->time < tto)) {
      printf("Removing key %d\n",it->key);
      db->setKey(*it,none);
    }
  }
  db->purge();

  return 0;
}

//
//  Remove keys
//
int dump_db(int argc, char** argv)
{
  static struct option _options[] = { { "db"  , 1, 0, 0},
                                      { 0, 0, 0, 0 } };

  const char* path = 0;

  char c;
  while( (c=getopt_long(argc, argv, "", _options, NULL)) != -1 ) {
    switch(c) {
    case 0: path    = optarg; break;
    default: return -1;
    }
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    return -1;
  }

  DbClient* db = DbClient::open(path);

  std::list<Key> keys = db->getKeys();
  for(std::list<Key>::iterator it=keys.begin(); it!=keys.end(); it++) {
    std::string c(ctime(&it->time)); c.erase(c.size()-1);
    printf("Key %x  [%s] [%s]\n",it->key,it->name.c_str(),c.c_str());
    std::list<KeyEntry> entries = db->getKey(it->key);
    for(std::list<KeyEntry>::iterator kit=entries.begin(); kit!=entries.end(); kit++) {
      DeviceEntry e(kit->source);
      char* buffer;
      switch(e.level()) {
      case Pds::Level::Source:
        buffer = (char*) &e;
        printf("\t[%s] ",Pds::DetInfo::name(*(const Pds::DetInfo*)buffer));
        break;
      default:
        printf("\t[%s] ",Pds::Level::name(e.level()));
        break;
      }
      printf("[%s] [%s]\n",Pds::TypeId::name(kit->xtc.type_id.id()),kit->xtc.name.c_str());
    }
  }

  return 0;
}

static void _copy_xtc(DbClient& idb, DbClient& odb, const XtcEntry& x)
{
  int sz = idb.getXTC(x);
  if (sz > 0) {
    char* payload = new char[sz];
    idb.getXTC(x,payload,sz);

    odb.begin();
    odb.setXTC(x,payload,sz);
    odb.commit();

    delete[] payload;
  }
  else
    printf("Error seeking %s_v%u %s\n",
           Pds::TypeId::name(x.type_id.id()),
           x.type_id.version(),
           x.name.c_str());
}
