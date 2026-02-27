#include "mqttcli.h"

int mqtt_dump(MQTTCLI *mqttcli)
{
    MQTC    *mqtc       = mqttcli->mqtc;
    
    fprintf(stderr, "------------------------------------------------\n");
    fprintf(stderr, "Program            %s\n", mqttcli->pgmname);
    fprintf(stderr, "mqtccli            %p\n");
    fprintf(stderr, "mqttcli->mqtc      %p\n", mqtc);
    fprintf(stderr, "mqttcli->flags     0x%02X\n", mqttcli->flags);
    if (mqttcli->flags & MQTTCLI_FLAG_CONN) {
        fprintf(stderr, "mqttcli->flags     + 0x%02X==CONNECT\n", MQTTCLI_FLAG_CONN);
    }
    if (mqttcli->flags & MQTTCLI_FLAG_EXIT) {
        fprintf(stderr, "mqttcli->flags     + 0x%02X==EXIT\n", MQTTCLI_FLAG_EXIT);
    }
    if (mqttcli->flags & MQTTCLI_FLAG_POLL) {
        fprintf(stderr, "mqttcli->flags     + 0x%02X==POLL\n", MQTTCLI_FLAG_POLL);
    }
    fprintf(stderr, "mqtccli->host      \"%s\"\n", mqttcli->host ? mqttcli->host : "(null)");
    fprintf(stderr, "mqtccli->port      \"%s\"\n", mqttcli->port ? mqttcli->port : "(null)");
    fprintf(stderr, "mqtccli->user      \"%s\"\n", mqttcli->user ? mqttcli->user : "(null)");
    fprintf(stderr, "mqtccli->pass      \"%s\"\n", mqttcli->pass ? mqttcli->pass : "(null)");
    fprintf(stderr, "mqtccli->topic     \"%s\"\n", mqttcli->topic ? mqttcli->topic : "(null)");
    fprintf(stderr, "mqtccli->msg       \"%s\"\n", mqttcli->msg ? mqttcli->msg : "(null)");
    fprintf(stderr, "mqtccli->verb      \"%s\"\n", mqttcli->verb ? mqttcli->verb : "(null)");
    fprintf(stderr, "mqttcli->qos       \"%u\"\n", mqttcli->qos);
    fprintf(stderr, "mqttcli->retain    \"%s\"\n", mqttcli->retain ? "True" : "False");
    fprintf(stderr, "------------------------------------------------\n");
    
    return 0;
}

