
/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

#ifndef caServerIh
#define caServerIh

#ifdef epicsExportSharedSymbols
#   define epicsExportSharedSymbols_caServerIh
#   undef epicsExportSharedSymbols
#endif

// external headers included here
#include "tsFreeList.h"
#include "caProto.h"

#ifdef epicsExportSharedSymbols_caServerIh
#   define epicsExportSharedSymbols
#   include "shareLib.h"
#endif

#include "clientBufMemoryManager.h"
#include "casEventRegistry.h"
#include "caServerIO.h"
#include "ioBlocked.h"
#include "caServerDefs.h"

class beaconTimer;
class beaconAnomalyGovernor;
class casIntfOS;
class casMonitor;

class caServerI : 
	public caServerIO, 
	public ioBlockedList, 
	public casEventRegistry {
public:
	caServerI ( caServer &tool );
	~caServerI ();
	bool roomForNewChannel() const;
	unsigned getDebugLevel() const { return debugLevel; }
	inline void setDebugLevel ( unsigned debugLevelIn );
	void show ( unsigned level ) const;
    void destroyMonitor ( casMonitor & );
	caServer * getAdapter ();
	caServer * operator -> ();
	void connectCB ( casIntfOS & );
	casEventMask valueEventMask () const; // DBE_VALUE registerEvent("value")
	casEventMask logEventMask () const; 	// DBE_LOG registerEvent("log") 
	casEventMask alarmEventMask () const; // DBE_ALARM registerEvent("alarm") 
    unsigned subscriptionEventsProcessed () const;
    void incrEventsProcessedCounter ();
    unsigned subscriptionEventsPosted () const;
    void updateEventsPostedCounter ( unsigned nNewPosts );
    void generateBeaconAnomaly ();
    casMonitor & casMonitorFactory (  casChannelI &, 
        caResId clientId, const unsigned long count, 
        const unsigned type, const casEventMask &, 
        class casMonitorCallbackInterface & );
    void casMonitorDestroy ( casMonitor & );
    void destroyClient ( casStrmClient & );
private:
    clientBufMemoryManager clientBufMemMgr;
    tsFreeList < casMonitor, 1024 > casMonitorFreeList;
	tsDLList < casStrmClient > clientList;
    tsDLList < casIntfOS > intfList;
	mutable epicsMutex mutex;
	mutable epicsMutex diagnosticCountersMutex;
	caServer & adapter;
    beaconTimer & beaconTmr;
    beaconAnomalyGovernor & beaconAnomalyGov;
	unsigned debugLevel;
    unsigned nEventsProcessed; 
    unsigned nEventsPosted; 

    casEventMask valueEvent; // DBE_VALUE registerEvent("value")
	casEventMask logEvent; 	// DBE_LOG registerEvent("log") 
	casEventMask alarmEvent; // DBE_ALARM registerEvent("alarm")

	caStatus attachInterface (const caNetAddr &addr, bool autoBeaconAddr,
			bool addConfigAddr);

    void sendBeacon ( ca_uint32_t beaconNo );

	caServerI ( const caServerI & );
	caServerI & operator = ( const caServerI & );

    friend class beaconAnomalyGovernor;
    friend class beaconTimer;
};


inline caServer * caServerI::getAdapter()
{
	return & this->adapter;
}

inline caServer * caServerI::operator -> ()
{
	return this->getAdapter();
}

inline void caServerI::setDebugLevel(unsigned debugLevelIn)
{
	this->debugLevel = debugLevelIn;
}

inline casEventMask caServerI::valueEventMask() const
{
    return this->valueEvent;
}

inline casEventMask caServerI::logEventMask() const
{
    return this->logEvent;
}

inline casEventMask caServerI::alarmEventMask() const
{
    return this->alarmEvent;
}

#endif // caServerIh