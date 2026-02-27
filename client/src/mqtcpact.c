#include "mqtc370.h"

static int packet_dump(FILE *fp, const char *title, void *varea, int size);

int mqtc_packet_trace(MQTC *mqtc, CPB *pb, int b_sent)
{
    int         rc      = 0;
    char        title[40];
    
    if (!mqtc->pkttrc) goto quit;
    if (!pb) goto quit;
    
    sprintf(title, "%s %s", b_sent ? "SENT" : "RECV", mqtc_packet_type(pb));

    rc = packet_dump(mqtc->pkttrc, title, pb->buf, pb->bufsize);
    if (rc < 0) {
        wtof("MQTC0010E Packet trace file error.");
        fclose(mqtc->pkttrc);
        mqtc->pkttrc = NULL;
    }
    rc = 0;
    
quit:
    return rc;
}

static const char dash[] = " "
    "--------------------------------------"
    "--------------------------------------"
    "-------------------------------------\n";
static int
packet_dump(FILE *fp, const char *title, void *varea, int size)
{
    const int       chunk   = 32;
    const char      *dup    = "";
    const char      *retain = "";
    int             rc      = 0;
    int             i, j;
    int             iHex    = 0;
    int             ie      = 0;
    int             x       = (chunk * 2) + ((chunk / 4) - 1);
    unsigned char   *area   = (unsigned char*)varea;
    unsigned char   e;
    unsigned char   qos[10] = "";
    char            sHex[100];
    char            eChar[40];
    mtime64_t       mnow;
    time64_t        now;
    struct tm       *tm;
    char            timestamp[40];

    fwrite(dash, sizeof(dash), 1, fp);

    mtime64(&mnow);
    __64_divmod_u32(&mnow, 1000, &now, &mnow);
    tm = localtime64(&now);
    i = strftime(timestamp, sizeof(timestamp), "%Y-%m-%d  %H:%M:%S", tm);
    sprintf(&timestamp[i], ".%03u", mnow.u32[1]);
    
    e = area[0] & 0x0F;
    if (e & 0x08)   dup = "DUP";
    if (e & 0x01)   retain = "RETAIN";
    e = (e & 0x06) >> 1;
    if (e) sprintf(qos, "QOS(%u)", e);

    rc = fprintf(fp, "%s %s (%d bytes) %s %s %s\n", timestamp, title, size, dup, qos, retain);
    if (rc < 0) goto quit;

    for (i=0, j=0; size > 0;i++ ) {
        if ( i==chunk ) {
            rc = fprintf(fp, " +%05X %-*.*s :%-*.*s:\n",
                j, x, x, sHex, chunk, chunk, eChar);
            if (rc < 0) goto quit;
            j += i;
            ie = iHex = i = 0;
        }
        iHex += sprintf(&sHex[iHex],"%02X",*area);
        if ((i & 3) == 3) iHex += sprintf(&sHex[iHex]," ");

        e = *area;
        e = utila2e[e];

        ie += sprintf(&eChar[ie],"%c", isgraph(e)?e:e==' '?e:'.');
        area++;
        size--;
    }

    if ( iHex ) {
        rc = fprintf(fp, " +%05X %-*.*s :%-*.*s:\n",
            j, x, x, sHex, chunk, chunk, eChar);
    }

quit:
    return rc;
}
