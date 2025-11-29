#include "sls/Detector.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>

#include <string>
#include <vector>
#include <map>

static const char sJungfrauTestVersion[] = "1.0";

static std::map<uint32_t, std::string> regMap {
  {0x00, "FPGA_VERSION_REG"},
  {0x01, "FIX_PATT_REG"},
  {0x02, "STATUS_REG"},
  {0x03, "LOOK_AT_ME_REG"},
  {0x04, "SYSTEM_STATUS_REG"},
  {0x0A, "MOD_SERIAL_NUM_REG"},
  {0x0F, "API_VERSION_REG"},
  {0x10, "TIME_FROM_START_LSB_REG"},
  {0x11, "TIME_FROM_START_MSB_REG"},
  {0x12, "GET_DELAY_LSB_REG"},
  {0x13, "GET_DELAY_MSB_REG"},
  {0x14, "GET_CYCLES_LSB_REG"},
  {0x15, "GET_CYCLES_MSB_REG"},
  {0x16, "GET_FRAMES_LSB_REG"},
  {0x17, "GET_FRAMES_MSB_REG"},
  {0x18, "GET_PERIOD_LSB_REG"},
  {0x19, "GET_PERIOD_MSB_REG"},
  {0x1c, "GET_TEMPERATURE_TMP112_REG"},
  {0x1d, "CONFIG_V11_STATUS_REG"},
  {0x22, "FRAMES_FROM_START_LSB_REG"},
  {0x23, "FRAMES_FROM_START_MSB_REG"},
  {0x24, "GET_FRAME_NUMBER_LSB_REG"},
  {0x25, "GET_FRAME_NUMBER_MSB_REG"},
  {0x26, "START_FRAME_TIME_LSB_REG"},
  {0x27, "START_FRAME_TIME_MSB_REG"},
  {0x40, "SPI_REG"},
  {0x41, "ADC_SPI_REG"},
  {0x42, "ADC_OFST_REG"},
  {0x43, "ADC_PORT_INVERT_REG"},
  {0x44, "READ_N_ROWS_REG"},
  {0x4d, "CONFIG_REG"},
  {0x4E, "EXT_SIGNAL_REG"},
  {0x4F, "CONTROL_REG"},
  {0x50, "PLL_PARAM_REG"},
  {0x51, "PLL_CNTRL_REG"},
  {0x57, "PEDESTAL_MODE_REG"},
  {0x58, "CONFIG_V11_REG"},
  {0x59, "SAMPLE_REG"},
  {0x5A, "CRRNT_SRC_COL_LSB_REG"},
  {0x5B, "CRRNT_SRC_COL_MSB_REG"},
  {0x5C, "EXT_DAQ_CTRL_REG"},
  {0x5D, "DAQ_REG"},
  {0x5E, "CHIP_POWER_REG"},
  {0x5F, "TEMP_CTRL_REG"},
  {0x60, "SET_DELAY_LSB_REG"},
  {0x61, "SET_DELAY_MSB_REG"},
  {0x62, "SET_CYCLES_LSB_REG"},
  {0x63, "SET_CYCLES_MSB_REG"},
  {0x64, "SET_FRAMES_LSB_REG"},
  {0x65, "SET_FRAMES_MSB_REG"},
  {0x66, "SET_PERIOD_LSB_REG"},
  {0x67, "SET_PERIOD_MSB_REG"},
  {0x68, "SET_EXPTIME_LSB_REG"},
  {0x69, "SET_EXPTIME_MSB_REG"},
  {0x6A, "FRAME_NUMBER_LSB_REG"},
  {0x6B, "FRAME_NUMBER_MSB_REG"},
  {0x6C, "COMP_DSBLE_TIME_REG"},
  {0x70, "SET_TRIGGER_DELAY_LSB_REG"},
  {0x71, "SET_TRIGGER_DELAY_MSB_REG"},
  {0x7C, "COORD_ROW_REG"},
  {0x7D, "COORD_COL_REG"},
  {0x7E, "MOD_ID_REG"},
  {0x7F, "ASIC_CTRL_REG"},
  {0xF0, "ADC_DSRLZR_0_REG"},
  {0xF1, "ADC_DSRLZR_1_REG"},
  {0xF2, "ADC_DSRLZR_2_REG"},
  {0xF3, "ADC_DSRLZR_3_REG"},
};

static void showVersion(const char* p)
{
  printf( "Version:  %s  Ver %s\n", p, sJungfrauTestVersion );
}

static void showUsage(const char* p)
{
  printf("Usage: %s [-v|--version] [-h|--help]\n"
         "[-a|--address <starting address>] [-n|--number <number of registers] [-s|--sls <shmem id>] [-l|--list]\n"
         "-s|--sls <sls>\n"
         " Options:\n"
         "    -a|--address  <starting address>        address of the first register to read (default: 0x0)\n"
         "    -n|--number   <number of images>        number of registers to be read (default: 256)\n"
         "    -i|--id       <shmem id>                the shmem id to use for the detector (default: 0)\n"
         "    -s|--sls      <sls>                     set the hostname of the slsDetector interface\n"
         "    -l|--list                               flag to list all registers instead of just diffs\n"
         "    -v|--version                            show file version\n"
         "    -h|--help                               print this message and exit\n", p);
}

template<class R>
static bool checkSize(std::vector<std::string> names, sls::Result<R> results)
{
  if (names.size() != results.size()) {
    printf("\nCan't print names and results list have different sizes!\n");
    return false;
  } else {
    return true;
  }
}

static void printReg(uint32_t address,
                     std::vector<std::string> names,
                     sls::Result<uint32_t> regRbv)
{
  if (checkSize(names, regRbv)) {
    auto it = regMap.find(address);
    if (it != regMap.end()) {
      printf("\nRegister 0x%02x (%s):\n", address, it->second.c_str());
    } else {
      printf("\nRegister 0x%02x:\n", address);
    }
    for (size_t mod=0; mod < names.size(); mod++) {
      printf("  %-20s: 0x%02x\n", names[mod].c_str(), regRbv[mod]);
    }
  }
}

static void printInfoInt(std::string desc,
                         std::vector<std::string> names,
                         sls::Result<int> results)
{
  if (checkSize(names, results)) {
    printf("\n%s:\n", desc.c_str());
    for (size_t mod=0; mod < names.size(); mod++) {
      printf("  %-20s: %d\n", names[mod].c_str(), results[mod]);
    }
  }
}

static void printInfoDouble(std::string desc,
                            std::vector<std::string> names,
                            sls::Result<double> results)
{
  if (checkSize(names, results)) {
    printf("\n%s:\n", desc.c_str());
    for (size_t mod=0; mod < names.size(); mod++) {
      printf("  %-20s: %.1f\n", names[mod].c_str(), results[mod]);
    }
  }
}

static void printInfoHex(std::string desc,
                         std::vector<std::string> names,
                         sls::Result<int64_t> results)
{
 if (checkSize(names, results)) { 
   printf("\n%s:\n", desc.c_str());
   for (size_t mod=0; mod < names.size(); mod++) {
     printf("  %-20s: 0x%lx\n", names[mod].c_str(), (uint64_t) results[mod]);
   }
 }
}

static void printInfoStr(std::string desc,
                         std::vector<std::string> names,
                         sls::Result<std::string> results)
{
  if (checkSize(names, results)) {
    printf("\n%s:\n", desc.c_str());
    for (size_t mod=0; mod < names.size(); mod++) {
      printf("  %-20s: %s\n", names[mod].c_str(), results[mod].c_str());
    }
  }
}

template<class R>
static void printInfo(std::string desc,
                      std::vector<std::string> names,
                      sls::Result<R> results)
{
  if (checkSize(names, results)) {
    printf("\n%s:\n", desc.c_str());
    for (size_t mod=0; mod < names.size(); mod++) {
      printf("  %-20s: %s\n", names[mod].c_str(), sls::ToString(results[mod]).c_str());
    }
  }
}

int main(int argc, char **argv)
{
  const char*         strOptions  = ":vha:n:i:s:l";
  const struct option loOptions[] =
  {
    {"ver",         0, 0, 'v'},
    {"help",        0, 0, 'h'},
    {"address",     1, 0, 'a'},
    {"number",      1, 0, 'n'},
    {"id",          1, 0, 'i'},
    {"sls",         1, 0, 's'},
    {"list",        0, 0, 'l'},
    {0,             0, 0,  0 }
  };

  bool lUsage = false;
  bool showAll = false;
  int shmem_id = 0;
  unsigned address = 0x0;
  unsigned numRegs = 256;
  std::vector<std::string> sSlsHost;

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
      case 'a':
        address = strtoul(optarg, NULL, 0);
        break;
      case 'n':
        numRegs = strtoul(optarg, NULL, 0);
        break;
      case 'i':
        shmem_id = strtol(optarg, NULL, 0);
        break;
      case 's':
        sSlsHost.push_back(optarg);
        break;
      case 'l':
        showAll = true;
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
 
  if(sSlsHost.empty()) {
    printf("%s: slsDetector interface hostname for at least one module must be specified\n", argv[0]);
    lUsage = true;
  }

  if (lUsage) {
    showUsage(argv[0]);
    return 1;
  }

  // if there is only one detector show all registers
  if (sSlsHost.size() == 1) {
    showAll = true;
  }

  try {
    printf("Connecting to detectors:");
    for (const auto& name : sSlsHost) {
      printf(" %s", name.c_str());
    }
    printf("\n");
    // allocate shared memory detector class with specified id.
    sls::Detector det(shmem_id);

    // try to connect to the detector
    try {
      // set the hostnames of the detector
      det.setHostname(sSlsHost);

      // detector type
      printInfo("Detector type", sSlsHost, det.getDetectorType());

      printInfoStr("Detector server version", sSlsHost, det.getDetectorServerVersion());

      printInfoStr("Kernel version", sSlsHost, det.getKernelVersion());

      printInfo("Hardware version", sSlsHost, det.getHardwareVersion());

      printInfoHex("Firmware version", sSlsHost, det.getFirmwareVersion());

      printInfoHex("Serial number", sSlsHost, det.getSerialNumber());

      printInfoInt("Module id", sSlsHost, det.getModuleId());

      printInfoDouble("Chip version", sSlsHost, det.getChipVersion());

      // now print the requested registers
      printf("\nChecking %u registers starting at address 0x%x\n", numRegs, address);

      uint32_t numDiffs = 0;

      for (uint32_t offset=0; offset<numRegs; offset++) {
        sls::Result<uint32_t> regs = det.readRegister(address + offset);
        bool diff = false;
        uint32_t value = 0;
        // first pass to see if they differ
        for (size_t mod=0; mod < sSlsHost.size(); mod++) {
          if (mod == 0) {
            value = regs[mod];
          } else if (value != regs[mod]) {
            numDiffs++;
            diff = true;
          }
        }
        if (showAll || diff) {
          printReg(address + offset, sSlsHost, regs);
        }
      }

      printf("\nTotal registers that differ: %u out of %u\n", numDiffs, numRegs);
    } catch (const sls::RuntimeError &err) {
      auto status = det.getDetectorStatus();
      if (status.any(sls::defs::RUNNING) || status.any(sls::defs::WAITING)) {
        det.stopDetector();
        det.setHostname(sSlsHost);
      } else {
        // if detector wasn't running or stop fails re-raise to outer handler
        throw;
      }
    }
  }
  catch(const sls::RuntimeError &err) {
    printf("Encountered error communicating with the detector: %s\n", err.what());
  }

  // clean up the shared memory
  printf("\nCleaning up shmem (id %d):\n", shmem_id);
  sls::freeSharedMemory(shmem_id);

  return 0;
}
