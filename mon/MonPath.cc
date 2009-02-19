#include <string.h>

#include "MonPath.hh"

using namespace Pds;

MonPath::MonPath(const char* name, char separator) :
  _current(_name),
  _last(_current)
{
  char* tmp = _name;
  if (name) {
    const char* end = _name+NameSize;
    do {
      if (*name == separator) {
	*tmp = 0;
      } else {
	*tmp = *name;
      }
      ++tmp;
      ++name;
    } while (*name && tmp < end);
    *tmp = 0;
    _last = tmp;
  }
}

const char* MonPath::nextdir()
{
  const char* tmp = 0;
  if (_current < _last) {
    tmp = _current;
    do {++_current;} while (*_current);
    do {++_current;} while (*_current == 0 && _current < _last);
  }
  return tmp;
}

void MonPath::rewind() {_current = _name;}

unsigned MonPath::split(const char* name, 
			    char* buf, 
			    const char** ptr, 
			    char separator)
{
  unsigned found=0;
  strcpy(buf, name);
  *ptr++ = buf;
  ++found;
  do {
    if (*buf == separator) {
      *buf = 0;
      *ptr++ = buf+1;
      ++found;
    }
    ++buf;
  } while (*buf);
  return found;
}

unsigned MonPath::split(const char* name, 
			    const char** ptr, 
			    unsigned howmany)
{
  unsigned found=0;
  for (unsigned i=0; i<howmany; i++, ptr++) {
    *ptr = name;
    if (*name) {
      ++found;
      do {
	++name;
      } while (*name);
      ++name;
    }
  }
  return found;
}
