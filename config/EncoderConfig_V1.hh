#ifndef ENCODERCONFIG_V1_HH
#define ENCODERCONFIG_V1_HH

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb
{
   class EncoderConfig_V1;
}

class Pds_ConfigDb::EncoderConfig_V1
   : public Serializer
{
 public:
   EncoderConfig_V1();
   ~EncoderConfig_V1() {}

   int  readParameters (void* from);
   int  writeParameters(void* to);
   int  dataSize       () const;

 private:
   class Private_Data;
   Private_Data* _private_data;
};

#endif
