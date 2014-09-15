#ifndef PdsConfigDb_SequenceFactory_hh
#define PdsConfigDb_SequenceFactory_hh

#include "pdsdata/psddl/epix.ddl.h"

namespace Pds_ConfigDb {
  namespace GenericPgp {
    class SequenceFactory {
    public:
      SequenceFactory(unsigned payload_offset);      
      ~SequenceFactory();
    public:
      void write  (unsigned addr, unsigned value );      
      void iwrite (unsigned addr, unsigned offset);
      void awrite (unsigned addr, unsigned value );      
      void aiwrite(unsigned addr, unsigned offset );      
      void read   (unsigned addr);      
      void iread  (unsigned addr, unsigned offset);
      void verify (unsigned addr, unsigned value ,unsigned mask);      
      void iverify(unsigned addr, unsigned offset,unsigned mask);
      void spin   (unsigned value);      
      void usleep (unsigned value);
      void flush  ();
    public:
      unsigned                   sequence_length() const;
      unsigned                   payload_length () const;
      Pds::GenericPgp::CRegister* sequence(Pds::GenericPgp::CRegister*) const;
      uint32_t*                  payload (uint32_t*) const;
    private:
      unsigned                               _payload_offset;
      std::vector<Pds::GenericPgp::CRegister> _seq;
      std::vector<uint32_t>                  _payload;
    };
  };
};

#endif
