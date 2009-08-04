#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <termios.h>

/**
 * C++ Library for providing non-blocking console/terminal IO
 */

namespace Pds {

class ConsoleIO
{	
public:
	static void resetTerminalMode();
	static void setNonBlockingTerminalMode();
	static int kbhit(char* pRetChar);
	static int getch(char* pRetChar);

private:
	static struct termios orig_termios;
	static bool bNonBlockingTerminalModeEnabled;       
};

} // namespace Pds
