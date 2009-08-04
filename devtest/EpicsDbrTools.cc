#include <stdio.h>
#include "EpicsDbrTools.hh"

namespace Pds
{

namespace EpicsDbrTools    
{
    
const char* sDbrPrintfFormat[] =
{
    "%s",   // DBR_STRING   returns a NULL terminated string
    "%d",   // DBR_SHORT  &&  DBR_INT      returns an unsigned short
    "%f",   // DBR_FLOAT    returns an IEEE floating point value
    "%d",   // DBR_ENUM returns an unsigned short which is the enum item
    "%d",   // DBR_CHAR    returns an unsigned char
    "%ld",  // DBR_LONG    returns an unsigned long
    "%lf"   // DBR_DOUBLE  returns a double precision floating point number
};

} // namespace EpicsDbrTools
    
} // namespace Pds

