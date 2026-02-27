#include "mqtb370.h"

unsigned __stklen = 64*1024;

static int auth_setup(MQTB *mqtb, const char *name);
static int unauth_setup(MQTB *mqtb, const char *name);
static unsigned build_ecblist(MQTB *mqtb, COM *com, unsigned **ecblist);

/* utility functions */
static int auth_name(const char *name);
static int identify_cthread(void);
static CDE *find_cde(const char *name);
static int auth_name(const char *name);

int main(int argc, char **argv)
{
    CLIBPPA     *ppa    = __ppaget();   /* A(CLIBPPA)               */
    CLIBCRT     *crt    = __crtget();   /* A(CLIBCRT)               */
    CLIBGRT     *grt    = __grtget();   /* A(CLIBGRT)               */
    CTHDTASK    *task;
    COM         *com;
    CIB         *cib;
    int         i;
    int         rc;
    unsigned    count;
    unsigned    *ecblist[10];
    char        buf[1024];
    MQTB        server  = {0};
    MQTB        *mqtb   = &server;

    grt->grtapp1    = mqtb;
    strcpy(mqtb->eye, MQTB_EYE);
    mqtb->listen    = -1;
    mqtb->proto     = MQTB_PROTO;

    if (crt->crtopts & CRTOPTS_AUTH) {
        /* this task was previously authorized */
        rc = auth_setup(mqtb, argv[0]);
    }
    else {
        /* this task has not been authorized yet */
        rc = unauth_setup(mqtb, argv[0]);
    }
    if (rc) goto quit;

    // wtof("%s: stack=%p size=%u hwm=%u", __func__, crt->crtsave, __stklen, util_stack_hwm(crt->crtsave, __stklen));

    com = __gtcom();
    if (!com) {
        wtof("MQTTD090E Unable to initialize console interface\n");
        goto quit;
    }

    /* set number of modify commands we'll queue */
    __cibset(5);

    /* get configuration settings */
    rc = mqtb_config(mqtb, argc, argv);
    if (rc) goto quit;

    /* Initialize using configuration settings */
    rc = mqtb_initialize(mqtb);
    if (rc) goto quit;

    task = cthread_self();
    if (task) {
        task->func = main;
        task->arg1 = (void*)argc;
        task->arg2 = argv;
        mqtb->main = task;
    }
    
    /* We should be ready to go */
    mqtb->flags |= MQTB_FLAG_READY;
    wtof("MQTTD001I MQTT Broker is READY on port %u", mqtb->mqtbc->port);

    /* wait for a console command and process as needed */
    for(;;) {
        /* check for console command */
        if (*(com->comecbpt) & 0x40000000) {
            cib = __cibget();
            if (cib) {
                /* process console command */
                i = mqtb_console(mqtb, cib);
                __cibdel(cib);
                if (i) break;	/* must be a stop "/P jobname" */
            }
        }

        memset(ecblist, 0, sizeof(ecblist));
        count = build_ecblist(mqtb, com, ecblist);
        if (count) {
            __asm__("WAIT ECBLIST=(%0)" : : "r"(ecblist));
        }
        else {
            /* we need to wait 1 second */
            __asm__("STIMER WAIT,BINTVL==F'100'   1 second");
        }
    }

quit:
    /* terminate the server */
    mqtb_terminate(mqtb);

    wtof("MQTTD002I MQTT Broker is SHUTDOWN");

    if (crt) {
        if (crt->crtauth & CRTAUTH_STEPLIB) {
            __uastep();    /* reset STEPLIB APF authorization  */
        }
        if (crt->crtauth & CRTAUTH_ON) {
            __uatask();    /* reset APF authorization  */
        }
    }
    
    return rc;
}

static int auth_setup(MQTB *mqtb, const char *name)
{
    int     rc = 0;

    wtof("MQTTD010I %s is APF authorized", name);

    return rc;
}

static int unauth_setup(MQTB *mqtb, const char *name)
{
    int         rc      = 0;
    CLIBCRT     *crt    = __crtget();   /* A(CLIBCRT)               */

    /* this task is not currently APF authorized */
    rc = __autask();    /* APF authorize this task  */
    /* wtof("%s __autask() rc=%d", __func__, rc); */
    if (rc==0) {
        if (crt->crtauth & CRTAUTH_ON) {
            wtof("MQTTD011I %s was APF authorized via SVC 244", name);
            /* we want the STEPLIB APF authorized as well */
            rc = __austep();    /* APF authorize the STEPLIB */
            if (rc==0) {
                if (crt->crtauth & CRTAUTH_STEPLIB) {
                    wtof("MQTTD013I STEPLIB is now APF authorized");
                    rc = auth_name(name);
                    /* wtof("%s auth_name(%s) rc=%d", __func__, name, rc); */
                    rc = identify_cthread();
                    /* wtof("%s identify_cthread() rc=%d", __func__, rc); */
                }
            }
        }
        else {
            wtof("MQTTD011I %s is APF authorized", name);
            rc = auth_name(name);
            /* wtof("%s auth_name(%s) rc=%d", __func__, name, rc); */
            rc = identify_cthread();
            /* wtof("%s identify_cthread() rc=%d", __func__, rc); */
        }
    }
    else {
        wtof("MQTTD012E %s unable to dynamically obtain APF "
            "authorization", name);
    }

    return rc;
}

static CDE *
find_cde(const char *name)
{
    unsigned    *psa    = 0;                        /* low core == PSA      */
    unsigned    *tcb    = (unsigned*)psa[0x21c/4];  /* TCB      == PSATOLD  */
    unsigned    *jstcb  = (unsigned*)tcb[0x7c/4];   /* JSTCB    == TCBJSTCB */
    CDE         *cde    = (CDE*)jstcb[0x2c/4];      /* CDE      == TCBJPQ   */
    int         i;
    char        temp[9] = "        ";

    for(i=0; i < 8 && name[i]; i++) {
        temp[i] = name[i];
    }

    while(cde) {
        if (memcmp(cde->CDNAME, temp, 8)==0) {
            return cde;
            break;
        }
        cde = cde->CDCHAIN;
    }

    return NULL;
}

static int
auth_cde(CDE *cde)
{
    int     rc = 0;

    if (cde) {
        __asm__("MODESET KEY=ZERO,MODE=SUP\n" : : : "0", "1", "14", "15");
        cde->CDATTR2 |= (CDSYSLIB | CDAUTH);
        __asm__("MODESET KEY=NZERO,MODE=PROB" : : : "0", "1", "14", "15");
    }

    return rc;
}


static int auth_name(const char *name)
{
    int     rc = 0;
    CDE     *cde = find_cde(name);

    if (cde) {
        /* wtodumpf(cde, sizeof(CDE), "%s %s CDE before auth_cde()", __func__, name); */
        rc = auth_cde(cde);
        /* wtodumpf(cde, sizeof(CDE), "%s %s CDE after auth_cde()", __func__, name); */
    }
    else {
        /* wtof("%s %s CDE was not found", __func__, name); */
        rc = 4;
    }

    return rc;
}

static int identify_cthread(void)
{
    int     rc = 0;

    /* CTHREAD does not already exist, identify it now */
    /* wtof("%s preparing to IDENTIFY CTHREAD", __func__); */

    __asm__("L     1,=V(CTHREAD)    A(thread driver routine)\n\t"
            "LA    0,=CL8'CTHREAD'\n\t"
            "IDENTIFY EPLOC=(0),ENTRY=(1)\n\t"
            "ST    15,0(,%0)\n" : : "r" (&rc): "0", "1", "14", "15");
    __asm__("\n*\n");

    /* wtof("%s IDENTIFY CTHREAD rc=%d", __func__, rc); */

    rc = auth_name("CTHREAD");

    return rc;
}

static unsigned
build_ecblist(MQTB *mqtb, COM *com, unsigned **ecblist)
{
    unsigned        pos     = 0;
    unsigned        *ecbp;

    if (com->comecbpt) {
        ecblist[pos++] = com->comecbpt;
    }

quit:
    if (pos) {
        ecbp = ecblist[pos-1];
        ecblist[pos-1] = (unsigned*)((unsigned)ecbp | 0x80000000);
    }

    return pos;
}
