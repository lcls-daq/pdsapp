#ifndef PDS_PNCCDSHUFFLE
#define PDS_PNCCDSHUFFLE

namespace Pds {

class Datagram;

class PnccdShuffle {
public:
  static void shuffle(Datagram& dg);
  static int  shuffle(void *in, void *out, unsigned int nelements);
};

}
#endif
