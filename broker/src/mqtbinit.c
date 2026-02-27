#include "mqtb370.h"

static void close_stale_port(int port);

int mqtb_initialize(MQTB *mqtb)
{
    int                 rc      = 8;
    MQTBC               *mqtbc  = mqtb->mqtbc;
	int					sock;
	int					i;
    int                 addrlen;
    struct sockaddr     addr;
    struct sockaddr_in  serv_addr;
    unsigned char       *a;
    
    if (!mqtbc) {
        wtof("MQTTD200E No Configuration");
        goto quit;
    }

    if (mqtbc->tz && *mqtbc->tz) {
        /* update TZ environment variable */
        setenv("TZ", mqtbc->tz, 1);
    }

    /* set the time zone */
    tzset();

    /* close socket for anyone using our configured port number */
    close_stale_port(mqtbc->port);

    if (mqtbc->retain_dataset) {
        i = mqtb_retain_load(mqtb, mqtbc->retain_dataset);
        if (i) wtof("%s: mqtb_retain_load() rc=%d", __func__, i);
    }

    /* create listener socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        /* failed */
		wtof("MQTTD201E socket() failed, rc=%d, error=%d", sock, errno);
        goto quit;
    }

    /* bind socket to IP address and port */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family        = AF_INET;
    serv_addr.sin_addr.s_addr   = INADDR_ANY;
    serv_addr.sin_port          = htons(mqtbc->port);

    rc = bind(sock, &serv_addr, sizeof(serv_addr));
    if (rc < 0) {
		int error = errno;
		wtof("MQTTD202E bind() failed for MQTTD port, rc=%d, error=%d", rc, errno);
        if (error==EADDRINUSE) for (i=0; i < mqtbc->bind_tries; i++) {
			wtof("MQTTD203I EADDRINUSE, waiting for TCPIP to release MQTTD port=%d", mqtbc->port);
			sleep(mqtbc->bind_sleep);
			rc = bind(sock, &serv_addr, sizeof(serv_addr));
			if (rc >= 0) break;
			error = errno;
		}
		if (rc < 0) {
			wtof("MQTTD203E bind() failed for MQTTD port, rc=%d, error=%d", rc, errno);
			closesocket(sock);
			goto quit;
		}
    }

    /* listen for connection */
    rc = listen(sock, mqtbc->listenq);
    if (rc < 0) {
		wtof("FTPD0004E listen() failed, rc=%d, error=%d", rc, errno);
        closesocket(sock);
        goto quit;
    }

    mqtb->listen = sock;

    mqtb->listen_thread = cthread_create_ex(mqtb_listen_thread, mqtb, NULL, 16*1024);
    if (!mqtb->listen_thread) {
        wtof("MQTTD201E Unable to create listener thread");
        goto quit;
    }

    mqtb->wait_thread = cthread_create_ex(mqtb_wait_thread, mqtb, NULL, 16*1024);
    if (!mqtb->wait_thread) {
        wtof("MQTTD201E Unable to create waiter thread");
        goto quit;
    }

    mqtb->publish_thread = cthread_create_ex(mqtb_publish_thread, mqtb, NULL, 16*1024);
    if (!mqtb->publish_thread) {
        wtof("MQTTD201E Unable to create publish thread");
        goto quit;
    }

    mqtb->monitor_thread = cthread_create_ex(mqtb_monitor_thread, mqtb, NULL, 16*1024);
    if (!mqtb->monitor_thread) {
        wtof("MQTTD201E Unable to create monitor thread");
        goto quit;
    }

    mqtb->session_mgr = cthread_manager_init(mqtbc->maxtask, mqtb_session_thread, mqtb, 16*1024);
    if (!mqtb->session_mgr) {
        /* gripe and continue */
        wtof("MQTTD034W Unable to create session manager.");
        goto quit;
    }

#if 0
    mqtb->publish_mgr = cthread_manager_init(mqtbc->maxtask, mqtb_publish_thread, mqtb, 16*1024);
    if (!mqtb->publish_mgr) {
        /* gripe and continue */
        wtof("MQTTD034W Unable to create publish manager.");
        goto quit;
    }

    lock(mqtb->publish_mgr, LOCK_EXC);
    mqtb->publish_mgr->maxtask = 1;
	mqtb->publish_mgr->mintask = 1;
	cthread_post(&mqtb->publish_mgr->wait, CTHDMGR_POST_DATA);
    unlock(mqtb->publish_mgr, LOCK_EXC);
#endif


    rc = 0;

quit:
    return rc;
}

static void
close_stale_port(int port)
{
	int					rc;
	int					i;
    int                 addrlen;
    struct sockaddr     addr;
    struct sockaddr_in  serv_addr;

	for(i = 1; i < FD_SETSIZE; i++) {
		addrlen = sizeof(addr);
		rc = getsockname(i, &addr, &addrlen);
		if (rc==0) {
			struct sockaddr_in *in = (struct sockaddr_in*)&addr;
			if (in->sin_port == port) {
				wtof("MQTTD027I Closing stale socket %d on port:%u\n",
					i, in->sin_port);
				closesocket(i);
				sleep(2);	/* give the underlying host a chance to cleanup the socket */
				break;
			}
		}
	}
}

