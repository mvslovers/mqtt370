#include "mqtc370.h"

int mqtc_packet_dump(MQTC *mqtc, CPB *pb, const char *title)
{
    int         rc      = 0;

    if (pb) {
        if (!title) title = "UNKNOWN";
        wtodumpf(pb->buf, pb->bufsize, "%s: Packet Buffer", title);
    }
    
    return rc;
}
