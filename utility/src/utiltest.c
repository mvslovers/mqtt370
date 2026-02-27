#include "mqttutil.h"

int main(int argc, char **argv)
{
    unsigned    i;
    unsigned    x;
    
    for(i=0; i < 0x7FFFFFFF; i++) {
        x = util_prng();
        if (x & 0xFFFF) continue;
        wtof("i=%u x=%p", i, x);
        break;
    }
    
    return 0;
}
