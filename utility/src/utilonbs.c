#include "mqttutil.h"

int util_open_nb_socket(const char* host, const char* port) 
{
	int		 	sock	= -1;
	int			rc;
	unsigned 	addr;
    struct sockaddr_in  sockaddr_in = {0};
    int			len = sizeof(sockaddr_in);
    unsigned    u;
    unsigned short uport;

	addr = getaddrbyname(host);
	if (!addr) {
		wtof("%s: Unable to resolve host name: \"%s\"", __func__, host);
		goto quit;
	}
		
	u = strtoul(port, NULL, 0);
    if (u == 0 || u > 0xFFFF) {
        wtof("%s: Invalid port number \"%s\".", __func__, port);
        goto quit;
    }
	uport = (unsigned short) u;

    /* get a socket from TCPIP */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		wtof("%s: Unable to create socket.", __func__);
		goto quit;
	}

    /* prepare to connect socket to address and port */
	sockaddr_in.sin_family 		= AF_INET;
	sockaddr_in.sin_port   		= htons(uport);
	sockaddr_in.sin_addr.s_addr = htonl(addr);

	rc = connect(sock, &sockaddr_in, len);
	if (rc < 0) {
		int error = errno;
		wtof("%s: Open \"%s\" port \"%s\" failed. \"%s\"", 
            __func__, host, port, strerror(error));
		closesocket(sock);
		sock = -1;
		goto quit;
	}

#if 0
    /* success */
    wtof("%s: Open \"%s\" port \"%s\" success, socket=%d", 
        __func__, host, port, sock);
#endif

    /* set non-blocking on the new socket */
    len = 1;
    if (ioctlsocket(sock, FIONBIO, &len)) {
        wtof("%s: Unable to set non-blocking I/O for socket=%d",
            __func__, sock);
    }

quit:	
	return sock;
}
