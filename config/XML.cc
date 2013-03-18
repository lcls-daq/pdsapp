#include "pdsapp/config/XML.hh"

using namespace Pds_ConfigDb::XML;

const int StringSize = 64;
const int IntSize = 16;
const int DoubleSize = 16;

static unsigned indent   = 0;
static bool     tag_open = false;

static const char ESC = '\\';
static const char* SPECIAL = "\\<";
static const char* CTRUE  = "True";
static const char* CFALSE = "False";

static char buff[8*1024];

TagIterator::TagIterator(const char*& p) :
  _p (p)
{
  _tag = IO::extract_tag(p);
}

bool  TagIterator::end() const
{
  return _tag.name.empty();
}

TagIterator::operator const StartTag*() const
{
  return &_tag;
}

TagIterator& TagIterator::operator++(int)
{
  if (!end()) {
    std::string stop_tag = IO::extract_tag(_p).element.substr(1);
    if (stop_tag != _tag.element)
      printf("Mismatch tags %s/%s\n",_tag.element.c_str(),stop_tag.c_str());
    const char* p = _p;
    _tag = IO::extract_tag(p);
    if (!end())
      _p = p;
  }
    
  return *this;
}

void IO::insert(char*& p, const StartTag& tag)
{
  *p++ = '\n';
  for(unsigned i=0; i<2*indent; i++)
    *p++ = ' ';

  p += sprintf(p, "<%s name=\"%s\">", tag.element.c_str(), tag.name.c_str());

  indent++;
  tag_open = true;
}

void IO::insert(char*& p, const StopTag& tag)
{
  indent--;
  if (!tag_open) {
    *p++ = '\n';
    for(unsigned i=0; i<2*indent; i++)
      *p++ = ' ';
  }

  p += sprintf(p, "</%s>", tag.element.c_str());

  tag_open = false;
}

void IO::insert(char*& p, const std::string& i)
{
  // escape special characters
  std::string s(i);
  size_t pos = std::string::npos;
  while((pos = s.find_last_of(SPECIAL,pos)) != std::string::npos)
    s.insert(pos,1,ESC);
  p += sprintf(p, "%s", s.c_str());
}

void IO::insert(char*& p, int s)
{
  p += sprintf(p,"%d",s);
}

void IO::insert(char*& p, unsigned s)
{
  p += sprintf(p,"%d",s);
}

void IO::insert(char*& p, double s)
{
  p += sprintf(p,"%g",s);
}

void IO::insert(char*& p, bool s)
{
  p += sprintf(p,"%s",s?CTRUE:CFALSE);
}

void IO::insert(char*& p, void* b, int len)
{
  char* end = (char*)b + len;
  for(char* bp = (char*)b; bp<end; bp++)
    p += sprintf(p, "%02hhx", *bp);
}

StartTag IO::extract_tag(const char*& p)
{
  while( *p++ != '<' ) ;

  StartTag tag;
  while( *p != ' ' && *p != '>')
    tag.element.push_back(*p++);
  if (*p++ != '>') {
    while( *p++ != '\"') ;
    while( *p != '\"' )
      tag.name.push_back(*p++);
    while( *p++ != '>') ;
  }

  if (*p == '\n') p++;

  return tag;
}

std::string IO::extract_s(const char*& p)
{
  std::string v;
  while(*p != '<') {
    if (*p == ESC) p++;
    v.append(1,*p++);
  }
  return v;
}

int IO::extract_i(const char*& p)
{
  char* endPtr;
  return strtol(p, &endPtr, 0);
  p = endPtr;
}

double IO::extract_d(const char*& p)
{
  char* endPtr;
  return strtod(p, &endPtr);
  p = endPtr;
}

bool IO::extract_b(const char*& p)
{
  std::string v = IO::extract_s(p);
  return (v == CTRUE);
}

void* IO::extract_op(const char*& p)
{
  char* b = buff;
  while(*p != '<') {
    sscanf(p, "%02hhx", b);
    p += 2;
    b++;
  }
  return buff;
}
