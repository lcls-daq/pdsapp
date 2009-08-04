#include <stdio.h>
#include <getopt.h>
#include "cadef.h"
#include "XtcEpicsFileReader.hh"
#include "XtcEpicsMonitor.hh"

namespace Pds
{
static const char sXtcEpicsTestVersion[] = "2.00";

// forward declarations
int xtcEpicsReadTest( char* sFnXtc );
int xtcEpicsMonitorTest( char* sFnXtc, int iNumPv, char* lsPvName[]);

}

using namespace Pds;

void showUsageXtcEpicsTest()
{
    printf( "Usage:  xtcEpicsTest  [-r|--read] [-v|--version] [-h|--help] <xtc filename>  <pv1 name> [ <pv2 name> ... ]\n" 
      "  Options:\n"
      "    -r      Read Xtc file and print out the values\n"
      "    -v      Show file version\n"
      "    -h      Show Usage\n"
      );
}

void showVersionXtcEpicsTest()
{
    printf( "Version:  xtcEpicsTest  Ver %s\n", sXtcEpicsTestVersion );
}

int main(int argc,char **argv)
{
    bool bReadXtc = false;
    
    int iOptionIndex = 0;
    struct option loOptions[] = 
    {
       {"ver",  0, 0, 'v'},
       {"read", 0, 0, 'r'},
       {"help", 0, 0, 'h'},
       {0,      0, 0, 0  }
    };
    
    while ( int opt = getopt_long(argc, argv, ":vhr", loOptions, &iOptionIndex ) )
    {
        if ( opt == -1 ) break;
        
        switch (opt) 
        {
        case 'v':               /* Print usage */
            showVersionXtcEpicsTest();
            return 0;
        case 'r':               /* Terse output mode */
            bReadXtc = true;
            break;
        case '?':               /* Terse output mode */
            printf( "xtcEpicsTest:main(): Unknown option: %c\n", optopt );
            break;
        case ':':               /* Terse output mode */
            printf( "xtcEpicsTest:main(): Missing argument for %c\n", optopt );
            break;
        default:
        case 'h':               /* Print usage */
            showUsageXtcEpicsTest();
            return 0;
        }
    }
    
	argc -= optind;
	argv += optind;
    
    if ( bReadXtc )
    {
        if ( argc < 1 ) 
        {
            showUsageXtcEpicsTest();
            return 1;        
        }
        
        xtcEpicsReadTest( argv[0] );
    }
    else
    {    
        if ( argc < 2 ) 
        {        
            showUsageXtcEpicsTest();
            return 1;        
        }
        
        xtcEpicsMonitorTest( argv[0], argc-1, &argv[1] );
    }
                    
    return 0;
}


using std::string;

namespace Pds
{
    
int xtcEpicsReadTest( char* sFnXtc )
{
    if ( sFnXtc == NULL )
    {
        printf( "xtcEpicsReadTest(): Invalid argument(s)\n" );
        return 1;
    }
    
    try
    {    
        XtcEpicsFileReader xtcEpicsFile(sFnXtc);
        xtcEpicsFile.doFileRead();
    }
    catch (string& sError)
    {
        printf( "xtcEpicsReadTest():XtcEpicsFileReader failed: %s\n", sError.c_str() );
    }
    
    return 0;
}

int xtcEpicsMonitorTest( char* sFnXtc, int iNumPv, char* lsPvName[])
{
    if ( sFnXtc == NULL || iNumPv <= 0 || lsPvName == NULL )
    {
        printf( "xtcEpicsMonitorTest(): Invalid argument(s)\n" );
        return 1;
    }
    
    int iCaStatus = ca_context_create(ca_disable_preemptive_callback);
    if ( iCaStatus != ECA_NORMAL ) 
    {
        printf( "xtcEpicsTest()::ca_context_create() failed, CA errmsg: %s\n", ca_message(iCaStatus));
        return 2;
    }
    
    try
    {
        XtcEpicsMonitor xtcEpicsMonitor(sFnXtc, iNumPv, lsPvName);
        xtcEpicsMonitor.runMonitorLoop();
    }
    catch (string& sError)
    {
        printf( "xtcEpicsMonitorTest(): xtcEpicsMonitor failed: %s\n", sError.c_str() );
    }
                        
    ca_context_destroy(); // no return value
    
    return 0;
}

} // namespace Pds
