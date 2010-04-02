#ifndef ENCODERCONFIG_HH
#define ENCODERCONFIG_HH

#include "pdsapp/config/Serializer.hh"

namespace Pds_ConfigDb
{
   class EncoderConfig;
}

class Pds_ConfigDb::EncoderConfig
   : public Serializer
{
 public:
   EncoderConfig();
   ~EncoderConfig() {}

   int  readParameters (void* from);
   int  writeParameters(void* to);
   int  dataSize       () const;

 private:
   class Private_Data;
   Private_Data* _private_data;
};

#endif
