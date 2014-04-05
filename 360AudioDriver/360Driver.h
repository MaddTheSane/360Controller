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

class Three60Driver : IUnknown {
	ULONG RefCount;
	
public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv);
    virtual ULONG STDMETHODCALLTYPE AddRef();
    virtual ULONG STDMETHODCALLTYPE Release();
};

#endif /* defined(___60_Driver___60Driver__) */
