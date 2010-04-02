#include "EncoderConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pdsapp/config/BitCount.hh"
#include "pds/config/EncoderConfigType.hh"

#include <new>

using namespace Pds_ConfigDb;

// FIXME: This is blatant duplication of code in
// pds/encoder/driver/pci3e-wrapper.cc.  Yuck!
namespace PCI3E
{
   extern const char* count_mode_to_name[] = {
      "COUNT_MODE_WRAP_FULL",
      "COUNT_MODE_LIMIT",
      "COUNT_MODE_HALT",
      "COUNT_MODE_WRAP_PRESET"
   };
   extern const char* quad_mode_to_name[] = {
      "QUAD_MODE_CLOCK_DIR",
      "QUAD_MODE_X1",
      "QUAD_MODE_X2",
      "QUAD_MODE_X4"
   };
}

class Pds_ConfigDb::EncoderConfig::Private_Data {
 public:
   Private_Data();

   void insert( Pds::LinkedList<Parameter>& pList );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(EncoderConfigType); }

   NumericInt<uint8_t> _chan_num;
   Enumerated<uint8_t> _count_mode;
   Enumerated<uint8_t> _quadrature_mode;
   NumericInt<uint8_t> _input_num;
   Enumerated<uint8_t> _input_rising;   
};

Pds_ConfigDb::EncoderConfig::Private_Data::Private_Data() 
   : _chan_num("Channel Number ", 1, 1, 3),
     _count_mode("Counter Mode ", 0, PCI3E::count_mode_to_name ),
     _quadrature_mode("Quadrature Mode ", 0, PCI3E::quad_mode_to_name ),
     _input_num("Trigger Input Number ", 0, 0, 3),
     _input_rising("Trigger on Rising Edge ", 1, Enums::Bool_Names )
{}

void Pds_ConfigDb::EncoderConfig::Private_Data::insert( Pds::LinkedList<Parameter>& pList )
{
   pList.insert( &_chan_num );
   pList.insert( &_count_mode );
   pList.insert( &_quadrature_mode );
   pList.insert( &_input_num );
   pList.insert( &_input_rising );
}

int Pds_ConfigDb::EncoderConfig::Private_Data::pull( void* from )
{
   EncoderConfigType& encoderConf = * new (from) EncoderConfigType;

   _chan_num.value        = encoderConf._chan_num;
   _count_mode.value      = encoderConf._count_mode;
   _quadrature_mode.value = encoderConf._quadrature_mode;
   _input_num.value       = encoderConf._input_num;
   _input_rising.value    = encoderConf._input_rising;

   return sizeof(EncoderConfigType);
}

int Pds_ConfigDb::EncoderConfig::Private_Data::push(void* to)
{
   new (to) EncoderConfigType( _chan_num.value,
                               _count_mode.value,
                               _quadrature_mode.value,
                               _input_num.value,
                               _input_rising.value );

   return sizeof(EncoderConfigType);
}

Pds_ConfigDb::EncoderConfig::EncoderConfig()
   : Serializer("Encoder_Config"),
     _private_data( new Private_Data )
{
   _private_data->insert(pList);
}

int Pds_ConfigDb::EncoderConfig::readParameters (void* from)
{
   return _private_data->pull(from);
}

int Pds_ConfigDb::EncoderConfig::writeParameters(void* to)
{
   return _private_data->push(to);
}

int Pds_ConfigDb::EncoderConfig::dataSize() const
{
   return _private_data->dataSize();
}

#include "Parameters.icc"
