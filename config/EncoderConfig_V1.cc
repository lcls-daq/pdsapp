#include "EncoderConfig_V1.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"

#include "pdsdata/psddl/encoder.ddl.h"

#include <new>

#include <stdio.h>

using namespace Pds_ConfigDb;

// FIXME: This is blatant duplication of code in
// pds/encoder/driver/pci3e-wrapper.cc.  Yuck!
//
// These must end in NULL.
namespace PCI3E_V1
{
   static const char* count_mode_to_name[] = {
      "COUNT_MODE_WRAP_FULL",
      "COUNT_MODE_LIMIT",
      "COUNT_MODE_HALT",
      "COUNT_MODE_WRAP_PRESET",
      NULL
   };
   static const char* quad_mode_to_name[] = {
      "QUAD_MODE_CLOCK_DIR",
      "QUAD_MODE_X1",
      "QUAD_MODE_X2",
      "QUAD_MODE_X4",
      NULL
   };
}

class Pds_ConfigDb::EncoderConfig_V1::Private_Data {
 public:
   Private_Data();

   void insert( Pds::LinkedList<Parameter>& pList );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
  { return sizeof(Pds::Encoder::ConfigV1); }

   NumericInt<uint8_t>     _chan_num;
   Enumerated<Pds::Encoder::ConfigV1::count_mode_type> _count_mode;
   Enumerated<Pds::Encoder::ConfigV1::quad_mode      > _quadrature_mode;
   NumericInt<uint8_t>     _input_num;
   Enumerated<Enums::Bool> _input_rising;   
   NumericInt<uint32_t>    _ticks_per_sec;

};

Pds_ConfigDb::EncoderConfig_V1::Private_Data::Private_Data() 
   : _chan_num(        "Channel Number ",
                       0, 0, 2 ),
     _count_mode(      "Counter Mode ",
                       Pds::Encoder::ConfigV1::WRAP_FULL,
                       PCI3E_V1::count_mode_to_name ),
     _quadrature_mode( "Quadrature Mode ",
                       Pds::Encoder::ConfigV1::X1,
                       PCI3E_V1::quad_mode_to_name ),
     _input_num(       "Trigger Input Number ",
                       0, 0, 3 ),
     _input_rising(    "Trigger on Rising Edge ",
                       Enums::True,
                       Enums::Bool_Names ),
     _ticks_per_sec(   "Timestamp ticks per second ",
                       33000000, 32000000, 34000000)
{}

void Pds_ConfigDb::EncoderConfig_V1::Private_Data::insert( Pds::LinkedList<Parameter>& pList )
{
   pList.insert( &_chan_num );
   pList.insert( &_count_mode );
   pList.insert( &_quadrature_mode );
   pList.insert( &_input_num );
   pList.insert( &_input_rising );
   pList.insert( &_ticks_per_sec );
}

int Pds_ConfigDb::EncoderConfig_V1::Private_Data::pull( void* from )
{
  Pds::Encoder::ConfigV1& encoderConf = *reinterpret_cast<Pds::Encoder::ConfigV1*>(from);

  _chan_num.value        = encoderConf.chan_num();
  _count_mode.value      = encoderConf.count_mode();
  _quadrature_mode.value = encoderConf.quadrature_mode();
  _input_num.value       = encoderConf.input_num();
  _input_rising.value    = encoderConf.input_rising() ? Enums::True : Enums::False;
  _ticks_per_sec.value   = encoderConf.ticks_per_sec();

   return sizeof(Pds::Encoder::ConfigV1);
}

int Pds_ConfigDb::EncoderConfig_V1::Private_Data::push(void* to)
{
  new (to) Pds::Encoder::ConfigV1( _chan_num.value,
                                   _count_mode.value,
                                   _quadrature_mode.value,
                                   _input_num.value,
                                   _input_rising.value == Enums::True,
                                   _ticks_per_sec.value );

  return sizeof(Pds::Encoder::ConfigV1);
}

Pds_ConfigDb::EncoderConfig_V1::EncoderConfig_V1()
   : Serializer("Encoder_Config_V1"),
     _private_data( new Private_Data )
{
   _private_data->insert(pList);
}

int Pds_ConfigDb::EncoderConfig_V1::readParameters (void* from)
{
   return _private_data->pull(from);
}

int Pds_ConfigDb::EncoderConfig_V1::writeParameters(void* to)
{
   return _private_data->push(to);
}

int Pds_ConfigDb::EncoderConfig_V1::dataSize() const
{
   return _private_data->dataSize();
}

#include "Parameters.icc"
