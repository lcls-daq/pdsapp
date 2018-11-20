#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "pdsapp/devtest/AxiMicronN25Q.hh"
#include "pdsapp/devtest/McsFile.hh"
#include "pdsapp/devtest/Device.hh"

using namespace PdsApp;

void AxiMicronN25Q::eraseProm()
{
  const unsigned ERASE_SIZE = 0x10000;
  unsigned address = mcs->startAddr();
  const unsigned size = mcs->write_size();
  unsigned report = address + size/10;
  while(address < mcs->endAddr()) {
    eraseCmd(address);
    address += ERASE_SIZE;
    if (address > report) {
      printf("Erase %d%% complete\n", 100*(address-mcs->startAddr())/size);
      report += size/10;
    }
  }
}
void AxiMicronN25Q::writeProm()
{
  unsigned wordCnt=0, byteCnt=0, wrd=0;
  unsigned addr,dummy,data;
  uint32_t dataArray[64];
  const unsigned write_size = mcs->write_size();
  unsigned report = write_size/10;
  unsigned nPrint=4;
  for(unsigned i=0; i<write_size; i++) {
    if (byteCnt==0) {
      if (wordCnt==0)
        mcs->entry(i,addr,data);
      else
        mcs->entry(i,dummy,data);
      wrd = (data&0xff) << 24;
      ++byteCnt;
    }
    else {
      mcs->entry(i,dummy,data);
      wrd |= (data&0xff) << (8*(3-byteCnt));

      if (++byteCnt==4) {
        byteCnt=0;
        dataArray[wordCnt++] = wrd;
        if (wordCnt==64) {
          wordCnt=0;
          setDataReg(dataArray);
          writeCmd(addr);
          if (nPrint) {
            nPrint--;
            printf("Write [addr=%x]:",addr);
            for(unsigned j=0; j<64; j++)
              printf(" %08x", dataArray[j]);
            printf("\n");
          }
        }
      }
    }
    if (i > report) {
      printf("WriteProm %d%% complete\n", 100*i/write_size);
      report += write_size/10;
    }
  }
  printf("Final write: wordCnt %u, byteCnt %u, wrd %x\n",
         wordCnt, byteCnt, wrd);
}
void AxiMicronN25Q::verifyProm()
{
  waitForFlashReady();
  unsigned wordCnt=0, byteCnt=0, data,addr;
  unsigned* dataArray = 0;
  const unsigned read_size = mcs->read_size();
  unsigned report = read_size/10;
  unsigned nPrint=0;
  bool lFail=false;
  unsigned nFail=0;
  for (unsigned i=0; i<read_size; i++) {
    if (!byteCnt && !wordCnt) {
      mcs->entry(i,addr,data);
      readCmd(addr);
      dataArray = getDataReg();
      if (nPrint) {
        nPrint--;
        printf("Read [addr=%x]:",addr);
        for(unsigned j=0; j<64; j++)
          printf(" %08x", dataArray[j]);
        printf("\n");
      }
      lFail=false;
    }
    mcs->entry(i,addr,data);
    unsigned prom = ((dataArray[wordCnt] >> (8*(3-byteCnt))) & 0xff);
    if (data != prom && !lFail) {
      printf("VerifyProm failed!\n");
      printf("Addr = 0x%x:  WordCnt %x, ByteCnt %x, MCS = 0x%x != PROM = 0x%x\n",
             addr,wordCnt,byteCnt,data,prom);
      lFail=true;
      if (++nFail==20)
        exit(1);
    }
    if (++byteCnt==4) {
      byteCnt=0;
      if (++wordCnt==64)
        wordCnt=0;
    }
    if (i > report) {
      printf("Verify %d%% complete\n",100*i/read_size);
      report += read_size/10;
    }
  }
}

void AxiMicronN25Q::resetFlash()
{
  setCmdReg(WRITE_MASK|(0x66<<16));
  usleep(1000);
  setCmdReg(WRITE_MASK|(0x99<<16));
  usleep(1000);
  setModeReg();
  if (addrMode) {
    setCmd(WRITE_MASK|ADDR_ENTER_CMD);
    setAddrReg(DEFAULT_4BYTE_CONFIG<<16);
  }
  else {
    setCmd(WRITE_MASK|ADDR_EXIT_CMD);
    setAddrReg(DEFAULT_3BYTE_CONFIG<<8);
  }
  usleep(1000);
  setCmd(WRITE_MASK|WRITE_NONVOLATILE_CONFIG|0x2);
  setCmd(WRITE_MASK|WRITE_VOLATILE_CONFIG|0x2);
}
void AxiMicronN25Q::eraseCmd(unsigned address)
{
  setAddrReg(address);
  setCmd(WRITE_MASK | ERASE_CMD | (addrMode ? 0x4:0x3));
}
void AxiMicronN25Q::writeCmd(unsigned address)
{
  setAddrReg(address);
  setCmd(WRITE_MASK|WRITE_CMD|(addrMode?0x104:0x103));
}
void AxiMicronN25Q::readCmd(unsigned address)
{
  setAddrReg(address);
  setCmd(READ_MASK|(addrMode?(READ_4BYTE_CMD|0x104):(READ_3BYTE_CMD|0x103)));
}
void AxiMicronN25Q::setCmd(unsigned value)
{
  if (value&WRITE_MASK) {
    waitForFlashReady();
    setCmdReg(WRITE_MASK|WRITE_ENABLE_CMD);
  }
  setCmdReg(value);
}
void AxiMicronN25Q::waitForFlashReady()
{
  while(1) {
    setCmdReg(READ_MASK|FLAG_STATUS_REG|0x1);
    volatile unsigned status = getCmdReg()&0xff;
    if (( status & FLAG_STATUS_RDY)) 
      break;
  }
}
void AxiMicronN25Q::setModeReg()            { dev.rawWrite(0x04,addrMode?0x1:0x0); }
void AxiMicronN25Q::setAddrReg(unsigned v)  { dev.rawWrite(0x08,v); }
void AxiMicronN25Q::setCmdReg (unsigned v)  { dev.rawWrite(0x0c,v); }
void AxiMicronN25Q::setDataReg(unsigned* v) { dev.rawWrite(0x200,v); }
unsigned  AxiMicronN25Q::getCmdReg () { return dev.rawRead(0x0c); }
unsigned* AxiMicronN25Q::getDataReg() { return dev.rawRead(0x200,64); }

void AxiMicronN25Q::setPromStatusReg(unsigned value)
{
  setAddrReg((value&0xff)<<(addrMode?24:16));
  setCmd(WRITE_MASK|STATUS_REG_WR_CMD|0x1);
}
unsigned AxiMicronN25Q::getPromStatusReg()
{
  setCmd(READ_MASK|STATUS_REG_RD_CMD|0x1);
  return getCmdReg()&0xff;
}
unsigned AxiMicronN25Q::getFlagStatusReg()
{
  setCmd(READ_MASK|FLAG_STATUS_REG|0x1);
  return getCmdReg()&0xff;
}
unsigned AxiMicronN25Q::getPromConfigReg()
{
  setCmd(READ_MASK|DEV_ID_RD_CMD|0x1);
  return getCmdReg()&0xff;
}
unsigned AxiMicronN25Q::getManufacturerId()
{
  setCmd(READ_MASK|DEV_ID_RD_CMD|0x1);
  return getCmdReg()&0xff;
}
unsigned AxiMicronN25Q::getManufacturerType()
{
  setCmd(READ_MASK|DEV_ID_RD_CMD|0x2);
  return getCmdReg()&0xff;
}
unsigned AxiMicronN25Q::getManufacturerCapacity()
{
  setCmd(READ_MASK|DEV_ID_RD_CMD|0x3);
  return getCmdReg()&0xff;
}

AxiMicronN25Q::AxiMicronN25Q(const char* fname,
                             Device&     vdev) :  
  addrMode(false),
  mcs(new McsFile(fname)),
  dev(vdev)
{
}

AxiMicronN25Q::~AxiMicronN25Q() { delete mcs; }

void AxiMicronN25Q::load()
{
  resetFlash();

  printf("Manufacturer ID Code  = %x\n",getManufacturerId());
  printf("Manufacturer Type     = %x\n",getManufacturerType());
  printf("Manufacturer Capacity = %x\n",getManufacturerCapacity());
  printf("Status Register       = %x\n",getPromStatusReg());
  printf("Flag Status Register  = %x\n",getFlagStatusReg());
  printf("Volatile Config Reg   = %x\n",getPromConfigReg());

  eraseProm();

  writeProm();
}

void AxiMicronN25Q::verify()
{
  printf("Manufacturer ID Code  = %x\n",getManufacturerId());
  printf("Manufacturer Type     = %x\n",getManufacturerType());
  printf("Manufacturer Capacity = %x\n",getManufacturerCapacity());
  printf("Status Register       = %x\n",getPromStatusReg());
  printf("Flag Status Register  = %x\n",getFlagStatusReg());
  printf("Volatile Config Reg   = %x\n",getPromConfigReg());
  verifyProm();
}

