// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

// code is based on the https://docs.microsoft.com/en-us/samples/microsoft/windows-classic-samples/ifilter-sample/
// the original license may be seen at https://github.com/microsoft/Windows-classic-samples/blob/main/LICENSE

#include <new>
#include <Windows.h>
#include <strsafe.h>
#include <tchar.h>
#include "FilterBase.h"

#ifndef UNICODE
#error Unicode environment required. Some day, I will fix, if anyone needs it.
#endif

void DllAddRef();
void DllRelease();

// Filter for ".filtersample" files
class CFilterSample : public CFilterBase
{
public:
	CFilterSample() : m_cRef(1), m_iEmitState(EMITSTATE_FLAGSTATUS)
	{
		DllAddRef();
	}

	~CFilterSample()
	{
		DllRelease();
	}

	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] =
		{
			QITABENT(CFilterSample, IInitializeWithStream),
			QITABENT(CFilterSample, IFilter),
			{0, 0},
		};
		return QISearch(this, qit, riid, ppv);
	}

	IFACEMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_cRef);
	}

	IFACEMETHODIMP_(ULONG) Release()
	{
		long cRef = InterlockedDecrement(&m_cRef);
		if (cRef == 0)
		{
			delete this;
		}
		return cRef;
	}

	virtual HRESULT OnInit();
	virtual HRESULT GetNextChunkValue(CChunkValue& chunkValue);

private:
	long m_cRef;

	// some props we want to emit don't come from the doc.  We use this as our state
	enum EMITSTATE { EMITSTATE_FLAGSTATUS = 0, EMITSTATE_ISREAD };

	DWORD m_iEmitState;
};


HRESULT CFilterSample_CreateInstance(REFIID riid, void** ppv)
{
	LOGFUNCTION;
	HRESULT hr = E_OUTOFMEMORY;
	CFilterSample* pFilter = new(std::nothrow) CFilterSample();
	if (pFilter)
	{
		hr = pFilter->QueryInterface(riid, ppv);
		pFilter->Release();
	}
	return hr;
}


// This is called after the stream (m_pStream) has been setup and is ready for use
HRESULT CFilterSample::OnInit()
{
	LOGFUNCTION;
	PCHAR streamContent = nullptr;
	streamContent = static_cast<PCHAR>(LocalAlloc(LPTR, 1024));
	if (nullptr != streamContent)
	{
		m_pStream->Read(streamContent, 1024, nullptr);
	}
	TCHAR strMsg[1024] = {0};
	_stprintf_s(strMsg, _countof(strMsg), TEXT("[GTIFilter] file content = %hs"), streamContent);
	OutputDebugString(strMsg);
	return S_OK;
}


// When GetNextChunkValue() is called we fill in the ChunkValue by calling SetXXXValue() with the property and value (and other parameters that you want)
// example:  chunkValue.SetTextValue(PKEY_ItemName, L"example text");
// return FILTER_E_END_OF_CHUNKS when there are no more chunks
HRESULT CFilterSample::GetNextChunkValue(CChunkValue& chunkValue)
{
	chunkValue.Clear();
	switch (m_iEmitState)
	{
	case EMITSTATE_FLAGSTATUS:
		// we are using this just to illustrate a numeric property
		chunkValue.SetIntValue(PKEY_FlagStatus, 1);
		m_iEmitState++;
		return S_OK;

	case EMITSTATE_ISREAD:
		// we are using this just to illustrate a bool property
		chunkValue.SetIntValue(PKEY_IsRead, true);
		m_iEmitState++;
		return S_OK;
	}

	// if we get to here we are done with this document
	return FILTER_E_END_OF_CHUNKS;
}
