#include "mqtb370.h"

MQTBC *mqtbc_new(MQTB *mqtb)
{
    MQTBC   *mqtbc  = NULL;
    
    if (!mqtb) goto quit;

    mqtbc = calloc(1, sizeof(MQTBC));
    if (!mqtbc) goto quit;

    strcpy(mqtbc->eye, MQTBC_EYE);

quit:
    return mqtbc;
}
