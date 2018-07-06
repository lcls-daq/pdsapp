#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "pds/archon/Driver.hh"

static const char sArchonTestVersion[] = "1.0";

static void showVersion(const char* p)
{
  printf( "Version:  %s  Ver %s\n", p, sArchonTestVersion );
}

static void showUsage(const char* p)
{
  printf("Usage: %s [-v|--version] [-h|--help]\n"
         "[-w|--write <filename prefix>] [-n|--number <number of images>] [-e|--exposure <exposure time (msec)>]\n"
         "[-C|--clear] -c|--config <config> -H|--host <host> [-P|--port <port>]\n"
         " Options:\n"
         "    -w|--write    <filename prefix>         output filename prefix\n"
         "    -n|--number   <number of images>        number of images to be captured (default: 1)\n"
         "    -e|--exposure <exposure time>           exposure time (msec) (default: 10000 msec)\n"
         "    -P|--port     <port>                    set the Archon controller tcp port number (default: 4242)\n"
         "    -H|--host     <host>                    set the Archon controller host ip\n"
         "    -c|--config   <config>                  the path to an Archon configuration file to use\n"
         "    -C|--clear                              clear the CCD before acquiring each frame\n"
         "    -v|--version                            show file version\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char *argv[]) {
  const char*         strOptions  = ":vhw:n:e:P:H:c:C";
  const struct option loOptions[] =
  {
    {"version",     0, 0, 'v'},
    {"help",        0, 0, 'h'},
    {"write",       1, 0, 'w'},
    {"number",      1, 0, 'n'},
    {"exposure",    1, 0, 'e'},
    {"port",        1, 0, 'P'},
    {"host",        1, 0, 'H'},
    {"config",      1, 0, 'c'},
    {"clear",       0, 0, 'C'},
    {0,             0, 0,  0 }
  };

  unsigned port = 4242;
  unsigned num_images = 1;
  unsigned exposure_time = 10000; // 10 sec
  bool lUsage = false;
  bool use_clear = false;
  char* filename = (char *)NULL;
  char* file_prefix = (char *)NULL;
  char* config = (char *)NULL;
  char* hostname = (char *)NULL;

  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        showUsage(argv[0]);
        return 0;
      case 'v':               /* Print usage */
        showVersion(argv[0]);
        return 0;
      case 'w':
        //filename = new char[strlen(optarg)+6];
        //sprintf(filename, "%s.data", optarg);
        file_prefix = optarg;
        break;
      case 'n':
        num_images = strtoul(optarg, NULL, 0);
        break;
      case 'e':
        exposure_time = strtoul(optarg, NULL, 0);
        break;
      case 'H':
        hostname = optarg;
        break;
      case 'P':
        port = strtoul(optarg, NULL, 0);
        break;
      case 'c':
        config = optarg;
        break;
      case 'C':
        use_clear = true;
        break;
      case '?':
        if (optopt)
          printf("%s: Unknown option: %c\n", argv[0], optopt);
        else
          printf("%s: Unknown option: %s\n", argv[0], argv[optind-1]);
      case ':':
        printf("%s: Missing argument for %c\n", argv[0], optopt);
      default:
        lUsage = true;
        break;
    }
  }

  if (!hostname) {
    printf("%s: Archon controller hostname is required\n", argv[0]);
    lUsage = true;
  }

  if (!config) {
    printf("%s: Archon configuration file is required\n", argv[0]);
    lUsage = true;
  }

  if (optind < argc) {
    printf("%s: invalid argument -- %s\n", argv[0], argv[optind]);
    lUsage = true;
  }

  if (lUsage) {
    showUsage(argv[0]);
    return 1;
  }

  if (file_prefix) {
    filename = new char[strlen(file_prefix) + 16];
  }

  Pds::Archon::Driver drv = Pds::Archon::Driver(hostname, port);
  if(drv.configure(config)) {
    const Pds::Archon::System& system = drv.system();
    const Pds::Archon::Status& status = drv.status();
    const Pds::Archon::BufferInfo& info = drv.buffer_info();
    if(drv.fetch_system()) {
      printf("Number of modules: %d\n", system.num_modules());
      printf("Backplane info:\n");
      printf(" Type: %u\n", system.type());
      printf(" Rev:  %u\n", system.rev());
      printf(" Ver:  %s\n", system.version().c_str());
      printf(" ID:   %#06x\n", system.id());
      printf(" Module present mask: %#06x\n", system.present());
      printf("Module Info:\n");
      for (unsigned i=0; i<16; i++) {
        if (system.module_present(i)) {
          printf(" Module %u:\n", i+1);
          printf("  Type: %u\n", system.module_type(i));
          printf("  Rev:  %u\n", system.module_rev(i));
          printf("  Ver:  %s\n", system.module_version(i).c_str());
          printf("  ID:   %#06x\n", system.module_id(i));
        }
      }
    }

    if (drv.fetch_status()) {
      if(status.is_valid()) {
        printf("Valid status returned by controller\n");
        printf("Number of module entries: %d\n", status.num_module_entries());
        printf("System status update count: %u\n", status.count());
        printf("Power status: %s\n", status.power_str());
        printf("Power is good: %s\n", status.is_power_good() ? "yes" : "no");
        printf("Overheated: %s\n", status.is_overheated() ? "yes" : "no");
        printf("Backplane temp: %.3f\n", status.backplane_temp());
      } else {
        printf("Invalid status returned by controller!\n");
        status.dump();
        return 1;
      }
    }


    if (drv.fetch_buffer_info()) {
      for(unsigned i=1; i<=info.nbuffers(); i++) {
        printf("%u - frame number %u, %lu\n", i, info.frame_num(i), info.timestamp(i));
      }
    }

    drv.set_preframe_clear(use_clear);
    drv.set_integration_time(exposure_time);

    Pds::Archon::FrameMetaData frame_meta;
    char* data = new char[info.size()];

    if (!drv.start_acquisition(num_images)) {
      printf("Failed to start acquisition!\n");
      return 1;
    }

    for (unsigned i=0; i<num_images; i++) {
      printf("waiting for image: %u/%u\n", i+1, num_images);
      if (drv.wait_frame(data, &frame_meta)) {
        printf("frame number, ts, size: %u %lu %ld\n", frame_meta.number, frame_meta.timestamp, frame_meta.size);
        if (file_prefix) {
          sprintf(filename, "%s_%u.raw", file_prefix, frame_meta.number);
          printf("writing image %u to file: %s\n", i+1, filename);
          FILE *f = fopen(filename, "wb");
          fwrite(data, sizeof(char), frame_meta.size, f);
          fclose(f);
        }
        //for (int i=0; i<100; i++) {
        //    printf("%u ", data[i]);
        //  }
        //  printf("\n");
      }
    }

    delete[] data;
    if (filename) delete[] filename;

  }

  return 0;
}
