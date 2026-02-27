#include "mqttcli.h"

int mqtt_loop(MQTTCLI *mqttcli)
{
    int         rc      = 0;
    int         argc;
    char        **argv  = NULL;
    char        *p;
    char        buf[4096];
    
    do {
        /* clear buffer */
        memset(buf, 0, sizeof(buf));
        
        /* display prompt */
        printf("%s>\n", mqttcli->pgmname);
        
        /* read input from stdin */
        p = fgets(buf, sizeof(buf)-1, stdin);
        if (!p) break;
        
        /* make sure we have a null terminated buffer */
        buf[sizeof(buf)-1] = 0;
        
        /* remove any newline character */
        p = strchr(buf, '\n');
        if (p) *p = 0;
        
        /* parse buffer into argv array */
        argv = mqtt_args_new(buf);
        argc = array_count(&argv);
        
        /* parse command and switches */
        rc = mqtt_parse(mqttcli, argc, argv);
        
        /* free argv array */
        mqtt_args_free(&argv);
        
        if (rc) break;
        
        /* execute command */
        rc = mqtt_exec(mqttcli);
        if (rc) break;
    } while(!(mqttcli->flags & MQTTCLI_FLAG_EXIT));
    
    if (argv) mqtt_args_free(&argv);
    
    return rc;
}
