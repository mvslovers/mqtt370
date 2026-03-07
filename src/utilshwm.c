#include "mqttutil.h"

/* returns high water mark for stack of given size in bytes */
unsigned util_stack_hwm(void *stk, unsigned bytes)
{
    unsigned    *ua     = (unsigned *)(((unsigned)stk + 3) & 0x00FFFFFC);
    unsigned    words   = (bytes+3) / 4;
    unsigned    hwm;
    unsigned    n;
    
    /* examine each word in the stack for non-zero value */
    for(n=0, hwm=0; n < words; n++) {
        if (ua[n]) hwm = n; /* non-zero value found */
    }
    
    return (hwm * 4);
}
