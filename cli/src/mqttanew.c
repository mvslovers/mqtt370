#include "mqttcli.h"

char **mqtt_args_new(char *str)
{
    char    *p      = str;
    char    **argv  = NULL;
    int     x       = 0;
    
    if (!p) goto quit;
    
    /* skip any leading spaces */
    while (*p == ' ') p++;

    while (*p) {
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

        str = p;
        p = strchr(str, srch);
        if (p) {
            *p = 0;
            p++;
        }

        array_addf(&argv, "%s", str);
        if (!p) break;

        /* skip trailing blanks */
        while (*p == ' ') p++;
    }

quit:
    return argv;
}
