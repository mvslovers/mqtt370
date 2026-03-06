#include "mqtc370.h"

int mqtc_open_client(MQTC *mqtc, const char *broker, const char *port)
{
    int         rc          = 8;
    int         sock;
#if 0
    char        hip[40];
    char        hport[10];
    char        pip[40];
    char        pport[10];
#endif

    if (!mqtc) goto quit;

    if (!broker) goto quit;

    if (!port)  port = "1883";

    /* open a non-blocking socket connected to MQTT broker */
    sock = util_open_nb_socket(broker, port);
    if (sock < 0) {
        wtof("%s: unable to open socket to MQTT broker", __func__);
        goto quit;
    }
#if 0    
    util_socket_name(sock, hip, hport);
    util_socket_peer(sock, pip, pport);
    wtof("Socket %d Host %s:%s Peer %s:%s", sock, hip, hport, pip, pport);
#endif    
    /* non-blocking socket opened to broker */
    mqtc->sock          = sock;
    mqtc->broker_host   = strdup(broker);
    mqtc->broker_port   = strdup(port);
    
    rc = 0;

quit:
    return rc;
}
