#include "mqttcli.h"

int mqtt_parse(MQTTCLI *mqttcli, int argc, char **argv)
{
	int		rc 		= 0;
	int		i;
	char	*p;

    util_free_char(&mqttcli->verb);
    
	/* process command line args */
	for(i=0; i < argc; i++) {
		p = argv[i];
		
		if (!p) continue;
		// wtof("%s: argv[%d]=\"%s\"", __func__, i, p);
		
		if (*p=='-') {
			strupper(p);
			p++;
			switch(*p) {
            case 'H':   /* host name */
                util_free_char(&mqttcli->host);
                p++;
                if (!*p) {
                    if (i < argc) p = argv[++i];
                }
                else {
                    fprintf(stderr, "Missing host name.\n");
                    rc = 4;
                    break;
                }
                mqttcli->host = strdup(p);
                break;
            case 'M':   /* message string */
                util_free_char(&mqttcli->msg);
                p++;
                if (!*p) {
                    if (i < argc) p = argv[++i];
                }
                else {
                    fprintf(stderr, "Missing message.\n");
                    rc = 4;
                    break;
                }
                mqttcli->msg = strdup(p);
                break;
            case 'N':   /* port number */
                util_free_char(&mqttcli->port);
                p++;
                if (!*p) {
                    if (i < argc) p = argv[++i];
                }
                else {
                    fprintf(stderr, "Missing port number.\n");
                    rc = 4;
                    break;
                }
                mqttcli->port = strdup(p);
                break;
            case 'P':   /* password */
                util_free_char(&mqttcli->pass);
                p++;
                if (!*p) {
                    if (i < argc) p = argv[++i];
                }
                else {
                    fprintf(stderr, "Missing password.\n");
                    rc = 4;
                    break;
                }
                mqttcli->pass = strdup(p);
                break;
            case 'Q':   /* QOS (0,1,2) */
                for(p++; isspace(*p); p++) ;
                if (!*p) {
                    if (i < argc) p = argv[++i];
                }
                else {
                    fprintf(stderr, "Missing qos level.\n");
                    rc = 4;
                    break;
                }
                mqttcli->qos = (unsigned char) strtoul(p, NULL, 0);
                if (mqttcli->qos > 2) mqttcli->qos = 2;
                break;
            case 'R':   /* retain (bool) */
                for(p++; isspace(*p); p++) ;
                if (!*p) {
                    if (i < argc) p = argv[++i];
                }
                else {
                    fprintf(stderr, "Missing retain value.\n");
                    rc = 4;
                    break;
                }
                if (isdigit(*p)) {
                    mqttcli->retain = (unsigned char) strtoul(p, NULL, 0);
                    if (mqttcli->retain > 1) mqttcli->retain = 1;
                }
                else if (*p=='t' || *p=='T' || *p=='y' || *p=='Y') {
                    mqttcli->retain = 1;
                }
                else if (*p=='f' || *p=='F' || *p=='n' || *p=='N') {
                    mqttcli->retain = 0;
                }
                else {
                    fprintf(stderr, "Invalid retain value\n");
                    rc = 4;
                }
                break;
            case 'T':   /* topic string */
                util_free_char(&mqttcli->topic);
                p++;
                if (!*p) {
                    if (i < argc) p = argv[++i];
                }
                else {
                    fprintf(stderr, "Missing topic.\n");
                    rc = 4;
                    break;
                }
                mqttcli->topic = strdup(p);
                break;
            case 'U':   /* userid */
                util_free_char(&mqttcli->user);
                p++;
                if (!*p) {
                    if (i < argc) p = argv[++i];
                }
                else {
                    fprintf(stderr, "Missing userid.\n");
                    rc = 4;
                    break;
                }
                mqttcli->user = strdup(p);
                break;
			default:
				fprintf(stderr, "Invalid command line flag: \"-%s\"\n", p);
				rc = 4;
				break;
			}

			continue;
		}

        if (!*p) continue;
        
        if (!mqttcli->verb) {
            mqttcli->verb = strdup(p);
            strupper(mqttcli->verb);
        }
        else {
            fprintf(stderr, "Unexpected parameter: \"%s\"\n", p);
            rc = 4;
        }
	}

	if (rc) goto quit;

    /* extract port from host name */
    if (mqttcli->host) {
        p = strchr(mqttcli->host, ':');
        if (p) {
            *p++ = 0;
            while(*p==':' || isspace(*p)) p++;
            if (*p && !mqttcli->port) mqttcli->port = strdup(p);
        }
    }

    /* set defaults */
    if (!mqttcli->port) mqttcli->port = strdup("1883");

quit:	
	return rc;
}

