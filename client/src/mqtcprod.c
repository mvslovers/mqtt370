#include "mqtc370.h"

/* mqtc_process_pingresp()
 * Process PINGRESP from client.
 */

int mqtc_process_pingresp(MQTC *mqtc, CPB *pb)
{
    int             rc      = 0;
    unsigned        len     = 0;
    unsigned        pos     = 0;
    unsigned short  msgid;

    if (!pb) goto quit;
    
    // mqtc_packet_dump(mqtc, pb, "PINGRESP");

    rc = 0;

quit:
    // wtof("%s: exit rc=%d", __func__, rc);
    return rc;
}
