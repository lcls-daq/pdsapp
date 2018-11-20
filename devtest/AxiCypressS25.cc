#include "pdsapp/devtest/AxiCypressS25.hh"
#include <unistd.h>

using namespace PdsApp;


static const unsigned _FLAG_STATUS_REG = (0x05 << 16);
static const unsigned _FLAG_STATUS_RDY = (0x01);
static const unsigned _BRAC_CMD = (0xB9 << 16);

AxiCypressS25::AxiCypressS25(const char* fname,
                             Device&     dev) :
  AxiMicronN25Q(fname,dev)
{
  addrMode = true;
}

void AxiCypressS25::resetFlash() 
{
  // Send the "Mode Bit Reset" command
  setCmdReg(AxiMicronN25Q::WRITE_MASK|(0xFF << 16));
  usleep(1000);
  // Send the "Software Reset" Command
  setCmdReg(AxiMicronN25Q::WRITE_MASK|(0xF0 << 16));
  usleep(1000);
  // Set the addressing mode
  setModeReg();
  // Check the address mode
  if (addrMode)
    setCmd(AxiMicronN25Q::WRITE_MASK|_BRAC_CMD|0x80);
  else
    setCmd(AxiMicronN25Q::WRITE_MASK|_BRAC_CMD);
}

unsigned AxiCypressS25::getFlagStatusReg()
{
  setCmd(AxiMicronN25Q::READ_MASK|_FLAG_STATUS_REG|0x1);
  return getCmdReg()&0xff;
}

void AxiCypressS25::waitForFlashReady()
{
  unsigned status;
  while(1) {
    // Get the status register
    setCmdReg(AxiMicronN25Q::READ_MASK|_FLAG_STATUS_REG|0x1);
    status = (getCmdReg()&0xFF) ;
    // Check if not busy
    if ( (status & _FLAG_STATUS_RDY) == 0 )
      break;
  }
}
