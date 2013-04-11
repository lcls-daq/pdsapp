#include "UsdUsbConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/UsdUsbConfigType.hh"

#include <new>

#include <stdio.h>

using namespace Pds_ConfigDb;

class Pds_ConfigDb::UsdUsbConfig::Private_Data {
 public:
   Private_Data();
  ~Private_Data();

   void insert( Pds::LinkedList<Parameter>& pList );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(UsdUsbConfigType); }

  Enumerated<UsdUsbConfigType::Count_Mode>* _count_mode[UsdUsbConfigType::NCHANNELS];
  Enumerated<UsdUsbConfigType::Quad_Mode >* _quad_mode [UsdUsbConfigType::NCHANNELS];
  char* _labels[UsdUsbConfigType::NCHANNELS];
};

Pds_ConfigDb::UsdUsbConfig::Private_Data::Private_Data() 
{
  for(unsigned i=0; i<UsdUsbConfigType::NCHANNELS; i++) {
    _labels[i] = new char[32];
    sprintf(_labels[i],"Chan %d",i);
    _count_mode[i] = new Enumerated<UsdUsbConfigType::Count_Mode>( _labels[i],
								   UsdUsbConfigType::WRAP_FULL,
								   UsdUsbConfigType::count_mode_labels() );
    _quad_mode [i] =  new Enumerated<UsdUsbConfigType::Quad_Mode >( "",
								    UsdUsbConfigType::X4,
								    UsdUsbConfigType::quad_mode_labels() );
  }
}

Pds_ConfigDb::UsdUsbConfig::Private_Data::~Private_Data() 
{
  for(unsigned i=0; i<UsdUsbConfigType::NCHANNELS; i++) {
    delete _count_mode[i];
    delete _quad_mode [i];
    delete _labels    [i];
  }
}

void Pds_ConfigDb::UsdUsbConfig::Private_Data::insert( Pds::LinkedList<Parameter>& pList )
{
  for(unsigned i=0; i<UsdUsbConfigType::NCHANNELS; i++) {
    pList.insert( _count_mode[i] );
    pList.insert( _quad_mode [i] );
  }
}

int Pds_ConfigDb::UsdUsbConfig::Private_Data::pull( void* from )
{
  UsdUsbConfigType& cfg = * new (from) UsdUsbConfigType;
  
  for(unsigned i=0; i<UsdUsbConfigType::NCHANNELS; i++) {
    _count_mode[i]->value = cfg.counting_mode  (i);
    _quad_mode [i]->value = cfg.quadrature_mode(i);
  }
  return sizeof(UsdUsbConfigType);
}
  
int Pds_ConfigDb::UsdUsbConfig::Private_Data::push(void* to)
{
  UsdUsbConfigType::Count_Mode cm[UsdUsbConfigType::NCHANNELS];
  UsdUsbConfigType::Quad_Mode  qm[UsdUsbConfigType::NCHANNELS];
  for(unsigned i=0; i<UsdUsbConfigType::NCHANNELS; i++) {
    cm[i] = _count_mode[i]->value;
    qm[i] = _quad_mode [i]->value;
  }
  
  new (to) UsdUsbConfigType( cm, qm );
  
  return sizeof(UsdUsbConfigType);
}
 
Pds_ConfigDb::UsdUsbConfig::UsdUsbConfig() :
  Serializer("UsdUsbConfig"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int Pds_ConfigDb::UsdUsbConfig::readParameters (void* from)
{
  return _private_data->pull(from);
}

int Pds_ConfigDb::UsdUsbConfig::writeParameters(void* to)
{
  return _private_data->push(to);
}

int Pds_ConfigDb::UsdUsbConfig::dataSize() const
{
  return _private_data->dataSize();
}

#include "Parameters.icc"

template class Enumerated<UsdUsbConfigType::Count_Mode>;
template class Enumerated<UsdUsbConfigType::Quad_Mode>;
