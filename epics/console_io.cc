#include <stdio.h>
#include "console_io.hh"

/**
 * Non-blocking console/terminal IO
 */

namespace Pds {

/*
 * public member functions
 */
 
void ConsoleIO::resetTerminalMode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
    
    bNonBlockingTerminalModeEnabled = false;
}

void ConsoleIO::setNonBlockingTerminalMode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);

    bNonBlockingTerminalModeEnabled = true;
}

/**
 * Perform a non-blocking read from keyboard (stdin)
 * 
 * @param pRetChar  The char input from keyboard
 * @return  0 if no char is read, 1 if a char is read from stdin
 */
int ConsoleIO::kbhit(char* pRetChar)
{   
    // Pre-condition : The non-blocking mode has to be enabled
    bool bAutoNonBlockingMode = !bNonBlockingTerminalModeEnabled;
    if (bAutoNonBlockingMode)
        setNonBlockingTerminalMode();

    struct timeval tv = { 0L, 0L };
    fd_set fds;
        
    FD_SET(0, &fds);
    int iError = select(1, &fds, NULL, NULL, &tv);

    
    if (iError == 0 ) 
    {
        if (bAutoNonBlockingMode)
            resetTerminalMode();        
        return 0; 
    }
    
    // consume the char and return it
    getch(pRetChar);
    // Post-condition : Reset the mode to original one
    if (bAutoNonBlockingMode)
        resetTerminalMode();
    
    return 1;
}

/**
 * Perform a blocking read from keyboard (stdin)
 * 
 * @param pRetChar  The char input from keyboard
 * @return  0 if no char is read, 1 if a char is read from stdin
 */
int ConsoleIO::getch(char* pRetChar)
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0)
        return 0;
        
    if (pRetChar!=NULL)
        *pRetChar = c;
        
    return 1;
}

/*
 * private static variables
 */
struct termios ConsoleIO::orig_termios;
bool ConsoleIO::bNonBlockingTerminalModeEnabled = false;

} // namespace Pds

//using Pds::ConsoleIO;

/*
// Code for testing 

int main()
{   
    ConsoleIO::setNonBlockingTerminalMode();

    char c;
    while (1) 
    {
        if ( ConsoleIO::kbhit(&c) == 0 )
            continue;
            
        if ( c == 13 )
            break;
            
        printf( "\r[%c %d]\n", c, c );
    }
    
    ConsoleIO::resetTerminalMode();
}
*/

