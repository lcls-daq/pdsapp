#ifndef PdsApp_AxiMicronN25Q_hh
#define PdsApp_AxiMicronN25Q_hh

namespace PdsApp {
  class Device;
  class McsFile;
  class AxiMicronN25Q {
  public:
    AxiMicronN25Q(const char* fname, Device& );
    virtual ~AxiMicronN25Q();
  public:
    void load();
    void verify();
  protected:
    void eraseProm();
    void writeProm();
    void verifyProm();

    virtual void resetFlash();
    void eraseCmd(unsigned address);
    void writeCmd(unsigned address);
    void readCmd (unsigned address);
    void setCmd  (unsigned value);
    virtual void waitForFlashReady();
    void setModeReg();
    void setAddrReg(unsigned v);
    void setCmdReg (unsigned v);
    void setDataReg(unsigned* v);
    unsigned  getCmdReg ();
    unsigned* getDataReg();

    void setPromStatusReg(unsigned value);
    unsigned getPromStatusReg();
    virtual unsigned getFlagStatusReg();
    unsigned getPromConfigReg();
    unsigned getManufacturerId();
    unsigned getManufacturerType();
    unsigned getManufacturerCapacity();
  protected:
    bool               addrMode; // true=32b mode
    McsFile*           mcs;
    Device&            dev;
  public:
    enum { READ_3BYTE_CMD    = 0x03<<16,
           READ_4BYTE_CMD    = 0x13<<16,
           FLAG_STATUS_REG   = 0x70<<16,
           WRITE_ENABLE_CMD  = 0x06<<16,
           WRITE_DISABLE_CMD = 0x04<<16,
           ADDR_ENTER_CMD    = 0xB7<<16,
           ADDR_EXIT_CMD     = 0xE9<<16,
           ERASE_CMD         = 0xD8<<16,
           WRITE_CMD         = 0x02<<16,
           STATUS_REG_WR_CMD = 0x01<<16,
           STATUS_REG_RD_CMD = 0x05<<16,
           DEV_ID_RD_CMD     = 0x9F<<16,
           WRITE_NONVOLATILE_CONFIG  = 0xB1<<16,
           WRITE_VOLATILE_CONFIG     = 0x81<<16,
           READ_NONVOLATILE_CONFIG   = 0xB5<<16,
           READ_VOLATILE_CONFIG      = 0x85<<16 };
    enum { FLAG_STATUS_RDY   = 0x80 };
    enum { DEFAULT_3BYTE_CONFIG = 0xFFFF,
           DEFAULT_4BYTE_CONFIG = 0xFFFE };
    enum { READ_MASK   = 0 };
    enum { WRITE_MASK  = 0x80000000,
           VERIFY_MASK = 0x40000000 };
  };
}

#endif
