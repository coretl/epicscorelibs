
/*
 *
 * escape into C to call signal because of a brain dead
 * signal() func proto supplied in signal.h by gcc 2.7.2
 *
 */

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "sigPipeIgnore.h"

typedef void (*pSigFunc) ();

static pSigFunc pReplacedFunc;

#ifndef UNIX // all os except UNIX
 
void installSigPipeIgnore (void)
{
}
 
#else // it is UNIX 

static void localInstallSigPipeIgnore (void);

/*
 * ignoreSigPipe ()
 */
static void ignoreSigPipe (int param)
{
	if (pReplacedFunc) {
		(*pReplacedFunc) (param);
	}
	/*
	 * some versios of unix reset to SIG_DFL
	 * each time that the signal occurs
	 */
	localInstallSigPipeIgnore ();
}

/*
 * installSigPipeIgnore ()
 */
void installSigPipeIgnore (void)
{
	static int init;

	if (init) {
		return;
	}
	localInstallSigPipeIgnore();
	init = 1;
}

/*
 * localInstallSigPipeIgnore ()
 *
 * dont allow disconnect to terminate process
 * when running in UNIX environment
 *
 * allow error to be returned to sendto()
 * instead of handling disconnect at interrupt
 */
static void localInstallSigPipeIgnore (void)
{
	pSigFunc sigRet;

	sigRet = signal (SIGPIPE, ignoreSigPipe);
	if (sigRet==SIG_ERR) {
		fprintf (stderr, "%s replace of SIGPIPE failed beacuse %s\n", 
			__FILE__, strerror(errno));
	}
	else if (sigRet!=SIG_DFL && sigRet!=SIG_IGN) {
		pReplacedFunc = sigRet;
	}
	/*
	 * no infinite loops 
	 */
	if (pReplacedFunc==ignoreSigPipe) {
		pReplacedFunc = NULL;
	}
}

#endif // UNIX 

