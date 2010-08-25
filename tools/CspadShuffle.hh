#ifndef Pds_CspadShuffle_hh
#define Pds_CspadShuffle_hh

namespace Pds {

class Datagram;

class CspadShuffle {
public:
  static void shuffle(Datagram& dg);
  //  static int  shuffle(void *in, void *out, unsigned int nelements);
};

}
#endif
