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
         "[-N|--nonexp <non-exposure time>] [-b|--vbin <binning>] [-l|--lines <lines>] [-s|--skip <lines>]\n"
         "[-m|--max <max>] [-C|--clear] [-o|--on] [-t|--trigger] -c|--config <config> -H|--host <host>\n"
         "[-P|--port <port>]\n"
         " Options:\n"
         "    -w|--write    <filename prefix>         output filename prefix\n"
         "    -n|--number   <number of images>        number of images to be captured (default: 1)\n"
         "    -e|--exposure <exposure time>           exposure time (msec) (default: 10000 msec)\n"
         "    -N|--nonexp   <non-exposure time>       non-exposure time (msec) to wait after exposure (default: 100 msec)\n"
         "    -W|--wait     <wait time>               wait time (msec) time between frames when using internal trigger (default: 0 msec)\n"
         "    -b|--vbin     <vbinning>                the vertical binning (default: 1)\n"
         "    -l|--lines    <lines>                   the number of lines (default: 300)\n"
         "    -s|--skip     <lines>                   the number of preframe skipped lines (default: 22)\n"
         "    -B|--batch    <batch size>              the number of acquisitions to batch into a single frame (default: 0)\n"
         "    -m|--max      <max>                     the maximum number of pixel values to print for each frame (default: 0)\n"
         "    -P|--port     <port>                    set the Archon controller tcp port number (default: 4242)\n"
         "    -H|--host     <host>                    set the Archon controller host ip\n"
         "    -c|--config   <config>                  the path to an Archon configuration file to use\n"
         "    -C|--clear                              clear the CCD before acquiring each frame\n"
         "    -o|--on                                 power on the CCD (default: false)\n"
         "    -t|--trigger  <trigger>                 use external trigger to acquire each frame\n"
         "    -v|--version                            show file version\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char *argv[]) {
  const char*         strOptions  = ":vhw:n:e:N:W:b:l:s:B:m:P:H:c:Cot";
  const struct option loOptions[] =
  {
    {"version",     0, 0, 'v'},
    {"help",        0, 0, 'h'},
    {"write",       1, 0, 'w'},
    {"number",      1, 0, 'n'},
    {"exposure",    1, 0, 'e'},
    {"nonexp",      1, 0, 'N'},
    {"wait",        1, 0, 'W'},
    {"vbin",        1, 0, 'b'},
    {"lines",       1, 0, 'l'},
    {"skip",        1, 0, 's'},
    {"batch",       1, 0, 'B'},
    {"max",         1, 0, 'm'},
    {"port",        1, 0, 'P'},
    {"host",        1, 0, 'H'},
    {"config",      1, 0, 'c'},
    {"clear",       0, 0, 'C'},
    {"on",          0, 0, 'o'},
    {"trigger",     0, 0, 't'},
    {0,             0, 0,  0 }
  };

  unsigned port = 4242;
  unsigned num_images = 1;
  unsigned exposure_time = 10000; // 10 sec
  unsigned non_exposure_time = 100;
  unsigned waiting_time = 0;
  unsigned vertical_binning = 1;
  unsigned lines = 300;
  unsigned skip = 22;
  unsigned batch = 0;
  unsigned max_display = 0;
  bool lUsage = false;
  bool use_clear = false;
  bool use_trigger = false;
  bool power_on = false;
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
      case 'N':
        non_exposure_time = strtoul(optarg, NULL, 0);
        break;
      case 'W':
        waiting_time = strtoul(optarg, NULL, 0);
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
      case 'o':
        power_on = true;
        break;
      case 't':
        use_trigger = true;
        break;
      case 'b':
        vertical_binning = strtoul(optarg, NULL, 0);
        break;
      case 'l':
        lines = strtoul(optarg, NULL, 0);
        break;
      case 's':
        skip = strtoul(optarg, NULL, 0);
        break;
      case 'B':
        batch = strtoul(optarg, NULL, 0);
        break;
      case 'm':
        max_display = strtoul(optarg, NULL, 0);
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
    const Pds::Archon::Config& config = drv.config();
    if(drv.fetch_system()) {
      printf("Number of modules: %d\n", system.num_modules());
      printf("Backplane info:\n");
      printf(" Type: %u\n", system.type());
      printf(" Rev:  %u\n", system.rev());
      printf(" Ver:  %s\n", system.version().c_str());
      printf(" ID:   %s\n", system.id().c_str());
      printf(" Module present mask: %#06x\n", system.present());
      printf("Module Info:\n");
      for (unsigned i=0; i<16; i++) {
        if (system.module_present(i)) {
          printf(" Module %u:\n", i+1);
          printf("  Type: %u\n", system.module_type(i));
          printf("  Rev:  %u\n", system.module_rev(i));
          printf("  Ver:  %s\n", system.module_version(i).c_str());
          printf("  ID:   %s\n", system.module_id(i).c_str());
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

    // Power on the ccd
    if (power_on) {
      printf("Attempting to power on ccd\n");
      if (!drv.power_on()) {
        printf("Failure issuing power on command!\n");
        return 1;
      } else {
        printf("Waiting for power to reach on state ...");
        if(!drv.wait_power_mode(Pds::Archon::On)) {
          printf(" failed to reach on state!\n");
          drv.power_off();
          return 1;
        } else {
          printf(" succeeded!\n");
        }
      }
    }

    drv.set_vertical_binning(vertical_binning);
    drv.set_number_of_lines(lines, batch);
    drv.set_preframe_clear(use_clear ? lines : 0);
    drv.set_preframe_skip(skip);
    drv.set_integration_time(exposure_time);
    drv.set_non_integration_time(non_exposure_time);
    drv.set_waiting_time(waiting_time);
    drv.set_idle_clear();
    drv.set_external_trigger(use_trigger);
    //drv.set_frame_poll_interval(10);

    unsigned pixels_per_line = config.pixels_per_line();
    unsigned num_lines = config.linecount();
    unsigned num_pixels = config.total_pixels();
    unsigned bytes_per_pixel = config.bytes_per_pixel();
    unsigned frame_size = config.frame_size();
    unsigned sample_mode = config.samplemode();

    printf("Expected from size of the frame is %u bytes (%u pixels with %u bytes per pixel)\n", frame_size, num_pixels, bytes_per_pixel);
    printf("Expected frame shape is %ux%u pixels\n", pixels_per_line, num_lines);

    Pds::Archon::FrameMetaData frame_meta;
    char* data = new char[frame_size];
    uint16_t* data16 = (uint16_t*) data;
    uint32_t* data32 = (uint32_t*) data;

    if (!drv.start_acquisition(batch ? num_images * batch : num_images)) {
      printf("Failed to start acquisition!\n");
      if (power_on)
        drv.power_off();
      return 1;
    }

    for (unsigned i=0; i<num_images; i++) {
      printf("waiting for image: %u/%u\n", i+1, num_images);
      if (drv.wait_frame(data, &frame_meta)) {
        printf("frame number, ts, size: %u %lu %ld\n", frame_meta.number, frame_meta.timestamp, frame_meta.size);
        if (max_display > 0) {
          for (unsigned j=0; j<(max_display>num_pixels ? num_pixels : max_display); j++) {
            if (sample_mode)
              printf(" %u", data32[j]);
            else
              printf(" %u", data16[j]);
          }
          printf("\n");
        }
        if (file_prefix) {
          sprintf(filename, "%s_%u.raw", file_prefix, frame_meta.number);
          printf("writing image %u to file: %s\n", i+1, filename);
          FILE *f = fopen(filename, "wb");
          fwrite(data, sizeof(char), frame_meta.size, f);
          fclose(f);
        }
      }
    }

    if (power_on) {
      printf("Powering off the ccd\n");
      drv.power_off();
    }

    delete[] data;
    if (filename) delete[] filename;
  }

  return 0;
}
