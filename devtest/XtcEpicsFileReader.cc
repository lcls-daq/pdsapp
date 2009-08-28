#include <string>
#include "pdsdata/xtc/XtcFileIterator.hh"
#include "pdsdata/xtc/Dgram.hh"
#include "XtcEpicsFileReader.hh"
#include "XtcEpicsIterator.hh"

namespace Pds
{    

XtcEpicsFileReader::XtcEpicsFileReader( char* lcFnXtc ) : 
  _sFnXtc(lcFnXtc)
{
    if ( lcFnXtc == NULL )
        throw std::string("XtcEpicsFileReader::XtcEpicsFileReader(): Invalid parameter(s)");    
}

XtcEpicsFileReader::~XtcEpicsFileReader()
{
}

int XtcEpicsFileReader::doFileRead()
{
    FILE* fileXtc = fopen(_sFnXtc.c_str(),"r");
    
    if (!fileXtc) 
    {
        printf("XtcEpicsFileReader::doFileRead(): Unable to open file %s\n", _sFnXtc.c_str());
        return 1;
    }

    XtcFileIterator xtcFileIter(fileXtc,XtcEpicsIterator::iMaxXtcSize);
    
    while ( Dgram* dg = xtcFileIter.next() )
    {
        printf( "Epics Xtc Size %d\n", dg->xtc.sizeofPayload());
          
        XtcEpicsIterator iter( &(dg->xtc), 0 );
        iter.iterate();
    }

    fclose(fileXtc);
    return 0;
}
  
} // namespace Pds
