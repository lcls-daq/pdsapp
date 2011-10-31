#ifndef Pds_CspadShuffle_hh
#define Pds_CspadShuffle_hh

namespace Pds {

class Dgram;

class CspadShuffle {
public:
  static void shuffle(Dgram& dg);
  //  static int  shuffle(void *in, void *out, unsigned int nelements);
};

}
#endif
