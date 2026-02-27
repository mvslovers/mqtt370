#include "mqttutil.h"

int util_length_decode(const unsigned char *buf, unsigned *length)
{
    int         bytes       = 0;
    unsigned    value       = 0;
    unsigned    multiplier  = 1;

    for(bytes=1; bytes < 5; bytes++) {
        unsigned char digit = buf[bytes];
        
        value += (digit & 0x7F) * multiplier;
        multiplier *= 128;
        if ((digit & 0x80) == 0) break;
    }

    if (length) *length = value;
    if (bytes) bytes++; /* include first byte 0 in count */
    return bytes;
}
