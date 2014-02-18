#ifndef Pds_SequencerSync_hh
#define Pds_SequencerSync_hh

#include "pds/management/Sequencer.hh"

#include "pds/epicstools/PVWriter.hh"

namespace Pds {
  class SequencerSync : public Sequencer {
  public:
    SequencerSync(unsigned id);
  public:
    void   start();
    void   stop ();
  private:
    Pds_Epics::PVWriter       _seq_cntl_writer;
  };
};

#endif
