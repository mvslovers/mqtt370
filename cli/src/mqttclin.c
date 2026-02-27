#include "mqttcli.h"

MQTTCLI *mqttcli_new(void)
{
    MQTTCLI     *mqttcli    = calloc(1, sizeof(MQTTCLI));
    
    if (!mqttcli) goto quit;

    strcpy(mqttcli->eye, MQTTCLI_EYE);
    
    mqttcli->mqtc = mqtc_new_client();
    if (!mqttcli->mqtc) {
        mqttcli_free(mqttcli);
        mqttcli = NULL;
    }

quit:
    return mqttcli;
}
