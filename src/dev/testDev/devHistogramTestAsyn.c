/* devHistogramTestAsyn.c */
/* base/src/dev $Id$ */
/*
 *      Author:		Janet Anderson
 *      Date:		07/02/91
 *
 *      Experimental Physics and Industrial Control System (EPICS)
 *
 *      Copyright 1991, the Regents of the University of California,
 *      and the University of Chicago Board of Governors.
 *
 *      This software was produced under  U.S. Government contracts:
 *      (W-7405-ENG-36) at the Los Alamos National Laboratory,
 *      and (W-31-109-ENG-38) at Argonne National Laboratory.
 *
 *      Initial development by:
 *              The Controls and Automation Group (AT-8)
 *              Ground Test Accelerator
 *              Accelerator Technology Division
 *              Los Alamos National Laboratory
 *
 *      Co-developed with
 *              The Controls and Computing Group
 *              Accelerator Systems Division
 *              Advanced Photon Source
 *              Argonne National Laboratory
 *
 * Modification Log:
 * -----------------
 * .01  11-11-91        jba     Moved set of alarm stat and sevr to macros
 * .02  01-08-92        jba     Added cast in call to wdStart to avoid compile warning msg
 * .03  02-05-92	jba	Changed function arguments from paddr to precord 
 * .04	03-13-92	jba	ANSI C changes
 * .05  04-10-92        jba     pact now used to test for asyn processing, not return value
 * .06  04-05-94        mrk	ANSI changes to callback routines
 *      ...
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "callback.h"
#include "cvtTable.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "recSup.h"
#include "devSup.h"
#include "link.h"
#include "dbCommon.h"
#include "histogramRecord.h"

/* Create the dset for devHistogramTestAsyn */
static long init_record();
static long read_histogram();
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_histogram;
	DEVSUPFUN	special_linconv;
}devHistogramTestAsyn={
	6,
	NULL,
	NULL,
	init_record,
	NULL,
	read_histogram,
	NULL};

static long init_record(phistogram)
    struct histogramRecord	*phistogram;
{
    CALLBACK *pcallback;

    /* histogram.svl must be a CONSTANT*/
    switch (phistogram->svl.type) {
    case (CONSTANT) :
	pcallback = (CALLBACK *)(calloc(1,sizeof(CALLBACK)));
	phistogram->dpvt = (void *)pcallback;
        if(recGblInitConstantLink(&phistogram->svl,DBF_DOUBLE,&phistogram->sgnl))
            phistogram->udf = FALSE;
	break;
    default :
	recGblRecordError(S_db_badField,(void *)phistogram,
	    "devHistogramTestAsyn (init_record) Illegal SVL field");
	return(S_db_badField);
    }
    return(0);
}

static long read_histogram(phistogram)
    struct histogramRecord	*phistogram;
{
    CALLBACK *pcallback=(CALLBACK *)(phistogram->dpvt);

    /* histogram.svl must be a CONSTANT*/
    switch (phistogram->svl.type) {
    case (CONSTANT) :
	if(phistogram->pact) {
		printf("%s Completed\n",phistogram->name);
		return(0); /*add count*/
	} else {
                if(phistogram->disv<=0) return(2);
                printf("Starting asynchronous processing: %s\n",
                    phistogram->name);
                phistogram->pact=TRUE;
                callbackRequestProcessCallbackDelayed(
                    pcallback,phistogram->prio,phistogram,
                    (double)phistogram->disv);
		return(0);
	}
    default :
        if(recGblSetSevr(phistogram,SOFT_ALARM,INVALID_ALARM)){
		if(phistogram->stat!=SOFT_ALARM) {
			recGblRecordError(S_db_badField,(void *)phistogram,
			    "devHistogramTestAsyn (read_histogram) Illegal SVL field");
		}
	}
    }
    return(0);
}
