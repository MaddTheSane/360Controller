//
//  360Driver.cpp
//  360 Driver
//
//  Created by C.W. Betts on 4/5/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

#include "360Driver.h"

Three60Driver::Three60Driver() : RefCount(1)
{
	
}

Three60Driver::~Three60Driver()
{
	
}

HRESULT Three60Driver::QueryInterface(REFIID iid, LPVOID *ppv)
{
	return 0;
}

ULONG Three60Driver::AddRef()
{
	return ++RefCount;
}

ULONG Three60Driver::Release()
{
	RefCount -= 1;
	if (RefCount == 0) {
		delete this;
		return 0;
	}
	return RefCount;
}

HRESULT Three60Driver::SQueryInterface(void *thisPointer, REFIID iid, LPVOID *ppv)
{
	return getThis(thisPointer)->QueryInterface(iid, ppv);
}

ULONG Three60Driver::SAddRef(void *thisPointer)
{
	return getThis(thisPointer)->AddRef();
}

ULONG Three60Driver::SRelease(void *thisPointer)
{
	return getThis(thisPointer)->Release();
}

IOCFPlugInInterface** Three60Driver::Alloc()
{
    Three60Driver *me = new Three60Driver;
    if (!me) {
        return NULL;
    }
    return (IOCFPlugInInterface **)(&me->iIOCFPlugInInterface.pseudoVTable);
}

extern void* Audio360Factory(CFAllocatorRef allocator, CFUUIDRef typeID)
{
    void* theAnswer = NULL;
    if(CFEqual(typeID, kAudioServerPlugInTypeUUID)) {
		theAnswer = Three60Driver::Alloc();
    }
    return theAnswer;
}
