namespace Pds
{

namespace ConfigurationMulticast
{
    static const unsigned int uDefaultAddr = 239<<24 | 255<<16 | 24<<8 | 1; /// multicast address
    static const unsigned int uDefaultPort = 10148;
    static const unsigned int uDefaultMaxDataSize = 512; /// in bytes
    static const unsigned char ucDefaultTTL = 32; /// minimum: 1 + (# of routers in the middle)
}

}
