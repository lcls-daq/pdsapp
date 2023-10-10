#include "pds/uxi/Detector.hh"
#include "pds/service/CmdLineTools.hh"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

static const char sUxiTestVersion[] = "1.0";

static void showVersion(const char* p)
{
  printf( "Version:  %s  Ver %s\n", p, sUxiTestVersion );
}

static void showUsage(const char* p)
{
  printf("Usage: %s [-v|--version] [-h|--help]\n"
         "[-w|--write <filename prefix>] [-n|--number <number of images>] [-t|--timing <ton>,<toff>[,tdel]]\n"
         "[-5|--pot5 <pot5>] [-7|--pot7 <pot7>] -H|--host <host> [-P|--port <portset>]\n"
         " Options:\n"
         "    -w|--write    <filename prefix>         output filename prefix\n"
         "    -n|--number   <number of images>        number of images to be captured (default: 1)\n"
         "    -t|--timing   <ton>,<toff>[,tdel]       the timing setup to use for the UXI detector (default: 2,2,0)\n"
         "    -P|--port     <portset>                 set the UXI detector server port set (default: 0)\n"
         "    -H|--host     <host>                    set the UXI detector server host ip\n"
         "    -5|--pot5     <pot5>                    the relaxation oscillator pot value (default: 2.8)\n"
         "    -7|--pot7     <pot7>                    the relaxation oscillator stationary param (default: 1.53)\n"
         "    -m|--max      <max>                     the maximum number of pixel values to print for each frame (default: 0)\n"
         "    -r|--roi      <r0,r1,f0,f1>             set an ROI with first/last row and then frame\n"
         "    -v|--version                            show file version\n"
         "    -h|--help                               print this message and exit\n", p);
}

int main(int argc, char *argv[]) {
  const char*         strOptions  = ":vhw:n:t:P:H:5:7:m:r:";
  const struct option loOptions[] =
  {
    {"version",     0, 0, 'v'},
    {"help",        0, 0, 'h'},
    {"write",       1, 0, 'w'},
    {"number",      1, 0, 'n'},
    {"timing",      1, 0, 't'},
    {"port",        1, 0, 'P'},
    {"host",        1, 0, 'H'},
    {"pot5",        1, 0, '5'},
    {"pot7",        1, 0, '7'},
    {"max",         1, 0, 'm'},
    {"roi",         1, 0, 'r'},
    {0,             0, 0,  0 }
  };

  unsigned portset = 0;
  unsigned baseport = Pds::Uxi::Detector::BasePort;
  unsigned num_images = 1;
  unsigned ton = 2;
  unsigned toff = 2;
  unsigned tdel = 0;
  unsigned max_display = 0;
  unsigned first_row = 0;
  unsigned last_row = 1023;
  unsigned first_frame = 0;
  unsigned last_frame = 3;
  double pot5 = 2.8;
  double pot7 = 1.53;
  const char* default_host = "localhost";
  bool use_roi = false;
  bool lUsage = false;
  char* filename = (char *)NULL;
  char* file_prefix = (char *)NULL;
  char* hostname = (char *)NULL;

  int optionIndex  = 0;
  while ( int opt = getopt_long(argc, argv, strOptions, loOptions, &optionIndex ) ) {
    if ( opt == -1 ) break;

    switch(opt) {
      case 'h':               /* Print usage */
        showUsage(argv[0]);
        return 0;
      case 'v':               /* Print version */
        showVersion(argv[0]);
        return 0;
      case 'w':
        file_prefix = optarg;
        break;
      case 'n':
        if (!Pds::CmdLineTools::parseUInt(optarg,num_images)) {
          printf("%s: option `-n' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 't':
        switch (Pds::CmdLineTools::parseUInt(optarg,ton,toff,tdel)) {
          case 2:
          case 3:
            break;
          default:
            printf("%s: option `-t' parsing error\n", argv[0]);
            lUsage = true;
            break;
        }
        break;
      case 'H':
        hostname = optarg;
        break;
      case 'P':
        if (!Pds::CmdLineTools::parseUInt(optarg,portset)) {
          printf("%s: option `-p' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case '5':
        if (!Pds::CmdLineTools::parseDouble(optarg,pot5)) {
          printf("%s: option `-r' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case '7':
        if (!Pds::CmdLineTools::parseDouble(optarg,pot7)) {
          printf("%s: option `-R' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'm':
        if (!Pds::CmdLineTools::parseUInt(optarg,max_display)) {
          printf("%s: option `-m' parsing error\n", argv[0]);
          lUsage = true;
        }
        break;
      case 'r':
        if (Pds::CmdLineTools::parseUInt(optarg,first_row,last_row,first_frame,last_frame)!=4) {
          printf("%s: option `-r' parsing error\n", argv[0]);
          lUsage = true;
        } else {
          use_roi = true;
        }
        break;
      case ':':
        printf("%s: Missing argument for %c\n", argv[0], optopt);
      default:
        lUsage = true;
        break;
    }
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

  Pds::Uxi::Detector drv = Pds::Uxi::Detector(
    hostname ? hostname : default_host,
    baseport + 2 * portset,
    baseport + 2 * portset +1
  );

  if (use_roi) {
    drv.set_row_roi(first_row, last_row);
    drv.set_frame_roi(first_row, last_row);
  } else {
    drv.reset_roi();
  }

  uint32_t width, height, nframes, nbytes, type;
  drv.width(&width);
  drv.height(&height);
  drv.nframes(&nframes);
  drv.nbytes(&nbytes);
  drv.type(&type);
  printf("Detector info:\n");
  printf(" Width is:      %u\n", width);
  printf(" Height is:     %u\n", height);
  printf(" Num frames is: %u\n", nframes);
  printf(" Num bytes is:  %u\n", nbytes);
  printf(" Type is:       %u\n", type);

  double temp;
  drv.temperature(&temp);
  printf(" Temp is:       %g C\n", temp);

  drv.set_pot(5, pot5);
  drv.set_pot(7, pot7);
  drv.set_timing_all(ton, toff, tdel);

  if (!drv.commit()) {
    printf("Failed to commit configuration to the detector!\n");
    return 1;
  }

  printf("Detector config:\n");

  uint32_t npots;
  double pot, pot_rbv;
  if (drv.num_pots(&npots)) {
    printf(" Pots:\n");
    for (unsigned i=1;i<=npots;i++) {
      drv.get_pot(i, &pot);
      drv.get_mon(i, &pot_rbv);
      printf("  %d: set %g, rbv %g\n", i, pot, pot_rbv);
    }
  }
  unsigned ton_rbv, toff_rbv, tdel_rbv;
  printf(" Timing:\n");
  drv.get_timing('A', &ton_rbv, &toff_rbv, &tdel_rbv);
  printf("  A: %u %u %u\n", ton_rbv, toff_rbv, tdel_rbv);
  drv.get_timing('B', &ton_rbv, &toff_rbv, &tdel_rbv);
  printf("  B: %u %u %u\n", ton_rbv, toff_rbv, tdel_rbv);

  int status = 0;
  uint16_t* data = 0;
  uint32_t acq_num = 0;
  uint32_t num_pixels;

  if (drv.num_pixels(&num_pixels)) {
    data = new uint16_t[num_pixels];

    if (drv.acquire(num_images)) {
      printf("Acquiring %u images from the detector.\n", num_images);
      for (unsigned i=0; i<num_images; i++) {
        if (drv.get_frames(acq_num, data)) {
          printf("Received frame acquisition %u", acq_num);
          if (max_display > 0) {
            printf(":");
            for (unsigned j=0; j<(max_display>num_pixels ? num_pixels : max_display); j++) {
              printf(" %u", data[j]);
            }
          }
          printf("\n");
          if (file_prefix) {
            sprintf(filename, "%s_%04u.raw", file_prefix, acq_num);
            printf("writing image %u to file: %s\n", i+1, filename);
            FILE *f = fopen(filename, "wb");
            fwrite(data, sizeof(uint16_t), num_pixels, f);
            fclose(f);
          }
        }
      }
    } else {
      printf("Failed to start acquisition!\n");
      status = 1;
    }
  } else {
    printf("Failed to determine number of pixels in detector!\n");
    status = 1;
  }

  if (data) delete[] data;
  if (filename) delete[] filename;

  return status;
}
