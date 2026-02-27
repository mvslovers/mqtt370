#include "mqttcli.h"

int mqttmain(int argc, char **argv)
{
    int         rc          = 8;
    MQTTCLI     *mqttcli    = NULL;

#if 0   /* test code for Mike G */
    wtof("before privileg(1)");
    privileg(1);
    wtof("after privileg(1) %s", __issup() ? "SUPERVISOR" : "PROBLEM");
    privileg(0);
    wtof("after privileg(0) %s", __issup() ? "SUPERVISOR" : "PROBLEM");
#endif

    printf("MQTT Command Line Interface - version 1.0\n");

    /* allocate MQTT command line handle */
    mqttcli = mqttcli_new();
    if (!mqttcli) goto quit;

    /* save the program name as the prompt string */
    mqttcli->pgmname = argv[0];

    /* parse remaining argv strings */
    rc = mqtt_parse(mqttcli, --argc, ++argv);
    if (rc) goto quit;

    /* execute command/verb */
    rc = mqtt_exec(mqttcli);
    if (rc) goto quit;          /* processing error */
    if (mqttcli->flags & MQTTCLI_FLAG_EXIT) goto quit;
    if (mqttcli->verb) {
        if (strcmp(mqttcli->verb, "PUB")==0 || strcmp(mqttcli->verb, "PUBLISH")==0) goto quit;
    }
    
    /* read input, create args, execute, release args, loop */
    rc = mqtt_loop(mqttcli);

quit:
    if (mqttcli) mqttcli_free(mqttcli);
    return rc;
}
