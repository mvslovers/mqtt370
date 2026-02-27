#ifndef MQTC370_H
#define MQTC370_H

/* MQTT370 project - Client Header 
 * 
 */

#include "mqttutil.h"

typedef struct mqtc         MQTC;   /* MQTT370 Client               */
typedef struct cpacketbuf   CPB;    /* MQTT370 Client Packet Buffer */
typedef struct cretrybuf    RPB;    /* MQTT370 Client Retry Packet  */

/* MQTT370 Client               */
struct mqtc {
    char            eye[7];         /* 00 eye catcher for dumps     */
#define MQTC_EYE    "*MQTC*"        /* ... eye catcher for dumps    */
    volatile unsigned char flags;   /* 07 processing flags          */
#define MQTC_FLAG_ERROR     0x80    /* ,,, processing error         */
#define MQTC_FLAG_SESSION   0x40    /* ... session established      */
#define MQTC_FLAG_WAITRESP  0x20    /* ... wait for response        */
#define MQTC_FLAG_RECONNECT 0x10    /* ... reconnect in progress    */
#define MQTC_FLAG_SHUTDOWN  0x08    /* ... stop processing thread   */
    int             sock;           /* 08 TCPIP socket              */
    CTHDTASK        *task;          /* 0C socket processing thread  */

    __64            taskdisp;       /* 10 task dispatch counter     */
    time64_t        tasktime;       /* 18 task time of dispatch     */

    unsigned        task_ecb;       /* 20 task ecb                  */
    char            *broker_host;   /* 24 broker host name          */
    char            *broker_port;   /* 28 broker port number        */
    char            *clientid;      /* 2C client id                 */
    
    char            *userid;        /* 30 session user id           */
    char            *password;      /* 34 session password          */
    char            *will_topic;    /* 35 will topic                */
    char            *will_msg;      /* 3C will message              */
    
    unsigned        will_len;       /* 40 will message length       */
    unsigned char   will_qos;       /* 44 will qos value (0,1,2)    */
    unsigned char   will_retain;    /* 45 will RETAIN               */
    unsigned char   clean_session;  /* 46 clean session             */
    unsigned char   unused1;        /* 47 unused                    */
    unsigned        packetid;       /* 48 packet id (publish msgid) */
    FILE            *pkttrc;        /* 4C packet trace file         */

    CPB             *rpb;           /* 50 current receive packet    */
    CPB             *spb;           /* 54 current send packet       */
    CPB             **spbq;         /* 58 send packet queue         */
    void            *ctx;           /* 5C callback context pointer  */

    /* various callback functions */
    int (*f_rcpb)(MQTC *mqtc, CPB *pb);    
                                    /* 60 received client packet    */
    int (*f_scpb)(MQTC *mqtc, CPB *pb);    
                                    /* 64 sent client packet        */
    int (*f_rtopic)(MQTC *mqtc, const char *topic, const char *msg, unsigned len); 
                                    /* 68 received topic and message*/
    int (*f_rtopic_bin)(MQTC *mqtc, const char *topic, const void *msg, unsigned len); 
                                    /* 6C received topic and message*/

    int (*f_reconnect)(MQTC *mqtc); /* 70 post reconnect callback   */
    unsigned        unused2[3];     /* 74 unused                    */

    time64_t        readtime;       /* 80 last socket read time     */
    time64_t        keeptime;       /* 88 next keep alive ping      */

    time64_t        retry;          /* 90 retry packet send         */
    unsigned short  keepalive;      /* 98 keep alive every x seconds*/
    unsigned short  unused3;        /* 9A unused                    */
    RPB             **rpbq;         /* 9C Retry packet buffer queue */
};                                  /* A0 (160 bytes)               */

/* MQTT370 Client Packet Buffer */
struct cpacketbuf {
    unsigned        bufsize;        /* 00 size of packet buffer     */
    unsigned        pos;            /* 04 current position in buffer*/
    unsigned short  varpos;         /* 08 offset to variable header */
    unsigned short  paypos;         /* 0A offset to payload         */
    unsigned char   buf[4];         /* 0C start of packet buffer    */
};                                  /* 10 (16 bytes)                */

/* MQTT370 Client Packet Retry Buffer */
struct cretrybuf {
    time64_t        retry;          /* 00 Retry after this time     */
    unsigned char   count;          /* 08 retry count               */
    unsigned char   unused;         /* 09 unused                    */
    unsigned short  msgid;          /* 0A message id                */
    CPB             *cpb;           /* 0C Client Packet Buffer      */
};                                  /* 10 (16 bytes)                */

MQTC *mqtc_new_client(void)                                             asm("MQTCNCLI");
MQTC *mqtc_init_client(MQTC *mqtc)                                      asm("MQTCICLI");
int mqtc_open_client(MQTC *mqtc, const char *broker, const char *port)  asm("MQTCOCLI");
int mqtc_close_client(MQTC *mqtc)                                       asm("MQTCCCLI");
void mqtc_reset_client(MQTC *mqtc)                                      asm("MQTCRCLI");
void mqtc_free_client(MQTC **pmqtc)                                     asm("MQTCFCLI");

int mqtc_set_clientid(MQTC *mqtc, const char *clientid)                 asm("MQTCSSID");
int mqtc_set_userid(MQTC *mqtc, const char *userid)                     asm("MQTCSUSE");
int mqtc_set_password(MQTC *mqtc, const char *password)                 asm("MQTCSPAS");
int mqtc_set_will_topic(MQTC *mqtc, const char *will_topic)             asm("MQTCSWTO");
int mqtc_set_will_msg(MQTC *mqtc, const char *will_message)             asm("MQTCSWMS");
int mqtc_set_will_msg_bin(MQTC *mqtc, const void *will_message, unsigned will_len) 
                                                                        asm("MQTCSWMB");
void mqtc_set_context(MQTC *mqtc, void *ctx)                            asm("MQTCSCTX");
void *mqtc_get_context(MQTC *mqtc)                                      asm("MQTCGCTX");

/* after a connection to the MQTT broker is established via mqtc_open_client()
 * you can call mqtc_create_thread() to create a MQTT processing thread
 * for this MQTC handle, or you can call mqtc_send_recv() as part of your
 * processing loop to send and recv MQTT packets.
 */
int mqtc_create_thread(MQTC *mqtc)                                      asm("MQTCCTHD");
int mqtc_send_recv(MQTC *mqtc)                                          asm("MQTCSNRC");

/* mqtc_connect() - send CONNECT packet to broker (establish a session) */
int mqtc_connect(MQTC *mqtc)                                            asm("MQTCCONN");

/* mqtc_sub() - subscribe to topic */
int mqtc_sub(MQTC *mqtc, unsigned qos, const char *topic)               asm("MQTCSUBS");

/* mqtc_subf() - subscribe to formatted topic */
int mqtc_subf(MQTC *mqtc, unsigned qos, const char *fmt, ...)           asm("MQTCSUBF");

/* mqtc_pub() - publish topic message string */
int mqtc_pub(MQTC *mqtc, unsigned qos, unsigned retain, const char *topic, const char *msg)
                                                                        asm("MQTCPUBL");
/* mqtc_pubf() - publish topic formatted message string */
int mqtc_pubf(MQTC *mqtc, unsigned qos, unsigned retain, const char *topic, const char *fmt, ...)        
                                                                        asm("MQTCPUBF");
/* mqtc_pub_bin() - publish topic message binary (asis) */
int mqtc_pub_bin(MQTC *mqtc, unsigned qos, unsigned retain, const char *topic, const void *msg, unsigned msglen)
                                                                        asm("MQTCPUBB");
/* mqtc_unsub() - unsubscribe from topic */
int mqtc_unsub(MQTC *mqtc, const char *topic)                           asm("MQTCUNSS");

/* mqtc_unsubf() - unsubscribe from formatted topic */
int mqtc_unsubf(MQTC *mqtc, const char *fmt, ...)                       asm("MQTCUNSF");



const char *mqtc_packet_type(CPB *pb)                                   asm("MQTCPTYP");

int mqtc_processing_thread(void *arg1, void *arg2)                      asm("MQTCPTHD");

int mqtc_session_process(MQTC *mqtc)                                    asm("MQTCSPRO");

int mqtc_session_send(MQTC *mqtc, CPB *pb)                              asm("MQTCSEND");

int mqtc_packet_dump(MQTC *mqtc, CPB *pb, const char *title)            asm("MQTCPDMP");

int mqtc_process_connack(MQTC *mqtc, CPB *pb)                           asm("MQTCPRO2");
int mqtc_process_publish(MQTC *mqtc, CPB *pb)                           asm("MQTCPRO3");
int mqtc_process_puback(MQTC *mqtc, CPB *pb)                            asm("MQTCPRO4");
int mqtc_process_pubrec(MQTC *mqtc, CPB *pb)                            asm("MQTCPRO5");
int mqtc_process_pubrel(MQTC *mqtc, CPB *pb)                            asm("MQTCPRO6");
int mqtc_process_pubcomp(MQTC *mqtc, CPB *pb)                           asm("MQTCPRO7");
int mqtc_process_suback(MQTC *mqtc, CPB *pb)                            asm("MQTCPRO9");
int mqtc_process_unsuback(MQTC *mqtc, CPB *pb)                          asm("MQTCPROB");
int mqtc_process_pingresp(MQTC *mqtc, CPB *pb)                          asm("MQTCPROD");


int mqtc_reply_puback(MQTC *mqtc, unsigned msgid)                       asm("MQTCREP4");
int mqtc_reply_pubrec(MQTC *mqtc, unsigned msgid)                       asm("MQTCREP5");

int mqtc_packet_retry(MQTC *mqtc, unsigned short msgid, CPB *pb)        asm("MQTCPACR");
int mqtc_packet_discard(MQTC *mqtc, unsigned short msgid)               asm("MQTCPACD");

int mqtc_packet_trace(MQTC *mqtc, CPB *pb, int b_sent)                  asm("MQTCPACT");

int mqtc_send_ping(MQTC *mqtc)                                          asm("MQTCPING");

#endif
