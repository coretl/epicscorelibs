// Author: Jim Kowalkowski
// Date: 2/96
// 
// $Id$
// 
// $Log$
//

// *Revision 1.4  1996/06/25 18:59:01  jbk
// *more fixes for the aitString management functions and mapping menus
// *Revision 1.3  1996/06/24 03:15:32  jbk
// *name changes and fixes for aitString and fixed string functions
// *Revision 1.2  1996/06/13 21:31:55  jbk
// *Various fixes and correction - including ref_cnt change to unsigned short
// *Revision 1.1  1996/05/31 13:15:24  jbk
// *add new stuff

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "gdd.h"

gdd_NEWDEL_NEW(gddBounds1D)
gdd_NEWDEL_DEL(gddBounds1D)
gdd_NEWDEL_STAT(gddBounds1D)

gdd_NEWDEL_NEW(gddBounds2D)
gdd_NEWDEL_DEL(gddBounds2D)
gdd_NEWDEL_STAT(gddBounds2D)

gdd_NEWDEL_NEW(gddBounds3D)
gdd_NEWDEL_DEL(gddBounds3D)
gdd_NEWDEL_STAT(gddBounds3D)

gdd_NEWDEL_NEW(gddDestructor)
gdd_NEWDEL_DEL(gddDestructor)
gdd_NEWDEL_STAT(gddDestructor)

gdd_NEWDEL_NEW(gdd)
gdd_NEWDEL_DEL(gdd)
gdd_NEWDEL_STAT(gdd)

// --------------------------The gddBounds functions-------------------

// gddBounds::gddBounds(void) { first=0; count=0; }

// --------------------------The gddDestructor functions-------------------

gddStatus gddDestructor::destroy(void* thing)
{
	if(ref_cnt==0 || --ref_cnt==0)
	{
		run(thing);
		delete this;
	}
	return 0;
}

void gddDestructor::run(void* thing)
{
	aitInt8* pd = (aitInt8*)thing;
	delete [] pd;
}

void gddFlattenDestructor::run(void*)
{
	return;
}

void gddContainerCleaner::run(void* v)
{
	gddContainer* cdd = (gddContainer*)v;
	int tot = cdd->total();
	int i;
	for(i=0;i<tot;i++) cdd->remove(0);
}

// --------------------------The gdd functions-------------------------

gdd::gdd(int app, aitEnum prim, int dimen)
{
	init(app,prim,dimen);
}

gdd::gdd(int app, aitEnum prim, int dimen, aitUint32* val)
{
	int i;
	init(app,prim,dimen);
	for(i=0;i<dimen;i++) bounds[i].set(0,val[i]);
}

void gdd::init(int app, aitEnum prim, int dimen)
{
	setApplType(app);
	setPrimType(prim);
	dim=(aitUint8)dimen;
	setData(NULL);
	destruct=NULL;
	ref_cnt=1;
	flags=0;
	bounds=NULL;

	if(dim)
	{
		switch(dim)
		{
		case 1:  bounds=(gddBounds*)new gddBounds1D; bounds->set(0,0); break;
		case 2:  bounds=(gddBounds*)new gddBounds2D; break;
		case 3:  bounds=(gddBounds*)new gddBounds3D; break;
		default: bounds=(gddBounds*)new gddBounds[dim]; break;
		}
	}
}

gdd::gdd(gdd* dd)
{
	ref_cnt=1;
	copyInfo(dd);
}

gdd::~gdd(void)
{
	gdd* dd;
	gdd* temp;

	// fprintf(stderr,"A gdd is really being deleted %8.8x!!\n",this);

	// this function need to be corrected for use of aitEnumString!

	if(isScaler())
	{
		if(primitiveType()==aitEnumFixedString)
		{
			// aitString type could have destructors
			if(destruct)
				destruct->destroy(dataPointer());
			else
				if(data.FString) delete [] data.FString;
		}
		else if(primitiveType()==aitEnumString)
		{
			// aitString type could have destructors
			if(destruct)
				destruct->destroy(dataAddress());
			else
			{
				aitString* s = (aitString*)&data;
				s->clear();
			}
		}
	}
	else if(isContainer())
	{
		if(destruct)
			destruct->destroy(voidData());
		else
		{
			for(dd=(gdd*)voidData();dd;)
			{
				temp=dd;
				dd=dd->next();
				temp->unreference();
			}
			freeBounds();
		}
	}
	else
	{
		if(destruct) destruct->destroy(voidData());
		if(bounds) freeBounds();
	}
}

void gdd::freeBounds(void)
{
	if(bounds)
	{
		switch(dim)
		{
		case 0: break;
		case 1: { gddBounds1D* d1=(gddBounds1D*)bounds; delete d1; } break;
		case 2: { gddBounds2D* d2=(gddBounds2D*)bounds; delete d2; } break;
		case 3: { gddBounds3D* d3=(gddBounds3D*)bounds; delete d3; } break;
		default: delete bounds; break;
		}
		bounds=NULL;
	}
}

gddStatus gdd::registerDestructor(gddDestructor* dest)
{
	// this is funky, will not register a destructor if one is present
	if(destruct)
		return gddErrorAlreadyDefined;
	else
		return replaceDestructor(dest);
}

gddStatus gdd::replaceDestructor(gddDestructor* dest)
{
	destruct=dest;

	if(isContainer()||isFlat())
		markManaged();

	return 0;
}

gddStatus gdd::genCopy(aitEnum t, const void* d)
{
	size_t sz;
	aitInt8* buf;
	gddStatus rc=0;

	if(isScaler())
		aitConvert(primitiveType(),&data,t,d,1);
	else if(isAtomic())
	{
		if(!voidData())
		{
			sz=DescribedDataSizeBytes();
			if((buf=new aitInt8[sz])==NULL)
				rc=gddErrorNewFailed;
			else
			{
				setData(buf);
				destruct=new gddDestructor;
				aitConvert(primitiveType(),voidData(),t,d,getDataSizeElements());
			}
		}
		else
			aitConvert(primitiveType(),voidData(),t,d,getDataSizeElements());
	}
	else
		rc=gddErrorTypeMismatch;

	return rc;
}


gddStatus gdd::changeType(int app,aitEnum prim)
{
	gddStatus rc=0;

	// this should only be allowed for setting the type if it is
	// undefined or if the data is a scaler

	if(isScaler() || primitiveType()==aitEnumInvalid)
	{
		setApplType(app);
		setPrimType(prim);
	}
	else
		rc=gddErrorTypeMismatch;

	return rc;
}

gddStatus gdd::setBound(unsigned index_dim, aitIndex first, aitIndex count)
{
	gddStatus rc=0;
	if(index_dim<dimension())
		bounds[index_dim].set(first,count);
	else
		rc=gddErrorOutOfBounds;
	return rc;
}

gddStatus gdd::getBound(unsigned index_dim, aitIndex& first, aitIndex& count)
{
	gddStatus rc=0;
	if(index_dim<dimension())
		bounds[index_dim].get(first,count);
	else
		rc=gddErrorOutOfBounds;
	return rc;
}

// should the copy functions in gdd use the flatten technique?
gddStatus gdd::copyStuff(gdd* dd,int ctype)
{
	unsigned i;
	gddStatus rc=0;
	gddContainer* cdd;
	gdd *pdd,*ndd;

	// blow me out quickly here
	if(isFlat()||isManaged())
		return gddErrorNotAllowed;

	clear();

	if(dd->isContainer())
	{
		changeType(dd->applicationType(),dd->primitiveType());
		cdd=(gddContainer*)dd;
		gddCursor cur=cdd->getCursor();

		for(ndd=cur.first();ndd;ndd=cur.next())
		{
			pdd=new gdd(ndd->applicationType(),
				ndd->primitiveType(),ndd->dimension());
			pdd->setNext((gdd*)voidData());
			setData(pdd);
			bounds->setSize(bounds->size()+1);
			pdd->copyStuff(ndd,ctype);
		}
	}
	else
	{
		init(dd->applicationType(),dd->primitiveType(),dd->dimension());

		if(dd->isScaler())
			data=dd->data;
		else // atomic
		{
			const gddBounds* bnds = dd->getBounds();
			for(i=0;i<dd->dimension();i++) bounds[i]=bnds[i];
	
			switch(ctype)
			{
			case 1: // copy()
				aitUint8* array;
				size_t a_size;
				a_size=dd->getDataSizeBytes();
				if(array=new aitUint8[a_size])
				{
					destruct=new gddDestructor;
					memcpy(array,dd->voidData(),a_size);
					setData(array);
				}
				else
					rc=gddErrorNewFailed;
				break;
			case 2: // Dup()
				data=dd->getData(); // copy the data reference
				destruct=dd->destruct;
				if(destruct) destruct->reference();
				break;
			case 0: // copyInfo()
			default: break;
			}
		}
	}
	return rc;
}

size_t gdd::getDataSizeBytes(void) const
{
	size_t sz=0;
	gdd* pdd;

	if(isContainer())
	{
		const gddContainer* cdd=(const gddContainer*)this;
		gddCursor cur=cdd->getCursor();
		for(pdd=cur.first();pdd;pdd=cur.next())
			sz+=pdd->getTotalSizeBytes();
	}
	else
	{
		if(aitValid(primitiveType()))
		{
			if(primitiveType()==aitEnumString)
				sz+=(size_t)(aitString::totalLength((aitString*)dataAddress(),
					getDataSizeElements()));
			else
				sz+=(size_t)(getDataSizeElements())*aitSize[primitiveType()];
		}
	}
	return sz;
}

size_t gdd::DescribedDataSizeBytes(void) const
{
	size_t sz=0;

	// does not work well for aitString - only reports the aitString info

	if(!isContainer())
		sz+=(size_t)(DescribedDataSizeElements())*aitSize[primitiveType()];

	return sz;
}

size_t gdd::getTotalSizeBytes(void) const
{
	size_t sz;
	unsigned long tmp,tsize;
	gdd* pdd;

	// add up size of bounds + size of this DD
	sz=sizeof(gdd)+(sizeof(gddBounds)*dimension());

	// special case the aitString/aitFixedString here - sucks bad
	if(isScaler())
	{
		if(primitiveType()==aitEnumString)
		{
			aitString* str=(aitString*)dataAddress();
			sz+=str->length()+1; // include the NULL
		}
		else if(primitiveType()==aitEnumFixedString)
			sz+=sizeof(aitFixedString);
	}
	else if(isAtomic())
	{
		if(aitValid(primitiveType()))
		{
			if(primitiveType()==aitEnumString)
			{
				// special case the aitString here
				tsize=(size_t)(aitString::totalLength((aitString*)dataPointer(),
					getDataSizeElements()));
			}
			else
				tsize=(size_t)(getDataSizeElements())*aitSize[primitiveType()];

			sz+=(size_t)align8(tsize); // include alignment
		}
	}
	else if(isContainer())
	{
		const gddContainer* cdd=(const gddContainer*)this;
		gddCursor cur=cdd->getCursor();
		for(pdd=cur.first();pdd;pdd=cur.next())
			sz+=pdd->getTotalSizeBytes();
	}
	return sz;
}

aitUint32 gdd::getDataSizeElements(void) const
{
	unsigned long total=0;
	unsigned i;

	if(dimension()==0)
		total=1;
	else
	{
		if(voidData())
			for(i=0;i<dimension();i++) total+=bounds[i].size();
	}

	return total;
}

aitUint32 gdd::DescribedDataSizeElements(void) const
{
	unsigned long total=0;
	unsigned i;

	if(dimension()==0)
		total=1;
	else
		for(i=0;i<dimension();i++) total+=bounds[i].size();

	return total;
}

size_t gdd::flattenWithOffsets(void* buf, size_t size, aitIndex* total_dd)
{
	gdd* flat_dd;
	size_t sz;
	
	sz = flattenWithAddress(buf,size,total_dd);
	flat_dd=(gdd*)buf;
	if(sz>0) flat_dd->convertAddressToOffsets();
	return sz;
}

// IMPORTANT NOTE:
// Currently the user cannot register an empty container as a prototype.
// The destructor will not be installed to clean up the container when
// it is freed.

// This is an important function
// Data should be flattened as follows:
// 1) all the GDDs, seen as an array an GDDs
// 2) all the bounds info for all the GDDs
// 3) all the data for all GDDs
//
// In addition, the user should be able to flatten GDDs without the data
// and flatten portions or all the data without flattening GDDs

size_t gdd::flattenWithAddress(void* buf, size_t size, aitIndex* total_dd)
{
	gdd* pdd = (gdd*)buf;
	size_t pos,sz,spos;
	aitUint32 i;
	gddBounds* bnds;

	// copy this gdd (first one) to get things started
	// this is done in two passes - one to copy DDs, one for bounds/data
	// need to be able to check if the DD has been flattened already

	if((sz=getTotalSizeBytes())>size) return 0;
	pdd[0]=*this;
	pdd[0].destruct=NULL;
	pdd[0].flags=0;
	pos=1;

	// not enough to just copy the gdd info if the primitive type is
	// aitString or aitFixedString (even if scaler gdd)
	// must special case the strings - that really sucks

	if(isScaler())
	{
		// here is special case for the string types
		if(primitiveType()==aitEnumFixedString)
		{
			if(data.FString)
				memcpy((char*)&pdd[pos],data.FString,sizeof(aitFixedString));
			else
				pdd[0].data.FString=(aitFixedString*)&pdd[pos];
		}
		else if(primitiveType()==aitEnumString)
		{
			aitString* str=(aitString*)pdd[0].dataAddress();
			if(str->string())
			{
				memcpy((char*)&pdd[pos],str->string(),str->length()+1);
				*str=(char*)&pdd[pos];
			}
		}
	}
	else if(isContainer())
	{
		// need to check for bounds in the container and flatten them
		if(voidData())
		{
			// process all the container's DDs
			spos=pos;
			pos+=flattenDDs((gddContainer*)this,&pdd[pos],
				size-(pos*sizeof(gdd)));

			// copy all the data from the entire container into the buffer
			flattenData(&pdd[0],pos,&pdd[pos],size-(pos*sizeof(gdd)));

			pdd[0].markFlat();
			pdd[0].setData(&pdd[spos]);
		}
		else
			sz=0; // failure should occur - cannot flatten an empty container
	}
	else if(isAtomic())
	{
		// Just copy the data from this DD into the buffer, copy bounds
		if(bounds)
		{
			pdd[0].markFlat();
			bnds=(gddBounds*)(&pdd[pos]);
			for(i=0;i<dimension();i++) bnds[i]=bounds[i];
			pdd[0].bounds=bnds;
			if(voidData())
			{
				if(primitiveType()==aitEnumString)
				{
					// not very good way to do it, size info bad
					aitString::compact((aitString*)dataPointer(),
						getDataSizeElements(),&bnds[i],size);
				}
				else
					memcpy(&bnds[i],voidData(),getDataSizeBytes());

				pdd[0].setData(&bnds[i]);
			}
			else
				sz=0; // should return a failure
		}
		else
			sz=0; // should return failure
	}
	if(total_dd) *total_dd=pos;
	return sz;
}

gddStatus gdd::flattenData(gdd* dd, int tot_dds, void* buf,size_t size)
{
	int i;
	unsigned j;
	size_t sz;
	gddBounds* bnds;
	aitUint8* ptr = (aitUint8*)buf;
	unsigned long tmp;

	// This functions needs to be divided into two sections
	// 1) copy ALL the bounds out
	// 2) copy ALL the data out

	for(i=0;i<tot_dds;i++)
	{
		if(dd[i].isContainer())
		{
			// don't mark flat if container - 1D bounds must be present
			if(dd[i].bounds)
			{
				bnds=(gddBounds*)(ptr);
				for(j=0;j<dd[i].dimension();j++) bnds[j]=dd[i].bounds[j];
				dd[i].bounds=bnds;
				ptr+=j*sizeof(gddBounds);
			}
			else
			{
				// this is an error condition!
				dd[i].bounds=NULL;
			}
		}
		else if(dd[i].isAtomic())
		{
			if(dd[i].bounds)
			{
				// copy the bounds
				// need to mark flat if bounds are present in an atomic type
				dd[i].markFlat();
				bnds=(gddBounds*)(ptr);
				for(j=0;j<dd[i].dimension();j++) bnds[j]=dd[i].bounds[j];
				dd[i].bounds=bnds;
				ptr+=j*sizeof(gddBounds);

				// copy the data
				if(dd[i].voidData())
				{
					if(dd[i].primitiveType()==aitEnumString)
					{
						// not very good way to do it, size info bad
						sz=aitString::compact((aitString*)dd[i].dataPointer(),
							dd[i].getDataSizeElements(),ptr,size);
					}
					else
					{
						// need to copy data here, align to size of double
						sz=dd[i].getDataSizeBytes();
						memcpy(ptr,dd[i].voidData(),sz);
					}
					dd[i].setData(ptr);
					ptr+=align8(sz); // do alignment
				}
			}
			else
			{
				// bounds not required
				dd[i].bounds=NULL;
			}
		}
		else if(dd[i].isScaler())
		{
			// here is special case for String types
			if(dd[i].primitiveType()==aitEnumString)
			{
				aitString* str=(aitString*)dd[i].dataAddress();
				if(str->string())
				{
					memcpy(ptr,str->string(),str->length()+1);
					*str=(char*)ptr;
					ptr+=str->length()+1;
				}
			}
			else if(dd[i].primitiveType()==aitEnumFixedString)
			{
				if(dd[i].data.FString)
					memcpy(ptr,dd[i].data.FString,sizeof(aitFixedString));
				dd[i].data.FString=(aitFixedString*)ptr;
				ptr+=sizeof(aitFixedString);
			}
		}
	}
	return 0;
}

int gdd::flattenDDs(gddContainer* dd, void* buf, size_t size)
{
	gdd* ptr=(gdd*)buf;
	int i,tot,pos,spos;
	gdd *pdd;
	gddCursor cur;

	cur=dd->getCursor();

	// make first pass to copy all the container's DDs into the buffer
	for(tot=0,pdd=cur.first();pdd;pdd=pdd->next(),tot++)
	{
		ptr[tot]=*pdd;
		ptr[tot].destruct=NULL;
		ptr[tot].setNext(&ptr[tot+1]);
		ptr[tot].noReferencing();
	}
	ptr[tot-1].setNext(NULL);

	// make second pass to copy all child containers into buffer
	for(pos=tot,i=0;i<tot;i++)
	{
		if(ptr[i].isContainer())
		{
			if(ptr[i].voidData())
			{
				spos=pos;
				pos+=flattenDDs((gddContainer*)&ptr[i],&ptr[pos],
					size-(pos*sizeof(gdd)));
				ptr[i].markFlat();
				ptr[i].setData(&ptr[spos]);
			}
			else
			{
				ptr[i].setData(NULL);
				ptr[i].destruct=new gddContainerCleaner(&ptr[i]);
			}
		}
	}
	return pos;
}

gddStatus gdd::convertOffsetsToAddress(void)
{
	aitUint8* pdd = (aitUint8*)this;
	aitUint32 bnds = (aitUint32)(bounds);
	aitUint32 dp = (aitUint32)(voidData());
	gdd* tdd;
	gddContainer* cdd;
	gddCursor cur;
	aitString* str;
	aitIndex i;
	const char* cstr;

	if(isContainer())
	{
		// change bounds and data first
		bounds=(gddBounds*)(pdd+bnds);
		setData(pdd+dp);
		cdd=(gddContainer*)this;
		cur=cdd->getCursor();

		for(tdd=cur.first();tdd;tdd=cur.next())
		{
			if(tdd->next()) tdd->setNext((gdd*)(pdd+(aitUint32)tdd->next()));
			tdd->convertOffsetsToAddress();
		}
	}
	else
	{
		if(isAtomic())
		{
			bounds=(gddBounds*)(pdd+bnds);
			setData(pdd+dp);
			if(primitiveType()==aitEnumString)
			{
				// force all the strings in the array to offsets
				str=(aitString*)dataPointer();
				for(i=0;i<getDataSizeElements();i++)
				{
					if(str[i].string())
					{
						cstr=str[i].string();
						str[i].force(pdd+(unsigned long)cstr);
					}
				}
			}
		}
		else if(isScaler())
		{
			if(primitiveType()==aitEnumFixedString)
				if(data.FString) setData(pdd+dp);
			else if(primitiveType()==aitEnumString)
			{
				str=(aitString*)dataAddress();
				if(str->string())
				{
					cstr=str->string();
					str->force(pdd+(unsigned long)cstr);
				}
			}
		}
	}
	return 0;
}

gddStatus gdd::convertAddressToOffsets(void)
{
	aitUint8* pdd = (aitUint8*)this;
	aitUint8* bnds = (aitUint8*)(bounds);
	aitUint8* dp = (aitUint8*)(voidData());
	gddContainer* tdd;
	gddCursor cur;
	gdd *cdd,*ddd;
	aitString* str;
	aitIndex i;
	const char* cstr;

	// does not ensure that all the members of a container are flat!
	if(!isFlat())
		return gddErrorNotAllowed;

	if(isContainer())
	{
		tdd=(gddContainer*)this;
		cur=tdd->getCursor();

		for(cdd=cur.first();cdd;)
		{
			ddd=cdd;
			cdd=cur.next();
			ddd->convertAddressToOffsets();
			if(cdd) ddd->setNext((gdd*)((aitUint8*)(ddd->next())-pdd));
		}

		// bounds and data of container to offsets
		setData((gdd*)(dp-pdd));
		bounds=(gddBounds*)(bnds-pdd);
	}
	else
	{
		if(isAtomic())
		{
			if(primitiveType()==aitEnumString)
			{
				// force all the strings in the array to offsets
				str=(aitString*)dataPointer();
				for(i=0;i<getDataSizeElements();i++)
				{
					cstr=str[i].string();
					if(cstr) str[i].force(cstr-(const char*)pdd);
				}
			}
			// bounds and data of atomic to offsets
			setData((gdd*)(dp-pdd));
			bounds=(gddBounds*)(bnds-pdd);
		}
		else if(isScaler())
		{
			// handle the special string scaler cases
			if(primitiveType()==aitEnumFixedString)
				if(data.FString) setData((gdd*)(dp-pdd));
			else if(primitiveType()==aitEnumString)
			{
				str=(aitString*)dataAddress();
				cstr=str->string();
				if(cstr) str->force(cstr-(const char*)pdd);
			}
		}
	}
	return 0;
}

gddStatus gdd::clear(void)
{
	if(isFlat()||isManaged())
		return gddErrorNotAllowed;

	if(isAtomic())
	{
		if(destruct)
		{
			destruct->destroy(voidData());
			destruct=NULL;
		}
		freeBounds();
		changeType(0,aitEnumInvalid);
		setData(NULL);
	}
	else if(isContainer())
	{
		gddContainer* cdd = (gddContainer*)this;
		gddCursor cur = cdd->getCursor();
		gdd *dd,*tdd;

		for(dd=cur.first();dd;)
		{
			tdd=dd;
			dd=cur.next();
			if(tdd->unreference()<0) delete tdd;
		}
		setBound(0,0,0);
		setData(NULL);
	}

	return 0;
}

// Curently gives no indication of failure, which is bad.
// Obviously managed or flattened DDs cannot be redone.
// However, a DD that is within a managed or flattened container can
// use this to describe data - how's that for obscure
// This is required if the "value" is to be allowed within a container
// The "value" could be scaler or an array of unknown size.  The same is
// true of the enum strings and "units" attribute.

gddStatus gdd::reset(aitEnum prim, int dimen, aitIndex* cnt)
{
	int i,app;
	gddStatus rc;

	if(isFlat()||isManaged()||isContainer())
		return gddErrorNotAllowed;

	app=applicationType();

	if((rc=clear())<0)
		return rc;

	init(app,prim,dimen);

	for(i=0;i<dimen;i++)
		setBound(i,0,cnt[i]);

	return 0;
}

// copy each of the strings into this DDs storage area
gddStatus gdd::put(const aitString* const d)
{
	gddStatus rc=0;

	if(isAtomic())
	  aitConvert(primitiveType(),voidData(),aitEnumString,d,getDataSizeElements());
	else
		rc=gddErrorTypeMismatch;

	return rc;
}

// copy each of the strings into this DDs storage area
gddStatus gdd::put(const aitFixedString* const d)
{
	gddStatus rc=0;

	if(isAtomic())
		if(data.FString)
		{
			aitConvert(primitiveType(),voidData(),aitEnumFixedString,d,
				getDataSizeElements());
		}
		// should have else error condition
	else
		rc=gddErrorTypeMismatch;

	return rc;
}

gddStatus gdd::put(const gdd* dd)
{
	gddStatus rc=0;

	if(isContainer() || dd->isContainer())
		rc=gddErrorNotSupported;
	if(isScaler() && dd->isScaler())
	{
		// this is the simple case - just make this scaler look like the other
		// not protected against screwing up the string type
		setPrimType(dd->primitiveType());
		put(((aitType*)dd->dataAddress()));
	}
	else if(isAtomic() && dd->isAtomic())
	{
		// carefully place values from dd into this
		if(dd->getDataSizeElements()>getDataSizeElements())
			rc=gddErrorOutOfBounds;
		else
		{
			if(dd->dimension()>1)
				return gddErrorNotSupported;
			else
			{
				aitUint8* arr = (aitUint8*)dataPointer();
				aitConvert(
					primitiveType(),
					&arr[aitSize[primitiveType()]*dd->getBounds()->first()],
					dd->primitiveType(),
					dd->dataPointer(),
					dd->getBounds()->size());
			}
#if 0
			// should handle single dimensional array as a special case
			const Bounds* b = dd->getBounds();
			const Bounds* bs = getBounds();
			aitIndex x[dd->dimension()];
			aitIndex offset,mult;
			aitUint8* arr_dest=(aitUint8*)dataPointer();
			aitUint8* arr_src=(aitUint8*)dd->dataPointer();
			int d = dd->dimension();
			int i,j;

			for(i=0;i<d;i++) x[i]=b[i].first();

			// look at this ugly mess - slow and horrible, but generic
			// this is so unbelievably sucky, I hate it
			j=0;
			while(x[d-1]<b[d-1].size())
			{
				// modify coordinate here
				for(offset=x[0],mult=1,i=1;i<d;i++)
				{
					mult*=bs[i];
					offset+=x[i]*mult;
				}

				aitConvert(primitiveType(),
					arr_dest[offset*aitSize[dd->primitiveType()],
					dd->primitiveType(),arr_src[j],1);

				if(++x[0]>=b[0].size())
				{
					for(i=0;i<(d-1);i++)
					{
						if(x[i]>=b[i].size())
						{
							++x[i+1];
							x[i]=b[i].first();
						}
					}
				}
				j++;
			}
#endif
		}
	}
	else if(isScaler())
	{
		// just put first element of dd into this scaler - sucks
		if(dd->getDataSizeElements()>0)
			aitConvert(primitiveType(),dataAddress(),
				dd->primitiveType(),dd->dataPointer(),1);
	}
	else
	{
		// just put dd scaler into first element of this - sucks
		if(getDataSizeElements()>0)
			aitConvert(primitiveType(),dataPointer(),
				dd->primitiveType(),dd->dataAddress(),1);
	}

	return rc;
}

// ----------------------The gddAtomic functions-------------------------

gddAtomic::gddAtomic(int app, aitEnum prim, int dimen, ...):
		gdd(app,prim,dimen)
{
	va_list ap;
	int i;
	aitIndex val;

	va_start(ap,dimen);
	for(i=0;i<dimen;i++)
	{
		val=va_arg(ap,aitUint32);
		bounds[i].set(0,val);
	}
	va_end(ap);
}

gddStatus gddAtomic::getBoundingBoxSize(aitUint32* b)
{
	unsigned i;
	gddStatus rc=0;

	if(dimension()>0)
		for(i=0;i<dimension();i++) b[i]=bounds[i].size();
	else
		rc=gddErrorOutOfBounds;

	return rc;
}

gddStatus gddAtomic::setBoundingBoxSize(const aitUint32* const b)
{
	unsigned i;
	gddStatus rc=0;

	if(dimension()>0)
		for(i=0;i<dimension();i++) bounds[i].setSize(b[i]);
	else
		rc=gddErrorOutOfBounds;

	return rc;
}

gddStatus gddAtomic::getBoundingBoxOrigin(aitUint32* b)
{
	unsigned i;
	gddStatus rc=0;

	if(dimension()>0)
		for(i=0;i<dimension();i++) b[i]=bounds[i].first();
	else
		rc=gddErrorOutOfBounds;

	return rc;
}

gddStatus gddAtomic::setBoundingBoxOrigin(const aitUint32* const b)
{
	unsigned i;
	gddStatus rc=0;

	if(dimension()>0)
		for(i=0;i<dimension();i++) bounds[i].setSize(b[i]);
	else
		rc=gddErrorOutOfBounds;

	return rc;
}

// --------------------The gddScaler functions---------------------

// --------------------The gddContainer functions---------------------

gddCursor gddContainer::getCursor(void) const
{
	gddCursor ec(this);
	return ec;
}

gddContainer::gddContainer(void):gdd(0,aitEnumContainer,1) { }
gddContainer::gddContainer(int app):gdd(app,aitEnumContainer,1) { }

gddContainer::gddContainer(int app,int tot) : gdd(app,aitEnumContainer,1)
	{ cInit(tot); }

void gddContainer::cInit(int tot)
{
	int i;
	gdd* dd_list;
	gdd* temp;

	setBound(0,0,tot);
	dd_list=NULL;

	for(i=0;i<tot;i++)
	{
		temp=new gdd;
		temp->noReferencing();
		temp->setNext(dd_list);
		dd_list=temp;
	}
	setData(dd_list);
}

gddContainer::gddContainer(gddContainer* ec)
{
	unsigned i;
	gdd* dd_list;
	gdd* temp;

	copy(ec);
	dd_list=NULL;

	// this needs to recursively add to the container, only copy the
	// info and bounds information and scaler data, not arrays

	for(i=0;i<dimension();i++)
	{
		temp=new gdd(getDD(i));
		temp->noReferencing();
		temp->setNext(dd_list);
		dd_list=temp;
	}
	setData(dd_list);
}

gdd* gddContainer::getDD(aitIndex index)
{
	aitIndex i;
	gdd* dd=(gdd*)voidData();
	for(i=0;i<index && i<bounds->size();i++) dd=(gdd*)dd->next();
	return dd;
}

gddStatus gddContainer::insert(gdd* dd)
{
	dd->setNext(cData());
	setData(dd);
	bounds->setSize(bounds->size()+1);
	return 0;
}

gddStatus gddContainer::remove(aitIndex index)
{
	gddCursor cur = getCursor();
	gdd *dd,*prev_dd;
	aitIndex i;

	prev_dd=NULL;

	for(i=0; (dd=cur[i]) && i!=index; i++) prev_dd=dd;

	if(i==index && dd)
	{
		if(prev_dd)
			prev_dd->setNext(dd->next());
		else
			setData(dd->next());

		dd->unreference();
		bounds->setSize(bounds->size()-1);
		return 0;
	}
	else
		return gddErrorOutOfBounds;
}

// ------------------------cursor functions-------------------------------

gdd* gddCursor::operator[](int index)
{
	int i,start;
	gdd* dd;

	if(index>=curr_index)
	{
		start=curr_index;
		dd=curr;
	}
	else
	{
		start=0;
		dd=list->cData();
	}

	for(i=start;i<index;i++) dd=dd->next();
	curr_index=index;
	curr=dd;
	return dd;
}

