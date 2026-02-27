#ifndef MQTTUTIL_H
#define MQTTUTIL_H

/* crent370 headers */
#include <stddef.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <time64.h>					/* 64 bit time prototypes		*/
#include <errno.h>
#include "clibppa.h"                /* C runtime program properties */
#include "clibcrt.h"                /* C runtime area for each task */
#include "clibenv.h"                /* C runtime environment vars   */
#include "clibstae.h"               /* C runtime recovery routines  */
#include "clibwto.h"                /* write to operator            */
#include "clibcib.h"                /* console information block    */
#include "clibthrd.h"               /* basic threads                */
#include "clibthdi.h"               /* thread management            */
#include "cliblink.h"               /* link to external program     */
#include "clibary.h"                /* dynamic arrays               */
#include "sha256.h"					/* SHA 256 function				*/
#include "clibb64.h"				/* base64 encode/decode			*/
#include "clibssib.h"				/* SSIB, __ssib(), __jobid()	*/
#include "clibtiot.h"				/* TIOT, __tiot(), __jobname()	*/
#include "clibsmf.h"                /* __smfid()                    */
#include "clibsock.h"               /* CLIBSOCK                     */
#include "cde.h"
/* our headers */
#include "socket.h"                 /* sockets via DYN75            */
// #include "errors.h"                 /* the missing errno values     */
#include "racf.h"                   /* security environment         */
// #include "cred.h"					/* Credentials					*/

#define MAX_TOPIC   255         /* max topic/subscription length    */
#define MAX_MESSAGE (64*1024)   /* max message size for our topics  */

/* Connection Ack Reasons */
#define CONNACK_ACCEPTED            0
#define CONNACK_REFUSED_PROTOCOL    1
#define CONNACK_REFUSED_IDENTIFER   2
#define CONNACK_REFUSED_SERVER      3
#define CONNACK_REFUSED_USER_PASS   4
#define CONNACK_REFUSED_NOT_AUTH    5

/* Packet Types */
#define TYPE_CONNECT        0x10    /* ... connect        0001 .... */
#define TYPE_CONNACK        0x20    /* ... connect ack    0010 .... */
#define TYPE_PUBLISH        0x30    /* ... publish        0011 .... */
#define TYPE_PUBACK         0x40    /* ... publish ack    0100 .... */
#define TYPE_PUBREC         0x50    /* ... pubrec         0101 .... */
#define TYPE_PUBREL         0x60    /* ... pubrel         0110 .... */
#define TYPE_PUBCOMP        0x70    /* ... pubcomp        0111 .... */
#define TYPE_SUBSCRIBE      0x80    /* ... subscribe      1000 .... */
#define TYPE_SUBACK         0x90    /* ... subscribe ack  1001 .... */
#define TYPE_UNSUBSCRIBE    0xA0    /* ... unsubscribe    1010 .... */
#define TYPE_UNSUBACK       0xB0    /* ... unsub ack      1011 .... */
#define TYPE_PINGREQ        0xC0    /* ... ping request   1100 .... */
#define TYPE_PINGRESP       0xD0    /* ... ping response  1101 .... */
#define TYPE_DISCONNECT     0xE0    /* ... disconnect     1110 .... */

/* Connect Flags */
#define FLAG_USERID         0x80    /* 1... .... */
#define FLAG_PASSWORD       0x40    /* .1.. .... */
#define FLAG_WILL_RETAIN    0x20    /* ..1. .... */
#define FLAG_WILL_QOS       0x18    /* ...1 1... */
#define FLAG_WILL           0x04    /* .... .1.. */
#define FLAG_CLEAN          0x02    /* .... ..1. */

/* Fixed Header Flags */
#define FIXED_FLAG_DUP      0x08    /* .... 1... */
#define FIXED_FLAG_QOS      0x06    /* .... .11. */
#define FIXED_FLAG_RETAIN   0x01    /* .... ...1 */


extern unsigned char *utila2e;      /* ascii to ebcdic table        */
extern unsigned char *utile2a;      /* ebcdic to ascii table        */ 

/* util_prng() - returns next pseudo random number */
uint32_t util_prng(void)                                                asm("UTILPRNG");

/* util_free_char() - free char * at location of pstr (safe free of char pointer) */
void util_free_char(char **pstr)                                        asm("UTILFCHR");

/* util_dup_a2e() - duplicate and convert ascii to ebcdic string */
char *util_dup_a2e(const unsigned char *uc, unsigned len)               asm("UTILDA2E");

/* util_dup_e2a() - duplicate and convert ebcdic to ascii string */
char *util_dup_e2a(const unsigned char *uc, unsigned len)               asm("UTILDE2A");

/* util_get_ushort() - get unsigned short value from buffer (2 characters) */
unsigned short util_get_ushort(const unsigned char *uc)                 asm("UTILGSHT");

/* util_put_ushort() - put unsigned shrt value into buffer (2 characters) */
void util_put_ushort(unsigned value, unsigned char *buf)                asm("UTILPSHT");

/* util_memdup() - duplicate memory */
char *util_memdup(unsigned char *uc, unsigned len)                      asm("UTILMDUP");

/* util_str_upper() - fold string to UPPER case */
char *util_str_upper(char *str)                                         asm("UTILSTRU");

/* util_stack_hwm() - returns stack high water mark */
unsigned util_stack_hwm(void *stk, unsigned bytes)                      asm("UTILSHWM");

/* util_task_hwm() - returns task stack high water mark */
unsigned util_task_hwm(const CTHDTASK *task, unsigned *stk, unsigned *bytes) asm("UTILTHWM");

/* util_open_ns_socket() - open non-blocking socket */
int util_open_nb_socket(const char* host, const char* port)             asm("UTILONBS");

/* util_length_encode() - returns number of encoded bytes in buf */
int util_length_encode(unsigned length, unsigned char *buf)             asm("UTILLENE");

/* mqtb_length_decode() - returns number of decoded bytes from buf */
int util_length_decode(const unsigned char *buf, unsigned *length)      asm("UTILLEND");

/* util_topic_is_valid() - return 0 if invalid, length if valid */
int util_topic_is_valid(const char *topic)                              asm("UTILTOPV");

/* util_socket_name() - get socket name ip and port */
int util_socket_name(int sock, char *ip, char *port)                    asm("UTILSOCN");

/* util_socket_peer() - get socket peer ip and port */
int util_socket_peer(int sock, char *ip, char *port)                    asm("UTILSOCP");

/* util_ntoa() - format addr as IPV4 address string */
int util_ntoa(unsigned addr, char *buf)                                 asm("UTILNTOA");

#endif /* MQTTUTIL_H */
