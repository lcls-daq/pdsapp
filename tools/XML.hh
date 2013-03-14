#ifndef Pds_XML_hh
#define Pds_XML_hh

#include <string>

namespace Pds {
  namespace XML {
    class StartTag {
    public:
      StartTag() {}
      StartTag(const std::string element_, const std::string name_) :
        element(element_), name   (name_) {}

    public:
      std::string element;
      std::string name;
    };

    class StopTag {
    public:
      StopTag(const std::string element_) : element(element_) {}
    public:
      const std::string element;
    };

    class TagIterator {
    public:
      TagIterator(const char*&);
    public:
      bool            end() const;
      operator  const StartTag*() const;
      TagIterator&    operator++(int);
    private:
      const char*& _p;
      StartTag     _tag;
    };

    class IO {
    public:
      static void insert(char*&, const StartTag&);
      static void insert(char*&, const StopTag&);
      static void insert(char*&, const std::string&);
      static void insert(char*&, unsigned);
      static void insert(char*&, int);
      static void insert(char*&, double);
      static void insert(char*&, bool);
      static void insert(char*&, void*, int);
      static StartTag    extract_tag(const char*&);
      static int         extract_i(const char*&);
      static double      extract_d(const char*&);
      static std::string extract_s(const char*&);
      static bool        extract_b(const char*&);
      static void*       extract_op(const char*&);
    };

  };
}

#define XML_iterate_open(pvar,tvar)                      \
  for(Pds::XML::TagIterator it(pvar); !it.end(); it++) { \
    const Pds::XML::StartTag& tvar = *it;                

#define XML_iterate_close(where,tvar)           \
  else                                          \
    printf(#where " unknown tag %s/%s\n",       \
           tvar.element.c_str(),                \
           tvar.name.c_str());                  \
  }

#define XML_insert( p, element, name, routine ) {               \
    QtPersistent::insert(p, Pds::XML::StartTag(element,name));  \
    { routine; }                                                \
    QtPersistent::insert(p, Pds::XML::StopTag (element)); }


#endif
