#include "mqttcli.h"

void mqttcli_free(MQTTCLI *mqttcli)
{
    MQTC        *mqtc;
    
    if (!mqttcli) goto quit;

    mqtc = mqttcli->mqtc;
    if (mqtc) mqtc_free_client(&mqttcli->mqtc);

    util_free_char(&mqttcli->host);
    util_free_char(&mqttcli->port);
    util_free_char(&mqttcli->user);
    util_free_char(&mqttcli->pass);
    util_free_char(&mqttcli->topic);
    util_free_char(&mqttcli->msg);
    util_free_char(&mqttcli->verb);

    free(mqttcli);

quit:
    return;
}
