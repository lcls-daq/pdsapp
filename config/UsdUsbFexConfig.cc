#define __STDC_LIMIT_MACROS

#include "UsdUsbFexConfig.hh"

#include "pdsapp/config/Parameters.hh"
#include "pdsapp/config/ParameterSet.hh"
#include "pds/config/UsdUsbFexConfigType.hh"

#include <new>

#include <stdint.h>
#include <float.h>
#include <stdio.h>


using namespace Pds_ConfigDb;

class Pds_ConfigDb::UsdUsbFexConfig::Private_Data {
 public:
   Private_Data();
  ~Private_Data();

   void insert( Pds::LinkedList<Parameter>& pList );
   int pull( void* from );
   int push( void* to );
   int dataSize() const
      { return sizeof(UsdUsbFexConfigType); }

  NumericFloat<double>*  _scale  [UsdUsbFexConfigType::NCHANNELS];
  NumericInt<int32_t>*   _offset [UsdUsbFexConfigType::NCHANNELS];
  TextParameter*         _name   [UsdUsbFexConfigType::NCHANNELS];
  char*                  _labels [UsdUsbFexConfigType::NCHANNELS];
};

Pds_ConfigDb::UsdUsbFexConfig::Private_Data::Private_Data() 
{
  for(unsigned i=0; i<UsdUsbFexConfigType::NCHANNELS; i++) {
    _labels[i] = new char[32];
    sprintf(_labels[i],"Chan %d Name",i);
    _name   [i] = new TextParameter       (_labels[i], "", UsdUsbFexConfigType::NAME_CHAR_MAX);
    _scale  [i] = new NumericFloat<double>("Scale      ", 1.0,    DBL_MIN,    DBL_MAX);
    _offset [i] = new NumericInt<int32_t> ("Offset     ",   0,  INT32_MIN,  INT32_MAX);
  }
}

Pds_ConfigDb::UsdUsbFexConfig::Private_Data::~Private_Data() 
{
  for(unsigned i=0; i<UsdUsbFexConfigType::NCHANNELS; i++) {
    delete _scale     [i];
    delete _offset    [i];
    delete _name      [i];
    delete _labels    [i];
  }
}

void Pds_ConfigDb::UsdUsbFexConfig::Private_Data::insert( Pds::LinkedList<Parameter>& pList )
{
  for(unsigned i=0; i<UsdUsbFexConfigType::NCHANNELS; i++) {
    pList.insert( _name   [i] );
    pList.insert( _offset [i] );
    pList.insert( _scale  [i] );
  }
}

int Pds_ConfigDb::UsdUsbFexConfig::Private_Data::pull( void* from )
{
  UsdUsbFexConfigType& cfg = * new (from) UsdUsbFexConfigType;
  
  for(unsigned i=0; i<UsdUsbFexConfigType::NCHANNELS; i++) {
    _scale  [i]->value = cfg.scale  ()[i];
    _offset [i]->value = cfg.offset ()[i];
    strncpy(_name[i]->value, cfg.name(i), UsdUsbFexConfigType::NAME_CHAR_MAX);
  }
  return sizeof(UsdUsbFexConfigType);
}
  
int Pds_ConfigDb::UsdUsbFexConfig::Private_Data::push(void* to)
{
  double  sm[UsdUsbFexConfigType::NCHANNELS];
  int32_t om[UsdUsbFexConfigType::NCHANNELS];
  char    nm[UsdUsbFexConfigType::NCHANNELS][UsdUsbFexConfigType::NAME_CHAR_MAX];   
  for(unsigned i=0; i<UsdUsbFexConfigType::NCHANNELS; i++) {
    sm[i] = _scale  [i]->value;
    om[i] = _offset [i]->value;
    strncpy(nm[i], _name[i]->value, UsdUsbFexConfigType::NAME_CHAR_MAX);
  }
  
  new (to) UsdUsbFexConfigType( (int32_t*) &om, (double*) &sm, (char*) &nm );
  
  return sizeof(UsdUsbFexConfigType);
}
 
Pds_ConfigDb::UsdUsbFexConfig::UsdUsbFexConfig() :
  Serializer("UsdUsbFexConfig"),
  _private_data( new Private_Data )
{
  _private_data->insert(pList);
}

int Pds_ConfigDb::UsdUsbFexConfig::readParameters (void* from)
{
  return _private_data->pull(from);
}

int Pds_ConfigDb::UsdUsbFexConfig::writeParameters(void* to)
{
  return _private_data->push(to);
}

int Pds_ConfigDb::UsdUsbFexConfig::dataSize() const
{
  return _private_data->dataSize();
}

#include "Parameters.icc"
