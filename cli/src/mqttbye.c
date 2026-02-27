#include "mqttcli.h"

int mqtt_bye(MQTTCLI *mqttcli)
{
    mqttcli->flags |= MQTTCLI_FLAG_EXIT;
    
    return 0;
}
