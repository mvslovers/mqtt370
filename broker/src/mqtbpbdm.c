#include "mqtb370.h"

int mqtb_packet_dump(MQTB *mqtb, MQTBS *mqtbs, PB *pb, const char *title)
{
    int         rc      = 0;

    if (pb) {
        if (!title) title = "UNKNOWN";
        wtodumpf(pb, sizeof(PB), "%s: PB", __func__);
        wtodumpf(pb->buf, pb->bufsize, "%s: Packet Buffer", title);
    }
    
    return rc;
}
