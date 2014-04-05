//
//  360Driver.h
//  360 Driver
//
//  Created by C.W. Betts on 4/5/14.
//  Copyright (c) 2014 GitHub. All rights reserved.
//

#ifndef ___60_Driver___60Driver__
#define ___60_Driver___60Driver__

#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFPlugInCOM.h>
#include <IOKit/IOCFPlugIn.h>

class Three60Driver : IUnknown {
	void *_reserved;
	ULONG RefCount;
	
	typedef struct _Audio360InterfaceMap
    {
        IUnknownVTbl *pseudoVTable;
        Three60Driver *obj;
    } Audio360InterfaceMap;

	Audio360InterfaceMap iIOCFPlugInInterface;
	static inline Three60Driver *getThis (void *oSelf) { return (Three60Driver *) ((Audio360InterfaceMap *) oSelf)->obj; }

public:
	
	Three60Driver();
	virtual ~Three60Driver();
	
	static IOCFPlugInInterface** Alloc();
	
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
	
    static HRESULT SQueryInterface(void *thisPointer, REFIID iid, LPVOID *ppv);
    static ULONG SAddRef(void *thisPointer);
    static ULONG SRelease(void *thisPointer);
};

#endif /* defined(___60_Driver___60Driver__) */
