#ifndef MQTB370_H
#define MQTB370_H
/*
    Message Queuing Telemetry Broker for legacy MVS 38J

    Based on the MQTT version 3.1 protocol specification.
    https://public.dhe.ibm.com/software/dw/webservices/ws-mqtt/MQTT_V3.1_Protocol_Specific.pdf

    This is FREE software. No warranty is implied or expressed.
    Use at your own risk.
*/

#include "mqttutil.h"

/* crent370 headers */
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


typedef struct mqtb             MQTB;   /* MQTT broker              */
typedef struct mqtb_config      MQTBC;  /* MQTB Config              */
typedef struct mqtb_session     MQTBS;  /* session                  */
typedef struct mqtb_publish     MQPUB;  /* publish request          */
typedef struct mqtb_subscriber  MQSUB;  /* subscribers              */
typedef struct mqtb_expect      MQEXP;  /* expected responses       */

typedef struct packetbuf        PB;     /* packet buffer            */

struct mqtb {
    char            eye[6];         /* 00 eye catcher               */
#define MQTB_EYE    "MQTB"          /* ... eye catcher              */
    volatile unsigned char flags;   /* 06 flags                     */
#define MQTB_FLAG_INIT      0x01    /* ... initializing             */
#define MQTB_FLAG_LISTENER  0x02    /* ... listener thread created  */
#define MQTB_FLAG_READY     0x04    /* ... ready                    */
#define MQTB_FLAG_X08       0x08    /* ... unused                   */
#define MQTB_FLAG_LOGGING   0x10    /* ... logging enabled          */
#define MQTB_FLAG_LOGWTO    0x20    /* ... log via WTO()            */
#define MQTB_FLAG_QUIESCE   0x40    /* ... don't accept new request */
#define MQTB_FLAG_SHUTDOWN  0x80    /* ... shutdown now             */

    unsigned char   listen_que;     /* 07 listen queue depth        */
    char            *proto;         /* 08 broker version string     */
#define MQTB_PROTO  "3.1.1"         /* ... broker version           */
    MQTBS           **mqtbs;        /* 0C session array (clients)   */

    MQPUB           **topics;       /* 10 array of published topics */
    MQSUB           **subscribers;  /* 14 array of topic subscriber */
    CTHDMGR         *session_mgr;   /* 18 session manager thread    */
    CTHDTASK        *main;          /* 1C main thread               */

    __64            listen_disp;    /* 20 listen dispatched         */
    __64            wait_disp;      /* 28 wait thread dispatched    */
    __64            monitor_disp;   /* 30 monitor dispatched        */
    __64            publish_disp;   /* 38 publish dispatched        */

    unsigned        listen_ecb;     /* 40 listener ecb              */
    int             listen;         /* 44 our listener socket       */
    unsigned        unused1;        /* 48 unused                    */
    CTHDTASK        *listen_thread; /* 4C listener thread           */
    
    unsigned        wait_ecb;       /* 50 wait queue thread ecb     */
    MQTBS           **wait_queue;   /* 54 sessions in WAIT state    */
    unsigned        unused2;        /* 58 unused                    */
    CTHDTASK        *wait_thread;   /* 5C session wait thread       */

    unsigned        publish_ecb;    /* 60 publish ecb               */
    MQPUB           **publish_queue;/* 64 publish queue             */
    unsigned        unused3;        /* 68 unused                    */
    CTHDTASK        *publish_thread;/* 6C publish thread            */

    unsigned        monitor_ecb;    /* 70 monitor thread ecb        */
    unsigned        unused4;        /* 74 unused                    */
    unsigned        unused5;        /* 78 unused                    */
    CTHDTASK        *monitor_thread;/* 7C monitor thread handle     */

    lua_State       *config;		/* 80 Lua configuration state	*/
    MQTBC           *mqtbc;         /* 84 MQTB configuration        */
};                                  /* 88 (136 bytes)               */

struct mqtb_config {
    char            eye[6];         /* 00 eye catcher               */
#define MQTBC_EYE   "MQTBC"         /* ... eye catcher              */
    unsigned short  port;           /* 06 port number               */
    char            *tz;            /* 08 time zone string          */
    unsigned        topic_expire;   /* 0C topic expire seconds      */
    char            **auth_list;    /* 10 authorized users list     */
    unsigned char   mintask;        /* 14 minimum task number       */
    unsigned char   maxtask;        /* 15 maimum task number        */
    unsigned char   listenq;        /* 16 listen queue depth        */
    unsigned char   bind_tries;     /* 17 bind tries                */
    unsigned char   bind_sleep;     /* 18 bind sleep interval       */
    unsigned char   auth;           /* 1A session authorization     */
#define MQTBC_AUTH_ANON     0x80    /* ... anonymous login allowed  */
#define MQTBC_AUTH_LOGIN    0x40    /* ... login userid & password  */
#define MQTBC_AUTH_LIST     0x20    /* ... userid in auth_list      */
    unsigned char   datetime;       /* 1B datetime flag             */
#define MQTBC_DATETIME_GMT  0x80    /* ... datetime in GMT format   */
    char            *retain_dataset;/* 1C retain dataset name       */
};                                  /* 20 (32 bytes)                */

MQTBC *mqtbc_new(MQTB *mqtb)                                            asm("MQTBCNEW");
void mqtbc_free(MQTBC **pmqtbc)                                         asm("MQTBCFRE");

struct packetbuf {
    unsigned        bufsize;        /* 00 size of packet buffer     */
    unsigned        pos;            /* 04 current position in buffer*/
    unsigned short  varpos;         /* 08 offset to variable header */
    unsigned short  paypos;         /* 0A offset to payload         */
    unsigned char   buf[4];         /* 0C start of packet buffer    */
};                                  /* 10 (16 bytes)                */

/* MQTT Client Session */
struct mqtb_session {
    char            eye[6];         /* 00 eye catcher               */
#define MQTBS_EYE   "MQTBS"         /* ... eye catcher              */
    unsigned char   proto;          /* 06 protocol number           */
#define MQTBS_PROTO_311     4       /* ... MQTT 3.1.1 protocol      */
#define MQTBS_PROTO_500     5       /* ... MQTT 5.0.0 protocol      */

    unsigned char   will_retain;    /* 07 Will retain flag (0,1)    */
    unsigned char   will_qos;       /* 08 Will QOS level (0,1,2)    */
    unsigned char   will_send;      /* 09 send will disconnect (0,1)*/
    unsigned char   clean_session;  /* 0A clean session/start (0,1) */
    unsigned char   flags;          /* 0B flags                     */
#define MQTBS_FLAG_ERROR        0x80/* ... session error            */
#define MQTBS_FLAG_SESSION      0x40/* ... valid session established*/
#define MQTBS_FLAG_WAITRESP     0x20/* ... don't send spbq packets  */
    
    char            *user;          /* 0C user name                 */
    char            *password;      /* 10 password                  */
    char            *will_topic;    /* 14 will topic                */
    char            *will_msg;      /* 18 will message              */
    unsigned        will_msg_len;   /* 1C will message length       */

    unsigned        keepalive;      /* 20 keep alive timer          */

    volatile unsigned state;        /* 24 state                     */
#define MQTBS_STATE_INIT        0   /* ... initializing             */
#define MQTBS_STATE_WAIT        1   /* ... waiting for work/timeout */
#define MQTBS_STATE_READ        2   /* ... reading freom socket     */
#define MQTBS_STATE_PROC        3   /* ... processing packet        */
#define MQTBS_STATE_SEND        4   /* ... sending to socket        */
#define MQTBS_STATE_CLOSE       5   /* ... session close is needed  */
#define MQTBS_STATE_SHUTDOWN    6   /* ... shutdown has occured     */

    time64_t        read_time;      /* 28 time of last packet read  */

    MQTB            *mqtb;          /* 30 broker handle             */
    int             sock;           /* 34 session socket            */
    PB              *rpb;           /* 38 recv packet buffer        */
    PB              *spb;           /* 3C send packet buffer        */

    char            *clientid;      /* 40 client id                 */
    unsigned        clientid_len;   /* 44 client id length          */
    __64            dispatched;     /* 48 dispatched count          */
    PB              **spbq;         /* 50 send packet buffer queue  */
    char            *protocal_name; /* 54 protocol name             */
    
    time64_t        retry;          /* 58 retry after this time     */
};                                  /* 60 (96 bytes)                */

/* Expected Responses */
struct mqtb_expect {
    char            eye[6];         /* 00 eye catcher               */
#define MQEXP_EYE   "MQEXP"         /* ... eye cacther              */
    unsigned short  msgid;          /* 06 message id (packet id)    */
    time64_t        retry;          /* 08 retry after this time     */
    char            topic[0];       /* 10 topic name                */
};                                  /* 10 (16 bytes)                */

/* Publish Request */
struct mqtb_publish {
    char            eye[6];         /* 00 eye catcher               */
#define MQPUB_EYE   "MQPUB"         /* ... eye catcher              */
    unsigned char   retain;         /* 06 retain flag               */
    unsigned char   qos;            /* 07 qos (0,1,2)               */
    time64_t        expires;        /* 08 expires after this time   */

    unsigned short  msgid;          /* 10 message id (0000-ffff)    */
    unsigned char   dup;            /* 12 dup flag                  */
    unsigned char   unused;         /* 13 unused                    */
    char            *topic;         /* 14 topic name                */
    unsigned char   *msg;           /* 18 message                   */
    unsigned        msg_len;        /* 1C message length            */
};                                  /* 20 (32 bytes)                */

/* Topic Subscriber */
struct mqtb_subscriber {
    char            eye[6];         /* 00 eye catcher               */
#define MQSUB_EYE   "MQSUB"         /* ... eye catcher              */
    unsigned char   unused1;        /* 06 unused                    */
    unsigned char   qos;            /* 07 QOS value (0,1,2)         */
    char            *topic;         /* 08 topic name                */
    MQTBS           **mqtbs;        /* 0C array of sessions         */
};

int mqtb_unsubscribe_session(MQTB *mqtb, MQTBS *mqtbs)                  asm("MQTBUNSS");

int mqtb_pub(MQTB *mqtb, const char *topic, void *msg, unsigned len)    asm("MQTBPB");
int mqtb_pubf(MQTB *mqtb, const char *topic, const char *fmt, ...)      asm("MQTBPBF");
int mqtb_pubv(MQTB *mqtb, const char *topic, const char *fmt, va_list args) asm("MQTBPBV");

int mqtb_publish_request(MQTB *mqtb, MQPUB *mqpub)                      asm("MQTBPUBR");
int mqtb_publish_update(MQTB *mqtb, MQPUB *mqpub)                       asm("MQTBPUBU");
void mqtb_publish_free(MQTB *mqtb, MQPUB **pmqpub)                      asm("MQTBPUBF");
int mqtb_publish_subscriber(MQTB *mqtb, MQPUB *mqpub, unsigned char subqos, MQTBS *mqtbs) asm("MQTBPUBS");


int mqtb_session_getc(MQTBS *mqtbs)                                     asm("MQTBSGET");
int mqtb_session_close(MQTB *mqtb, MQTBS **pmqtbs)                      asm("MQTBSCLS");
int mqtb_session_process(MQTB *mqtb, MQTBS *mqtbs)                      asm("MQTBSPRO");
int mqtb_session_send(MQTB *mqtb, MQTBS *mqtbs, PB *pb)                 asm("MQTBSEND");


void mqtb_logf(MQTB *mqtb, const char *fmt, ...)                        asm("MQTBLOGF");
void mqtb_logv(MQTB *mqtb, const char *fmt, va_list args)               asm("MQTBLOGV");


/* Broker config, initialization, termination, console processor */
int mqtb_config(MQTB *mqtb, int argc, char **argv)                      asm("MQTBCONF");
int mqtb_initialize(MQTB *mqtb)                                         asm("MQTBINIT");
int mqtb_terminate(MQTB *mqtb)                                          asm("MQTBTERM");
int mqtb_console(MQTB *mqtb, CIB *cib)                                  asm("MQTBCONS");

/* Threads */
int mqtb_listen_thread(void *arg1, void *arg2)                          asm("MQTBTHDL");
int mqtb_monitor_thread(void *arg1, void *arg2)                         asm("MQTBTHDM");
int mqtb_wait_thread(void *arg1, void *arg2)                            asm("MQTBTHDW");
int mqtb_publish_thread(void *arg1, void *arg2)                         asm("MQTBTHDP");
int mqtb_session_thread(void *udata, CTHDWORK *work)                    asm("MQTBTHDS");


/* Packet Processors */
int mqtb_process_connect(MQTB *mqtb, MQTBS *mqtbs, PB *pb)              asm("MQTBPRO1");
int mqtb_process_publish(MQTB *mqtb, MQTBS *mqtbs, PB *pb)              asm("MQTBPRO3");
int mqtb_process_puback(MQTB *mqtb, MQTBS *mqtbs, PB *pb)               asm("MQTBPRO4");
int mqtb_process_pubrec(MQTB *mqtb, MQTBS *mqtbs, PB *pb)               asm("MQTBPRO5");
int mqtb_process_pubrel(MQTB *mqtb, MQTBS *mqtbs, PB *pb)               asm("MQTBPRO6");
int mqtb_process_pubcomp(MQTB *mqtb, MQTBS *mqtbs, PB *pb)              asm("MQTBPRO7");
int mqtb_process_subscribe(MQTB *mqtb, MQTBS *mqtbs, PB *pb)            asm("MQTBPRO8");
int mqtb_process_suback(MQTB *mqtb, MQTBS *mqtbs, PB *pb)               asm("MQTBPRO9");
int mqtb_process_unsubscribe(MQTB *mqtb, MQTBS *mqtbs, PB *pb)          asm("MQTBPROA");
int mqtb_process_pingreq(MQTB *mqtb, MQTBS *mqtbs, PB *pb)              asm("MQTBPROC");
int mqtb_process_disconnect(MQTB *mqtb, MQTBS *mqtbs, PB *pb)           asm("MQTBPROE");

/* Reply Messages */
int mqtb_reply_connack(MQTB *mqtb, MQTBS *mqtbs, unsigned rsn)          asm("MQTBREP2");
int mqtb_reply_puback(MQTB *mqtb, MQTBS *mqtbs, unsigned msgid)         asm("MQTBREP4");
int mqtb_reply_pubrec(MQTB *mqtb, MQTBS *mqtbs, unsigned msgid)         asm("MQTBREP5");
int mqtb_reply_pubrel(MQTB *mqtb, MQTBS *mqtbs, unsigned msgid)         asm("MQTBREP6");
int mqtb_reply_suback(MQTB *mqtb, MQTBS *mqtbs, unsigned msgid, unsigned *aqos) asm("MQTBREP9");
int mqtb_reply_unsuback(MQTB *mqtb, MQTBS *mqtbs, unsigned msgid)       asm("MQTBREPB");
int mqtb_reply_pingresp(MQTB *mqtb, MQTBS *mqtbs)                       asm("MQTBREPD");

int mqtb_subscribe_topic(MQTB *mqtb, MQTBS *mqtbs, char *topic, char *sqos) asm("MQTBSUBT");
int mqtb_unsubscribe_topic(MQTB *mqtb, MQTBS *mqtbs, char *topic)       asm("MQTBUNST");



/* Utility */
const char *mqtb_packet_type(PB *pb)                                    asm("MQTBPTYP");
int mqtb_packet_dump(MQTB *mqtb, MQTBS *mqtbs, PB *pb, const char *title) asm("MQTBPBDM");

/* mqtb_topic_is_valid() - returns topic length if topic is valid, 0 if not */
int mqtb_topic_is_valid(const char *topic)                              asm("MQTBTOPV");

/* mqtb_topic_sub_valid() - returns subscribtion length if subscription is valid, 0 if not */
int mqtb_topic_sub_valid(const char *sub)                               asm("MQTBTOPS");

/* mqtb_topic_match() - return 1 for sub matching topic, 0 if not */
int mqtb_topic_match(const char *topic, const char *sub)                asm("MQTBTOPM");



/* make a copy of string and convert from ascii to ebcdic character */
char *dupa2e(unsigned char *uc, unsigned len)                           asm("DUPA2E");
/* make a copy of string and convert from ebcdic to ascii character */
char *dupe2a(unsigned char *uc, unsigned len)                           asm("DUPE2A");
char *memdup(unsigned char *uc, unsigned len)                           asm("MEMDUP");
unsigned short get_unsigned_short(const unsigned char *uc)              asm("GETUSHRT");
void put_unsigned_short(unsigned value, unsigned char *buf)             asm("PUTUSHRT");

/* __stkhwm() - returns high water mark for stack of bytes size */
unsigned __stkhwm(void *stk, unsigned bytes)                            asm("@@STKHWM");

/* taskhwm() - returns high water mark for task stack.
 *             stk and bytes are returned if not NULL.
 */
unsigned taskhwm(const CTHDTASK *task, unsigned *stk, unsigned *bytes)  asm("TASKHWM");

char *mqtb_fmt_datetime(MQTB *mqtb, time64_t *t64, char *timebuf)       asm("MQTBDT");

int mqtb_retain_save(MQTB *mqtb, const char *dataset)                   asm("MQTBRETS");
int mqtb_retain_load(MQTB *mqtb, const char *dataset)                   asm("MQTBRETL");

#endif  /* MQTB370_H */
