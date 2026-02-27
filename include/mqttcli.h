#ifndef MQTTCLI_H
#define MQTTCLI_H

#include "mqtc370.h"                /* MQTT Client                  */
#include "clibstae.h"               /* C runtime recovery routines  */
#include "clib.h"
#include <clibstr.h>                /* __patmat()                   */
#include <clibispf.h>		        /* ISPF prototypes				*/
#include <cliblink.h>		        /* __link()						*/
#include <clibtso.h>		        /* tsocmd()						*/
#include <ikjcppl.h>		        /* CPPL typedef 				*/
#include <ikject.h>			        /* ECT typedef					*/

typedef struct mqttcli  MQTTCLI;    /* MQTT Command Line Interface  */

/* MQTT Command Line Interface */
struct mqttcli {
    char            eye[8];         /* 00 eye catcher for dumps     */
#define MQTTCLI_EYE "MQTTCLI"       /* ... eye catcher for dumps    */
    MQTC            *mqtc;          /* 08 MQTT Client Handle        */
    const char      *pgmname;       /* 0C program name              */
    char            *host;          /* 10 host name                 */
    char            *port;          /* 14 port number               */
    char            *user;          /* 18 user name                 */
    char            *pass;          /* 1C password stirng           */
    char            *topic;         /* 20 topic name                */
    char            *msg;           /* 24 message string            */
    char            *verb;          /* 28 verb/command name         */
    unsigned char   flags;          /* 2C processing flags          */
#define MQTTCLI_FLAG_EXIT   0x80    /* ... EXIT/END/QUIT            */
#define MQTTCLI_FLAG_CONN   0x40    /* ... Connected to broker      */
#define MQTTCLI_FLAG_POLL   0x20    /* ... Use send/recv polling    */
    unsigned char   qos;            /* 2D qos level (0,1,2)         */
    unsigned char   retain;         /* 2E retain (bool)             */
    char            unusedc;        /* 2F unused                    */
    unsigned        ecb;            /* 30 event control block       */
    unsigned        unusedu[3];     /* 34 unused                    */
};                                  /* 40 (64 bytes)                */


MQTTCLI *mqttcli_new(void)                                              asm("MQTTCLIN");
void mqttcli_free(MQTTCLI *mqttcli)                                     asm("MQTTCLIF");

/* __start() - open MQTTOUT, MQTTERR, MQTTIN - parse command line - call mqttmain() */
int __start(char *p, char *pgmname, int tsojbid, void **pgmr1); /* in MQTTCLI.C */

/* mqttmain() - process command line args */
int mqttmain(int argc, char **argv)                                     asm("MQTTMAIN");

/* utility functions */
char *strupper(char *str);

/* mqtt_parse() - parse argc, argv into mqttcli handle */
int mqtt_parse(MQTTCLI *mqttcli, int argc, char **argv)                 asm("MQTTPARS");

/* mqtt_dump() - dump values from mqttcli handle */
int mqtt_dump(MQTTCLI *mqttcli)                                         asm("MQTTDUMP");

/* mqtt_args_new() - create argv array from string buffer */
char **mqtt_args_new(char *str)                                         asm("MQTTANEW");

void mqtt_args_free(char ***pargv)                                      asm("MQTTAFRE");

int mqtt_exec(MQTTCLI *mqttcli)                                         asm("MQTTEXEC");

int mqtt_loop(MQTTCLI *mqttcli)                                         asm("MQTTLOOP");

int mqtt_bye(MQTTCLI *mqttcli)                                          asm("MQTTBYE");

int mqtt_connect(MQTTCLI *mqttcli)                                      asm("MQTTCONN");

int mqtt_pub(MQTTCLI *mqttcli)                                          asm("MQTTPUB");

#endif
