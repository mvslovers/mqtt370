#include "mqtb370.h"

void mqtb_publish_free(MQTB *mqtb, MQPUB **pmqpub)
{
    MQPUB       *mqpub;
    
    if (!pmqpub) goto quit;
    
    mqpub = *pmqpub;
    if (!mqpub) goto quit;
    
    util_free_char(&mqpub->topic);
    util_free_char((char**)&mqpub->msg);

    free(mqpub);
    *pmqpub = NULL;
    
quit:
    return;
}
