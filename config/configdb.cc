#include "Experiment.hh"

using namespace Pds_ConfigDb;

// Commands
static string create_cmd("--create");
static string update_cmd("--update-keys");
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
	  "%s --create-device <DEV> <Src1> .. <SrcN>\n",p);
  printf( "  Create an alias entry for device\n");
  printf("%s --create-device-alias <DEV> <ALIAS>\n",p);
  printf( "  Copy an alias entry for device\n");
  printf("%s --copy-device-alias <DEV> <NEW_ALIAS> <OLD_ALIAS>\n",p);
  printf( "  Import configuration data for device\n");
  printf("%s --import-device-data <DEV> <Type> <File> Description\n",p);
  printf( "  Assign configuration data to an alias entry for device\n");
  printf("%s --assign-device-alias <DEV> <ALIAS> <Type> <File>\n",p);
  printf( "\n" );
  printf( "  Create a expt alias entry\n");
  printf("%s --create-expt-alias <ALIAS>\n",p);
  printf( "  Copy a expt alias entry\n");
  printf("%s --copy-expt-alias <NEW_ALIAS> <OLD_ALIAS>\n",p);
  printf( "  Assign a device alias entry to a expt alias entry\n");
  printf("%s --assign-expt-alias <ALIAS> <DEV> <DEV_ALIAS>\n",p);
  printf( "\n");
  printf( "  Create the database\n");
  printf("%s --create\n",p);
  printf( "\n");
  printf( "DB Management\n");
  printf( "  Update keys\n");
  printf("%s --update-keys\n",p);
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
  Experiment db(dbname);

  if (!db.is_valid()) {
    if (cmd==create_cmd)
      db.create();
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
      else if (cmd==import_device_data_cmd)
	db.import_data(dev,
		       string(argv[4]),
		       string(argv[5]),
		       string(argv[6]));
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
