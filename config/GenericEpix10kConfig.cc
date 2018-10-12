#include "pdsapp/config/GenericEpix10kConfig.hh"
#include "pdsapp/config/Epix10kConfigP.hh"
#include "pdsapp/config/SequenceFactory.hh"
#include "pds/config/GenericPgpConfigType.hh"
#include "pds/config/EpixConfigType.hh"
#include "pds/config/EpixDataType.hh"
#include "pds/config/EpixSamplerConfigType.hh"
#include "pds/config/EpixSamplerDataType.hh"
#include "pds/epix10k/Epix10kConfigurator.hh"

#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>

using Pds_ConfigDb::GenericPgp::SequenceFactory;
using Pds::GenericPgp::CRegister;
using Pds::GenericPgp::CDimension;
using Pds::GenericPgp::CStream;

namespace Pds_ConfigDb {

  class GenericEpix10kConfig::PrivateData : public Epix10kConfigP {
    enum { MaxStreams=2 };
  public:
    PrivateData(unsigned key) { 
      _stream[0] = CStream(0,                         // pgp_channel
			   _epixDataType.value(),     // data_type
			   key,                       // config_type
			   0);                        // payload_offset
      _stream[1] = CStream(2,
			   _epixSamplerDataType.value(),
			   _epixSamplerConfigType.value(),
			   0);
    }
    ~PrivateData() {}
  public:
    int pull(void* p) {
      const GenericPgpConfigType& c = *reinterpret_cast<const GenericPgpConfigType*>(p);
      //
      //  Build the register set from the opaque section of the configuration
      //
      uint32_t* from = const_cast<uint32_t*>(c.payload().data());
      Epix10kConfigP::pull(from);

      return c._sizeof();
    }
    int push(void* to) {
      //
      //  Create a temporary configuration
      //
      unsigned nstreams=1;
      unsigned payload_size = Epix10kConfigP::dataSize()/sizeof(uint32_t);

      uint32_t* payload = new uint32_t[payload_size];
      Epix10kConfigP::push((char*)payload);

      const Epix10kConfigType&   e = *new(payload) Epix10kConfigType;
      const Epix10kConfigShadow& s = *new(payload) Epix10kConfigShadow;

      unsigned columns = e.numberOfAsicsPerRow()*e.numberOfPixelsPerAsicRow();
      unsigned rows    = e.numberOfRows();

      uint32_t* pixel_settings = new uint32_t[rows*columns];
      unsigned v = *e.asicPixelConfigArray().data();
      for(unsigned i=0; i<rows*columns; i++)
        pixel_settings[i] = v;

      if (e.scopeEnable()) {
	_stream[nstreams++] = CStream(2,  // virtual channel
				      _epixSamplerDataType.value(),
				      _epixSamplerConfigType.value(),
				      payload_size);

	payload_size += EpixSamplerConfigType::_sizeof()/sizeof(uint32_t);
      }

      //
      //  Create the sequences
      //
      unsigned reg_offset = payload_size;
      SequenceFactory* cs = _configuration_sequence(e,s,payload_size);
      payload_size += cs->payload_length();
      SequenceFactory* es = _enable_sequence       (payload_size);
      payload_size += es->payload_length();
      SequenceFactory* ds = _disable_sequence      (payload_size);
      payload_size += ds->payload_length();

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
      uint32_t* p = new uint32_t[payload_size];

      Epix10kConfigP::push((char*)(p+_stream[0].config_offset()));

      new (p+_stream[1].config_offset()) 
	EpixSamplerConfigType(1,
			      e.runTrigDelay(),
			      e.daqTrigDelay(),
			      e.dacSetting(),
			      e.adcClkHalfT(),
			      e.adcPipelineDelay(),
			      e.digitalCardId0(),
			      e.digitalCardId1(),
			      e.analogCardId0(),
			      e.analogCardId1(),
			      2,
			      e.scopeTraceLength(),
			      e.baseClockFrequency(),
			      0);
      
      ds->payload(es->payload(cs->payload(p+reg_offset)));

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
				     payload_size,
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
      //
      //  Create a temporary configuration
      //
      unsigned nstreams = 1;
      unsigned payload_size = Epix10kConfigP::dataSize()/sizeof(uint32_t);

      uint32_t* payload = new uint32_t[payload_size];
      const_cast<PrivateData*>(this)->Epix10kConfigP::push((char*)payload);

      const Epix10kConfigType&   e = *new(payload) Epix10kConfigType;
      const Epix10kConfigShadow& s = *new(payload) Epix10kConfigShadow;

      unsigned columns = e.numberOfAsicsPerRow()*e.numberOfPixelsPerAsicRow();
      unsigned rows    = e.numberOfRows();

      if (e.scopeEnable()) {
	nstreams++;
	payload_size += EpixSamplerConfigType::_sizeof()/sizeof(uint32_t);
      }

      //
      //  Create the sequences
      //
      SequenceFactory* cs = _configuration_sequence(e,s,payload_size);
      payload_size += cs->payload_length();
      SequenceFactory* es = _enable_sequence       (payload_size);
      payload_size += es->payload_length();
      SequenceFactory* ds = _disable_sequence      (payload_size);
      payload_size += ds->payload_length();

      GenericPgpConfigType c(CDimension(rows,
					columns),
			     cs->sequence_length()+
			     es->sequence_length()+
			     ds->sequence_length(), 
			     3, 
			     nstreams,
			     payload_size);
      delete[] payload;
      delete cs;
      delete es;
      delete ds;

      return c._sizeof();
    }
  private:
    SequenceFactory* _configuration_sequence(const Epix10kConfigType& e,
                                             const Epix10kConfigShadow& s,
                                             unsigned o) const 
    {
      //
      //  Prepare the configuration sequence:
      //
      SequenceFactory& cs = *new SequenceFactory(o);
      cs.write( Pds::Epix10k::ResetAddr,1 );
      cs.spin ( 10 );
      cs.flush();
      //  (1) resetSequenceCount
      cs.write( Pds::Epix10k::ResetFrameCounter,1 );
      cs.write( Pds::Epix10k::ResetAcqCounter  ,1 );
      cs.spin ( 10 );
      //  (2) enableRunTrigger(false)
      cs.write( Pds::Epix10k::RunTriggerEnable ,0 );
      //  (3) writeConfig();
      cs.write( Pds::Epix10k::PowerEnableAddr  ,Pds::Epix10k::PowerEnableValue );
      cs.write( Pds::Epix10k::SaciClkBitAddr   ,Pds::Epix10k::SaciClkBitValue  );
      cs.write( Pds::Epix10k::NumberClockTicksPerRunTriggerAddr,
                Pds::Epix10k::NumberClockTicksPerRunTrigger );
      cs.write( Pds::Epix10k::EnableAutomaticRunTriggerAddr,0 );
      cs.write( Pds::Epix10k::EnableAutomaticDaqTriggerAddr,0 );
      cs.write( Pds::Epix10k::EnableAutomaticRunTriggerAddr,1 );
      cs.write( Pds::Epix10k::TotalPixelsAddr, Pds::Epix10k::PixelsPerBank*(e.numberOfRowsPerAsic()-1) );
      for (unsigned i=0; i<Epix10kConfigShadow::NumberOfValues; i++) {
        if (Epix10kConfigShadow::use(i)==Epix10kConfigShadow::ReadWrite)
          cs.iwrite( Epix10kConfigShadow::address(i),i );
        if (Epix10kConfigShadow::use(i)==Epix10kConfigShadow::ReadOnly)
          cs.iread ( Epix10kConfigShadow::address(i),i );
        cs.write( Pds::Epix10k::DaqTrigggerDelayAddr, 
                  Pds::Epix10k::RunToDaqTriggerDelay+s.get(Epix10kConfigShadow::RunTrigDelay));
        cs.spin ( 100 );
      }
      //  (4) checkWrittenConfig(writeBack)
      for (unsigned i=0; i<Epix10kConfigShadow::NumberOfValues; i++)
        if (Epix10kConfigShadow::use(i)==Epix10kConfigShadow::ReadWrite)
          cs.iverify( Epix10kConfigShadow::address(i), i, Epix10kConfigShadow::mask(i) );
      //  (5) enableRunTrigger(true);
      cs.write( Pds::Epix10k::RunTriggerEnable, 1);
      //  (6) enableRunTrigger(false);
      cs.write( Pds::Epix10k::RunTriggerEnable, 0);
      //  (7) writeASIC();
      unsigned m = e.asicMask();
      for (unsigned index=0; index<e.numberOfAsics(); index++) {
	if (!(m&(1<<index))) continue;
        uint32_t u = (uint32_t*) &e.asics(index) - (uint32_t*)&e;
        uint32_t a = Pds::Epix10k::AsicAddrBase + Pds::Epix10k::AsicAddrOffset * index;
        for (unsigned i=0; i<Epix10kASIC_ConfigShadow::NumberOfValues; i++) {
          if (Epix10kASIC_ConfigShadow::use(i)==Epix10kASIC_ConfigShadow::ReadWrite || 
              Epix10kASIC_ConfigShadow::use(i)==Epix10kASIC_ConfigShadow::WriteOnly)
            cs.aiwrite( Epix10kASIC_ConfigShadow::address(i)+a, i+u );
          else if (Epix10kASIC_ConfigShadow::use(i)==Epix10kASIC_ConfigShadow::ReadOnly)
            cs.iread( Epix10kASIC_ConfigShadow::address(i)+a, i+u );
        }
        for (unsigned i=0; i<Pds::Epix10k::RepeatControlCount; i++)
          cs.awrite( Pds::Epix10k::WritePixelCommand+a, 0 );

        unsigned pixel = *e.asicPixelConfigArray().data();
        for (unsigned i=0; i<Pds::Epix10k::RepeatControlCount; i++)
          cs.awrite( Pds::Epix10k::GloalPixelCommand+a, pixel );

        for (unsigned i=0; i<Pds::Epix10k::RepeatControlCount; i++)
          cs.awrite( Pds::Epix10k::PrepareForReadout+a, 0 );

        cs.awrite( a+0, 0 );
        cs.awrite( a+0x8000, 0 );
        cs.awrite( a+0x6011, 0 );
        cs.awrite( a+0x2000, 2 );
      }
      cs.write( Pds::Epix10k::RunTriggerEnable, 1 );
      return &cs;
    }
    SequenceFactory* _enable_sequence(unsigned o) const
    {
      //
      //  Prepare the enable sequence:
      //
      SequenceFactory& cs = *new SequenceFactory(o);
      cs.write( Pds::Epix10k::DaqTriggerEnable, 1 );
      return &cs;
    }
    SequenceFactory* _disable_sequence(unsigned o) const
    {
      //
      //  Prepare the disable sequence:
      //
      SequenceFactory& cs = *new SequenceFactory(o);
      cs.write( Pds::Epix10k::DaqTriggerEnable, 0 );
      return &cs;
    }
  private:
    CStream  _stream[MaxStreams];
  };
};

using namespace Pds_ConfigDb;

GenericEpix10kConfig::GenericEpix10kConfig(unsigned key) :
  Serializer("Epix10k_Config"),
  _private(new PrivateData(key))
{
  name("EPIX10K");
  pList.insert(_private);
}

GenericEpix10kConfig::~GenericEpix10kConfig()
{
  delete _private;
}

int GenericEpix10kConfig::readParameters(void* from) { // pull "from xtc"
  return _private->pull(from);
}

int GenericEpix10kConfig::writeParameters(void* to) {
  return _private->push(to);
}

int GenericEpix10kConfig::dataSize() const {
  return _private->dataSize();
}



