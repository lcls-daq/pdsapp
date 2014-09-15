#include "pdsapp/config/GenericEpixConfig.hh"
#include "pdsapp/config/EpixConfigP.hh"
#include "pdsapp/config/SequenceFactory.hh"
#include "pds/config/GenericPgpConfigType.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixDataType.hh"
#include "pds/config/EpixASICConfigV1.hh"
#include "pds/config/EpixConfigV1.hh"
#include "pds/epix/EpixConfigurator.hh"

#include <new>

using Pds_ConfigDb::GenericPgp::SequenceFactory;
using Pds::Epix::EpixConfigurator;
using Pds::GenericPgp::CRegister;
using Pds::GenericPgp::CDimension;
using Pds::GenericPgp::CStream;

namespace Pds_ConfigDb {

  class GenericEpixConfig::PrivateData : public EpixConfigP {
    enum { nstreams=1 };
  public:
    PrivateData(unsigned key) {
      _stream[0] = CStream(0,
			   _epixDataType.value(),
			   key,
			   0);
      printf("GenericEpixConfig key 0x%x\n",key);
    }
    ~PrivateData() {}
  public:
    int pull(void* p) {
      const GenericPgpConfigType& c = *reinterpret_cast<const GenericPgpConfigType*>(p);
      //
      //  Build the register set from the opaque section of the configuration
      //
      uint32_t* from = const_cast<uint32_t*>(c.payload().data());
      EpixConfigP::pull(from);

      return c._sizeof();
    }
    int push(void* to) {
      //
      //  Create a temporary configuration
      //
      unsigned payload_size = EpixConfigP::dataSize()/sizeof(uint32_t);
      uint32_t* payload = new uint32_t[payload_size];
      EpixConfigP::push((char*)payload);

      const EpixConfigType&   e = *new(payload) EpixConfigType;
      const EpixConfigShadow& s = *new(payload) EpixConfigShadow;

      unsigned columns = e.numberOfAsicsPerRow()*e.numberOfPixelsPerAsicRow();
      unsigned rows    = e.numberOfRows();

      uint32_t* pixel_settings = new uint32_t[rows*columns];
      unsigned v = 
        ((*e.asicPixelMaskArray().data()==0) ? 0:0x1) |
        ((*e.asicPixelTestArray().data()==0) ? 0:0x2);
      for(unsigned i=0; i<rows*columns; i++)
        pixel_settings[i] = v;

      //
      //  Create the sequences
      //
      unsigned o = payload_size;
      SequenceFactory* cs = _configuration_sequence(e,s,o);
      o += cs->payload_length();
      SequenceFactory* es = _enable_sequence       (o);
      o += es->payload_length();
      SequenceFactory* ds = _disable_sequence      (o);
      o += ds->payload_length();

      uint32_t sequence_lengths[3];
      sequence_lengths[0] = cs->sequence_length();
      sequence_lengths[1] = es->sequence_length();
      sequence_lengths[2] = ds->sequence_length();

      unsigned reg_length = cs->sequence_length()+es->sequence_length()+ds->sequence_length();
      CRegister* regs = new CRegister[reg_length];
      ds->sequence(es->sequence(cs->sequence(regs)));

      //
      //  Create the opaque section
      //
      unsigned psize = o;
      uint32_t* p = new uint32_t[psize];

      EpixConfigP::push((char*)p);
      ds->payload(es->payload(cs->payload(p+payload_size)));

      GenericPgpConfigType* c = 
	new(to) GenericPgpConfigType(0,
				     CDimension(rows,
						columns),
				     CDimension(e.lastRowExclusions(),
						columns),
				     CDimension(1,e.numberOfAsics()),
				     cs->sequence_length()+
				     es->sequence_length()+
				     ds->sequence_length(), 3, 
				     nstreams,
				     psize,
				     pixel_settings, 
				     sequence_lengths,
				     regs,
				     _stream,
				     p);
      delete[] pixel_settings;
      delete[] payload;
      delete[] p;
      delete[] regs;
      delete cs;
      delete es;
      delete ds;

      return c->_sizeof();
    }
    int dataSize() const {
      unsigned payload_size = EpixConfigP::dataSize()/sizeof(uint32_t);
      uint32_t* payload = new uint32_t[payload_size];
      const_cast<PrivateData*>(this)->EpixConfigP::push((char*)payload);
      const EpixConfigType&   e = *new(payload) EpixConfigType;
      const EpixConfigShadow& s = *new(payload) EpixConfigShadow;

      unsigned columns = e.numberOfAsicsPerRow()*e.numberOfPixelsPerAsicRow();
      unsigned rows    = e.numberOfRows();

      unsigned o = payload_size;
      SequenceFactory* cs = _configuration_sequence(e,s,o);
      o += cs->payload_length();
      SequenceFactory* es = _enable_sequence       (o);
      o += es->payload_length();
      SequenceFactory* ds = _disable_sequence      (o);
      o += ds->payload_length();

      GenericPgpConfigType c(CDimension(rows,        // frame size (pixel_settings)
					columns),
			     cs->sequence_length()+  // register operations
			     es->sequence_length()+
			     ds->sequence_length(),
			     3,                      // number of sequences
			     nstreams,
			     o);                     // payload size
                            
      delete[] payload;
      delete cs;
      delete ds;
      delete es;

      return c._sizeof();
    }
  private:
    SequenceFactory* _configuration_sequence(const EpixConfigType& e,
                                             const EpixConfigShadow& s,
                                             unsigned o) const 
    {
      //
      //  Prepare the configuration sequence:
      //
      SequenceFactory* f = new SequenceFactory(o);
      SequenceFactory& cs = *f;
      //  (1) resetSequenceCount
      cs.write( Pds::Epix::ResetFrameCounter,1 );
      cs.write( Pds::Epix::ResetAcqCounter  ,1 );
      cs.spin ( 10 );
      //  (2) enableRunTrigger(false)
      cs.write( Pds::Epix::RunTriggerEnable ,0 );
      //  (3) writeConfig();
      cs.write( Pds::Epix::PowerEnableAddr  ,Pds::Epix::PowerEnableValue );
      cs.write( Pds::Epix::SaciClkBitAddr   ,Pds::Epix::SaciClkBitValue  );
      cs.write( Pds::Epix::NumberClockTicksPerRunTriggerAddr,
                Pds::Epix::NumberClockTicksPerRunTrigger );
      cs.write( Pds::Epix::EnableAutomaticRunTriggerAddr,1 );
      cs.write( Pds::Epix::EnableAutomaticDaqTriggerAddr,0 );
      cs.write( Pds::Epix::TotalPixelsAddr, Pds::Epix::PixelsPerBank*(e.numberOfRowsPerAsic()-1) );
      for (unsigned i=0; i<EpixConfigShadow::NumberOfValues; i++) {
        if (EpixConfigShadow::use(i)==EpixConfigShadow::ReadWrite)
          cs.iwrite( EpixConfigShadow::address(i),i );
        if (EpixConfigShadow::use(i)==EpixConfigShadow::ReadOnly)
          cs.iread ( EpixConfigShadow::address(i),i );
        cs.write( Pds::Epix::DaqTrigggerDelayAddr, 
                  Pds::Epix::RunToDaqTriggerDelay+s.get(EpixConfigShadow::RunTrigDelay));
        cs.spin ( 100 );
      }
      //  (4) checkWrittenConfig(writeBack)
      for (unsigned i=0; i<EpixConfigShadow::NumberOfValues; i++)
        if (EpixConfigShadow::use(i)==EpixConfigShadow::ReadWrite)
          cs.iverify( EpixConfigShadow::address(i), i, EpixConfigShadow::mask(i) );
      //  (5) enableRunTrigger(true);
      cs.write( Pds::Epix::RunTriggerEnable, 1);
      //  (6) enableRunTrigger(false);
      cs.write( Pds::Epix::RunTriggerEnable, 0);
      //  (7) writeASIC();
      for (unsigned index=0; index<e.numberOfAsics(); index++) {
        uint32_t u = (uint32_t*) &e.asics(index) - (uint32_t*)&e;
        uint32_t a = Pds::Epix::AsicAddrBase + Pds::Epix::AsicAddrOffset * index;
        for (unsigned i=0; i<EpixASIC_ConfigShadow::NumberOfValues; i++) {
          if (EpixASIC_ConfigShadow::use(i)==EpixASIC_ConfigShadow::ReadWrite || 
              EpixASIC_ConfigShadow::use(i)==EpixASIC_ConfigShadow::WriteOnly)
            cs.iwrite( EpixASIC_ConfigShadow::address(i)+a, i+u );
          else if (EpixASIC_ConfigShadow::use(i)==EpixASIC_ConfigShadow::ReadOnly)
            cs.iread( EpixASIC_ConfigShadow::address(i)+a, i+u );
        }
        for (unsigned i=0; i<Pds::Epix::RepeatControlCount; i++)
          cs.write( Pds::Epix::WritePixelCommand+a, 0 );
        unsigned test = e.asicPixelTestArray()[index][0][0];
        unsigned mask = e.asicPixelMaskArray()[index][0][0];
        uint32_t bits = (test ? 1 : 0) | (mask ? 2 : 0);
        for (unsigned i=0; i<Pds::Epix::RepeatControlCount; i++)
          cs.write( Pds::Epix::GloalPixelCommand+a, bits );
        cs.write( a+0, 0 );
        cs.write( a+0x8000, 0 );
        cs.write( a+0x6011, 0 );
        cs.write( a+0x2000, 2 );
      }
      cs.write( Pds::Epix::RunTriggerEnable, 1 );
      return f;
    }
    SequenceFactory* _enable_sequence(unsigned o) const
    {
      //
      //  Prepare the enable sequence:
      //
      SequenceFactory* f = new SequenceFactory(o);
      SequenceFactory& cs = *f;
      cs.write( Pds::Epix::DaqTriggerEnable, 1 );
      return f;
    }
    SequenceFactory* _disable_sequence(unsigned o) const
    {
      //
      //  Prepare the disable sequence:
      //
      SequenceFactory* f = new SequenceFactory(o);
      SequenceFactory& cs = *f;
      cs.write( Pds::Epix::DaqTriggerEnable, 0 );
      return f;
    }
  private:
    CStream _stream[nstreams];
  };
};


using namespace Pds_ConfigDb;

GenericEpixConfig::GenericEpixConfig(unsigned key) :
  Serializer("Epix_Config"),
  _private(new PrivateData(key))
{
  name("EPIX");
  pList.insert(_private);
}

GenericEpixConfig::~GenericEpixConfig()
{
  delete _private;
}

int GenericEpixConfig::readParameters(void* from) { // pull "from xtc"
  return _private->pull(from);
}

int GenericEpixConfig::writeParameters(void* to) {
  return _private->push(to);
}

int GenericEpixConfig::dataSize() const {
  return _private->dataSize();
}
