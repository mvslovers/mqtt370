#include "mqttcli.h"

#define MAXPARMS 50 /* maximum number of arguments we can handle */

/* @@CRT0 calls @@START, @@START calls MQTTMAIN */
int
__start(char *p, char *pgmname, int tsojbid, void **pgmr1)
{
	CLIBPPA		*ppa	= __ppaget();
    CLIBGRT     *grt    = __grtget();
    CPPL		*cppl	= NULL;		/* TSO CPPL */
    char 		stdoutdsn[12];
    char 		stderrdsn[12];
    char 		stdindsn[12];
    int			stdoutdyn	= 0;
    int			stderrdyn	= 0;
    int			stdindyn	= 0;
    int         x;
    int         argc;
    unsigned    u;
    char        *argv[MAXPARMS + 1];
    int         rc;
    int         parmLen = 0;
    int         progLen = 0;
    char        parmbuf[310];

	if (!ispf_available()) {
		/* if something goes all wrong, capture it! */
		abendrpt(ESTAE_CREATE, DUMP_DEFAULT);
	}

	/* If this is a TSO environment, get the CPPL address */
    parmLen = ((unsigned int)p[0] << 8) | (unsigned int)p[1];
    if ((parmLen > 0) && (p[2] == 0)) {
        progLen = (unsigned int)p[3];
        cppl = (CPPL *)pgmr1;
        ppa->ppacppl = cppl;	/* save TSO CPPL address */

        // wtodumpf(cppl, sizeof(CPPL), "%s: CPPL", __func__);
        // wtodumpf(cppl->cpplcbuf, cppl->cpplcbuf->cbuflen, "%s: CBUF", __func__);
    }
    
	/* default daatset names */
	strcpy(stdoutdsn, "*MQTTOUT");
	strcpy(stderrdsn, "*MQTTERR");
	strcpy(stdindsn, "*MQTTIN");

    stdout = fopen(stdoutdsn, "w");
    if (!stdout) {
		wtof("Unable to open %s DD for output", stdoutdsn);
		__exita(EXIT_FAILURE);
	}

    stderr = fopen(stderrdsn, "w");
    if (!stderr) {
		wtof("Unable to open %s DD for output", stderrdsn);
        fclose(stdout);
        __exita(EXIT_FAILURE);
    }

    stdin = fopen(stdindsn, "r");
#if 0 /* debugging */
    wtof("mqttcli.c:%s: fopen(\"%s\",\"%s\") fp=0x%08X", __func__, stdindsn, "r", stdin);
    if (stdin) {
		wtodumpf(stdin, sizeof(FILE), "mqttcli.c:%s: STDIN", __func__);
		wtodumpf(stdin->dcb, sizeof(DCB), "STDIN DCB");
	}
#endif
    if (!stdin) {
		stdin = fopen("'NULLFILE'", "r");
	}
    if (!stdin) {
		wtof("Unable to open %s DD for input", stdindsn);
        fclose(stdout);
        fclose(stderr);
        __exita(EXIT_FAILURE);
    }

    /* load any environment variables */
    if (loadenv("dd:MQTTENV")) {
        /* no LUAENV DD, try ENVIRON DD */
        loadenv("dd:ENVIRON");
    }

    /* initialize time zone offset for this thread */
    tzset();

    if (parmLen >= sizeof(parmbuf) - 2) {
        parmLen = sizeof(parmbuf) - 1 - 2;
    }
    if (parmLen < 0) parmLen = 0;

    /* We copy the parameter into our own area because
       the caller hasn't necessarily allocated room for
       a terminating NUL, nor is it necessarily correct
       to clobber the caller's area with NULs. */
    memset(parmbuf, 0, sizeof(parmbuf));
    if (cppl) {
		/* TSO */
        parmLen -= 4;
        memcpy(parmbuf, p+4, parmLen);
    }
    else {
        memcpy(parmbuf, p+2, parmLen);
    }

    p = parmbuf;

    if (pgmr1) {
        /* save the program parameter list values (max 10 pointers)
           note: the first pointer is always the raw EXEC PGM=...,PARM
           or CPPL (TSO) address.
        */
        for(x=0; x < 10; x++) {
            u = (unsigned)pgmr1[x];
            /* add to array of pointers from caller */
            arrayadd(&grt->grtptrs, (void*)(u&0x7FFFFFFF));
            if (u&0x80000000) break; /* end of VL style address list */
        }
    }

    if (cppl) {
        argv[0] = p;
        for(x=0;x<=progLen;x++) {
            if (argv[0][x]==' ') {
                argv[0][x]=0;
                break;
            }
        }
        p += progLen;
    }
    else {       /* batch or tso "call" */
        argv[0] = pgmname;
        pgmname[8] = '\0';
        pgmname = strchr(pgmname, ' ');
        if (pgmname) *pgmname = '\0';
    }

    while (*p == ' ') p++;

    x = 1;
    if (*p) {
        while(x < MAXPARMS) {
            char srch = ' ';

            if (*p == '"') {
                /* double quoted string */
                p++;
                srch = '"';
            }
            else if (*p == '\'') {
                /* single quoted string */
                p++;
                srch = '\'';
            }
            
            argv[x++] = p;
            p = strchr(p, srch);
            if (!p) break;

            *p = '\0';
            p++;
            /* skip trailing blanks */
            while (*p == ' ') p++;
            if (*p == '\0') break;
        }
    }
    argv[x] = NULL;
    argc = x;

	/* NOTE: We did not set the TSO flag in the GRT to prevent
	 * fopen() from inserting a dataset "prefix" in the dataset
	 * named passed to fopen() as that messes with Lua processing.
	 */

    rc = mqttmain(argc, argv);

	if (!ispf_available()) {
		/* remove ESTAE */
		abendrpt(ESTAE_DELETE, DUMP_DEFAULT);
	}

    __exit(rc);
    return (rc);
}
