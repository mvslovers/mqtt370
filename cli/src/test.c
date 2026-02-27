#include <clibos.h>
#include <clibauth.h>

#include <cvt.h>
#include <ieecucm.h>    /* UCM - console devices */

#if 0
#include <ieebasea.h>   /* Master Scheduler Base, BASE */
#include <ieezb806.h>   /* MTTABLE, MTENTRY */
#endif

#if 0
#include <iefjesct.h>
#include <clibssct.h>
#include <clibssvt.h>
#include <haspsvt.h>
#endif

unsigned __stklen   = 1024 * 32;

static int privilege(int state);

int main(int argc, char **argv)
{
    unsigned        *psa        = (unsigned *)0;
    CVT             *cvt        = (CVT*) psa[0x10/4];
    UCM             *ucm        = (UCM*) cvt->cvtcucb;
    UCMLIST         *ucmlist    = ucm->ucmvea;
    unsigned        ucmlen      = ucm->ucmvez;
    UCMLIST         *ucmlast    = ucm->ucmvel;

    do {
        wtodumpf(ucmlist, ucmlen, "UCMLIST");
        ucmlist = (UCMLIST*)((unsigned)ucmlist + ucmlen);
    } while (ucmlist <= ucmlast);
    
#if 0
    BASE            *base       = (BASE*) cvt->cvtmser;
    unsigned        *word;

    privilege(1);

    wtof("%s: BASE          %p", __func__, base);
    wtof("%s: base->balog   %p", __func__, base->balog);
    wtodumpf(base->balog, 512, "base->balog");

    word = (unsigned*)base->balog;
    wtodumpf((void*)0x00EF9AE8, 512, "0x00EF9AE8");

    privilege(0);
#endif

#if 0
    unsigned    *psa        = (unsigned *)0;
    CVT         *cvt        = (CVT*) psa[0x10/4];
    BASE        *base       = (BASE*) cvt->cvtmser;
    MTTABLE     *mttable;
    MTENTRY     *mtentry;

    wtodumpf(psa, 32, "PSA");
   
    wtodumpf(cvt, sizeof(CVT), "CVT");
    
    wtodumpf(base, sizeof(BASE), "Master Scheduler Base");
    
    privilege(1);

    mttable = (MTTABLE*) base->bamttbl;
    wtodumpf(mttable, sizeof(MTTABLE), "mttable");
    
    mtentry = (MTENTRY*) mttable->mttcurpt;
    wtodumpf(mtentry, mtentry->mtentlen + 10, "mtentry");
    wtodumpf(mtentry->mtentdat, mtentry->mtentlen, "mtentdat"); 
    
    for(mtentry = (MTENTRY*) mttable->mttcurpt; 
        INBOUNDS(mttable, mtentry); 
        mtentry = (MTENTRY*)((unsigned)mtentry + mtentry->mtentlen + 10) ) {
        
        printf("%p %p %*.*s\n", mtentry, mtentry->mtentimm, mtentry->mtentlen, mtentry->mtentlen, mtentry->mtentdat);
    }

    printf("-----------------------------------------------------------\n");

    for(mtentry = (MTENTRY*) mttable->mttwrppt; 
        INBOUNDS(mttable, mtentry) && mtentry < mttable->mttcurpt; 
        mtentry = (MTENTRY*)((unsigned)mtentry + mtentry->mtentlen + 10) ) {
        
        printf("%p %p %*.*s\n", mtentry, mtentry->mtentimm, mtentry->mtentlen, mtentry->mtentlen, mtentry->mtentdat);
    }

    privilege(0);
#endif

#if 0
    wtodumpf(mtentry, mtentry->mtentlen + 10, "mtentry");
    
    mtentry = (MTENTRY*)((unsigned)mtentry + mtentry->mtentlen + 10);
    wtodumpf(mtentry, mtentry->mtentlen + 10, "mtentry");
    

    mtentry = (MTENTRY*) mttable->mttwrppt;
    wtodumpf(mtentry, mtentry->mtentlen + 10, "mtentry");
    
    mtentry = (MTENTRY*)((unsigned)mtentry + mtentry->mtentlen + 10);
    wtodumpf(mtentry, mtentry->mtentlen + 10, "mtentry");
    
    privilege(0);
#endif


#if 0    
    JESCT       *jesct      = (JESCT*) cvt->cvtjesct;
    SSCT        *ssct       = (SSCT*) jesct->jesssct;
    SSVT        *ssvt       = (SSVT*) ssct->ssctssvt;
    HASPSVT     *svt        = (HASPSVT*) ssct->ssctssvt;
    char        *p;
    
    wtodumpf(psa, 32, "PSA");
   
    wtodumpf(cvt, sizeof(CVT), "CVT");
    
    wtodumpf(cvt->cvtmser, 512, "cvtmser");
    
    wtodumpf(cvt->cvtcucb, 256, "cvt->cvtcucb");

    wtodumpf(cvt->cvtjesct, 32, "cvtjesct");
    
    wtodumpf(jesct, sizeof(JESCT), "JESCT");

    wtof("Primary subsystem name \"%-4.4s\"", jesct->jespjesn);
    wtof("First subsystem SSCT %p", ssct);
    
    wtodumpf(ssct, sizeof(SSCT), "SSCT");

    if (ssct->ssctssid == SSCTUNKN) {
        wtof("Subsystem type %s", "UNKNOWN");
    }
    else if (ssct->ssctssid == SSCTJES2) {
        wtof("Subsystem type %s", "JES2");
    }
    else if (ssct->ssctssid == SSCTJES3) {
        wtof("Subsystem type %s", "JES3");
    }

    wtodumpf(svt, sizeof(HASPSVT), "JES2 SVT");
#endif

#if 0
    p = (char*)ssvt->ssvtfrtn;
    wtodumpf(p, (ssvt->ssvtfnum * 4), "ssvtfrtn");
    p += (ssvt->ssvtfnum * 4);
    wtodumpf(p, 1024, "after ssvtfrtn");

    wtodumpf(&svt->svcmbfq0, 8, "JES2 SVT->svcmbfq0");
    wtof("svcmbfq0  = %p", svt->svcmbfq0);
    wtof("svcmbfq1  = %p (%d)", svt->svcmbfq1, svt->svcmbfq1);
    
    privilege(1);
    
    wtodumpf(svt->svcmbfq0, 4096, "svccmbfq");
    wtodumpf(svt->svcmbrq, 4096, "svcmbrq");
    wtodumpf(svt->svcmba, 4096, "svcmba");
    
    privilege(0);
#endif
    return 0;
}

// __asm("\nSVCPOST POST  *-*,ASCB=*-*,ERRET=$SVBR14,MF=L");

static int privilege(int state)
{
    int     rc      = 8;

    if (state) {
        /* check/set task as APF authorized */
        rc = __autask();
        if (rc) goto quit;

        /* set SUPERVISOR state, KEY 0 */
        rc = __super(PSWKEY0, NULL);
        goto quit;
    }

    /* change to PROBLEM state */
    rc = __prob(PSWKEY8, NULL);
    __uatask();    /* reset APF authorization  */
    
quit:
    return rc;
}

// __asm("IEECUCM FORMAT=NEW");
// __asm("IEEBASEA");
// __asm("LINK  EP=IEEVIPL,SF=L");
__asm("\nIEECDCM DSECT ,"); 
__asm("IEECDCM");
