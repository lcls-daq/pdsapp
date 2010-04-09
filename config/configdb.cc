#include "Path.hh"
#include "Experiment.hh"
#include "PdsDefs.hh"
using namespace Pds_ConfigDb;

// Commands
static string create_cmd("--create");
static string update_cmd("--update-keys");
static string branch_cmd("--branch");
static string create_device_cmd("--create-device");
static string create_device_alias_cmd("--create-device-alias");
static string copy_device_alias_cmd("--copy-device-alias");
static string import_device_data_cmd("--import-device-data");
static string assign_device_alias_cmd("--assign-device-alias");
static string create_expt_alias_cmd("--create-expt-alias");
static string copy_expt_alias_cmd("--copy-expt-alias");
static string assign_expt_alias_cmd("--assign-expt-alias");

void print_help(const char* p)
{
  printf( "Transactions:\n"
	  "  Create device with source ids\n"
	  " --create-device <DEV> <Src1> .. <SrcN>\n");
  printf( "  Create an alias entry for device\n");
  printf(" --create-device-alias <DEV> <ALIAS>\n");
  printf( "  Copy an alias entry for device\n");
  printf(" --copy-device-alias <DEV> <NEW_ALIAS> <OLD_ALIAS>\n");
  printf( "  Import configuration data for device\n");
  printf(" --import-device-data <DEV> <Type> <File> Description\n");
  printf( "  Assign configuration data to an alias entry for device\n");
  printf(" --assign-device-alias <DEV> <ALIAS> <Type> <File>\n");
  printf( "\n" );
  printf( "  Create a expt alias entry\n");
  printf(" --create-expt-alias <ALIAS>\n");
  printf( "  Copy a expt alias entry\n");
  printf(" --copy-expt-alias <NEW_ALIAS> <OLD_ALIAS>\n");
  printf( "  Assign a device alias entry to a expt alias entry\n");
  printf(" --assign-expt-alias <ALIAS> <DEV> <DEV_ALIAS>\n");
  printf( "\n");
  printf( "  Create the database\n");
  printf(" --create\n");
  printf( "\n");
  printf( "DB Management\n");
  printf( "  Update keys\n");
  printf(" --update-keys\n");
  printf( "  Branch a new db\n");
  printf(" --branch\n");
}

int main(int argc, char** argv)
{
  if (argc<3) {
    printf("Too few arguments\n");
    print_help(argv[0]);
    exit(1);
  }
      
  string cmd(argv[1]);
  string dbname(argv[2]);
  Path path(dbname);
  Experiment db(dbname);

  if (!path.is_valid()) {
    if (cmd==create_cmd)
      path.create();
    else {
      printf("No valid database found at %s\nExiting.\n",argv[2]);
      exit(1);
    }
  }
  else {
    db.read();

    printf("\n==BEFORE==\n");
    db.dump();

    if (cmd==update_cmd)
      db.update_keys();
    else if (cmd==branch_cmd || strcmp(cmd.c_str(),branch_cmd.c_str())==0) {
      Experiment* newdb = db.branch(string(argv[3]));
      newdb->read();
      printf("\n==NEWDB==\n");
      newdb->dump();
      delete newdb;
    }
    else {
      string dev(argv[3]);

      if (cmd==create_device_cmd) {
	list<DeviceEntry> devices;
	for(int k=4; k<argc; k++)
	  devices.push_back(DeviceEntry(string(argv[k])));
	db.add_device(dev,devices);
      }
      else if (cmd==create_device_alias_cmd)
	db.device(dev)->table().new_top_entry(string(argv[4]));
      else if (cmd==copy_device_alias_cmd)
	db.device(dev)->table().copy_top_entry(string(argv[4]),
					       string(argv[5]));
      else if (cmd==import_device_data_cmd) {
	string type(argv[4]);
	UTypeName utype(type);
	db.import_data(dev,
		       utype,
		       string(argv[5]),
		       string(argv[6]));
      }
      else if (cmd==assign_device_alias_cmd)
	db.device(dev)->table().set_entry(string(argv[4]),
					  FileEntry(string(argv[5]),
						    string(argv[6])));
      else if (cmd==create_expt_alias_cmd)
	db.table().new_top_entry(string(argv[3]));
      else if (cmd==copy_expt_alias_cmd)
	db.table().copy_top_entry(string(argv[3]),string(argv[4]));
      else if (cmd==assign_expt_alias_cmd)
	db.table().set_entry(string(argv[3]),FileEntry(string(argv[4]),
						       string(argv[5])));
      else
	printf("unknown command %s, try --help\n",cmd.c_str());
    }
  }

  printf("\n==AFTER==\n");
  db.dump();

  db.write();
}
