#include "mqttcli.h"

static int find(const void *key, const void *item);

typedef struct {
    char    *word;                  /* keyword name         */
    int     len;                    /* keyword length       */
    int     (*func)(MQTTCLI *mqttcli);    /* function pointer     */
} MQTT_CMDS;

static MQTT_CMDS cmds[] = {
	{"BYE",		3, mqtt_bye},       /* terminate MQTT program       */
    {"CON",     3, mqtt_connect},   /* connect to broker            */
    {"CONN",    4, mqtt_connect},   /* connect to broker            */
    {"CONNECT", 7, mqtt_connect},   /* connect to broker            */
    {"DUMP",    4, mqtt_dump},      /* dump mqttcli handle          */
	{"END",		3, mqtt_bye},       /* terminate MQTT program       */
	{"EXIT",	4, mqtt_bye},       /* terminate MQTT program       */
    {"PUB",     3, mqtt_pub},       /* publish                      */
    {"PUBLISH", 7, mqtt_pub},       /* publish                      */
	{"QUIT",    4, mqtt_bye},       /* terminate MQTT program       */
};
#define MAX_CMDS (sizeof(cmds) / sizeof(MQTT_CMDS))

int 
mqtt_exec(MQTTCLI *mqttcli)
{
    int         rc          = 0;
	int			resp	    = 0;
    MQTT_CMDS   *cmd        = NULL;

    // mqtt_dump(mqttcli);

    /* check for command/verb to execute */
    if (!mqttcli->verb) goto quit;

    cmd = bsearch(mqttcli->verb, cmds, MAX_CMDS, sizeof(MQTT_CMDS), find);
	if (!cmd) {
		/* command not found */
		printf("Unknown command \"%s\"\n", mqttcli->verb);
		rc = 4;
		goto quit;
	}

    /* call function */
	rc = cmd->func(mqttcli);

#if 0
    /* are we done? */
    if (strcmp(mqttcli->verb, "EXIT")==0 ||
        strcmp(mqttcli->verb, "QUIT")==0 ||
        strcmp(mqttcli->verb, "END")==0 ||
        strcmp(mqttcli->verb, "BYE")==0) {
        mqttcli->flags |= MQTTCLI_FLAG_EXIT;
        goto quit;
    }
#endif
    
quit:
    return rc;
}

static int
find(const void *vitem, const void *vkey)
{
    MQTT_CMDS   *item   = (MQTT_CMDS *)vitem;
    char        *key    = (char *)vkey;
    int         rc;
    /* compare key with keyword ignoring case */
    rc =  strncmpi(item->word, key, item->len);
    if (rc==0) {
        // wtof("%s: item->word=\"%s\" key=\"%s\" item->len=%d", __func__, item->word, key, item->len);
        if (key[item->len] == ' ' || key[item->len] == 0) {
            /* looks good to me */
        }
        else {
            rc = -1;
        }
    }
    return rc;
}
