#include "mqttutil.h"

int util_socket_peer(int sock, char *ip, char *port)
{
    int                 rc      = 4;
    struct sockaddr     addr;
    struct sockaddr_in  *in = (struct sockaddr_in*)&addr;
    int                 addrlen = sizeof(addr);

    if (ip) *ip = 0;
    if (port) *port = 0;
    
    if (sock < 0) goto quit;

    rc = getpeername(sock, &addr, &addrlen);
    if (rc < 0) goto quit;

    // wtodumpf(&addr, sizeof(addr), "%s: ADDR", __func__);

    if (ip)     util_ntoa(in->sin_addr.s_addr, ip);
    if (port)   sprintf(port, "%u", in->sin_port);

quit:
    return rc;
}
