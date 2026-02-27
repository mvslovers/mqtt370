#include "mqtb370.h"

int mqtb_session_getc(MQTBS *mqtbs)
{
    fd_set          read;
    timeval         wait;
    int             rc;
    int             maxsock;
    int             len;
    unsigned char   buf[4];

    len = recv(mqtbs->sock, buf, 1, 0);
    if (len > 0) {
        /* we read a single character */
        len = (int) buf[0];
        goto quit;
    }

    if (len == 0) {
        /* nothing was read */
        // wtof("%s: recv() len=%d errno=%d", __func__, len, errno);
        len = -1; 
    }

    if (len < 0) {
        /* socket error, make sure errno is set */
        if (!errno) errno = EIO;
        // wtof("%s: recv() len=%d errno=%d", __func__, len, errno);
        goto quit;
    }

quit:
    return len;
}
