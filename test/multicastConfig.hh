namespace Pds
{

namespace ConfigurationMulticast
{
    static const unsigned int uDefaultAddr = 239<<24 | 255<<16 | 24<<8 | 1; /// multicast address
    static const unsigned int uDefaultPort = 50000;
    static const unsigned int uDefaultMaxDataSize = 72; /// in bytes
    static const unsigned char ucDefaultTTL = 32; /// minimum: 1 + (# of routers in the middle)
    static const unsigned int uDefaultSendDataSize = 52; /// in bytes
}

}
