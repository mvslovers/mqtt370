#include "mqttutil.h"

int util_length_encode(unsigned length, unsigned char *buf)
{
    int     bytes   = 0;

    do {
        unsigned char digit = length % 128;
        length = length / 128;
        // if there are more digits to encode, set the top bit of this digit
        if ( length > 0 ) {
            digit |= 0x80;
        }
        buf[bytes++] = digit;
    } while ( length > 0 );

    return bytes;
}
  
