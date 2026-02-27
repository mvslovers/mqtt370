#include "mqtc370.h"

MQTC *mqtc_new_client(void)
{
    MQTC    *mqtc   = calloc(1, sizeof(MQTC));
    
    if (mqtc) mqtc_init_client(mqtc);
    
    return mqtc;
}
