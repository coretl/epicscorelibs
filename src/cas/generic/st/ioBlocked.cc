
//
// $Id$
// Author Jeff Hill
//
// IO Blocked list class
// (for single threaded version of the server)
//
// $Log$
// Revision 1.1  1996/11/02 01:01:34  jhill
// installed
//
//

#include <stdio.h>

#include "casdef.h"
#include "osiMutexNOOP.h"


//
// ioBlocked::~ioBlocked()
//
ioBlocked::ioBlocked() :
	pList(NULL)
{
}

//
// ioBlocked::~ioBlocked()
//
ioBlocked::~ioBlocked()
{
}

//
// ioBlocked::ioBlockedSignal()
//
void ioBlocked::ioBlockedSignal()
{
	//
	// NOOP
	//
}

//
// ioBlockedList::ioBlockedList ()
//
ioBlockedList::ioBlockedList () 
{
}
 
//
// ioBlockedList::~ioBlockedList ()
//
ioBlockedList::~ioBlockedList ()
{
        ioBlocked *pB;
 
        while ( (pB = this->get ()) ) {
                pB->pList = NULL;
        }
}
 
//
// ioBlockedList::signal()
//
// works from a temporary list to avoid problems
// where the virtual function adds items to the
// list
//
void ioBlockedList::signal()
{
	tsDLList<ioBlocked> tmp(*this);
        ioBlocked *pB;

        while ( (pB = tmp.get ()) ) {
                pB->pList = NULL;
                pB->ioBlockedSignal ();
        }
}

//
// ioBlockedList::removeItemFromIOBLockedList()
//
void ioBlockedList::removeItemFromIOBLockedList(ioBlocked &item)
{
        if (item.pList==this) {
                this->remove(item);
                item.pList = NULL;
        }
}
 
//
// ioBlockedList::addItemToIOBLockedList()
//
void ioBlockedList::addItemToIOBLockedList(ioBlocked &item)
{
        if (item.pList==NULL) {
                this->add(item);
                item.pList = this;
        }
        else {
                assert(item.pList == this);
        }
}
 

