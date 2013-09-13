#include "pdsapp/control/Preferences.hh"

#include <sstream>
#include <string>

using namespace Pds;

Preferences::Preferences(const char* title,
                         unsigned    platform,
                         const char* mode)
{
  std::stringstream o;
  char* home = getenv("HOME");
  if (home)
    o << home << '/';
  o << "." << title << " for platform " << platform;

  _f = fopen(o.str().c_str(),mode);
  if (_f) {
    printf("Opened %s in %s mode\n",o.str().c_str(),mode);
  }
  else {
    std::string msg("Failed to open ");
    msg += o.str();
    perror(msg.c_str());
  }
}

Preferences::~Preferences() { if (_f) fclose(_f); }

FILE* Preferences::file() { return _f; }

