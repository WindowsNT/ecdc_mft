

class ECDCSink : public IMFMediaSink, public IMFStreamSink, public IMFMediaTypeHandler
{
public:

	ULONG ref = 1;
	CComPtr<IMFMediaType> cmt;
	std::wstring file;

	ECDCSink()
	{
		int nch = 2;
		int sr = 48000;
		int br = 16;

		CComPtr<IMFMediaType> ina;
		MFCreateMediaType(&ina);
		ina->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
		ina->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, nch);
		ina->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sr);
		int BA = (int)((br / 8) * nch);
		ina->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, (UINT32)(sr * BA));
		ina->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, BA);
		ina->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, br);
		ina->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		cmt = ina;
	}

	// IUnknown
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override
	{
		if (riid == __uuidof(IUnknown))
		{
			*ppvObject = (ECDCSink*)this;
			AddRef();
			return S_OK;
		}
		if (riid == __uuidof(IMFMediaSink))
		{
			*ppvObject = (IMFMediaSink*)this;
			AddRef();
			return S_OK;
		}
		if (riid == __uuidof(IMFStreamSink))
		{
			*ppvObject = (IMFStreamSink*)this;
			AddRef();
			return S_OK;
		}
		if (riid == __uuidof(IMFMediaEventGenerator))
		{
			*ppvObject = (IMFMediaEventGenerator*)this;
			AddRef();
			return S_OK;
		}
		if (riid == __uuidof(IMFMediaTypeHandler))
		{
			*ppvObject = (IMFMediaTypeHandler*)this;
			AddRef();
			return S_OK;
		}
		return E_NOTIMPL;
	}
	virtual ULONG __stdcall AddRef(void) override
	{
		InterlockedIncrement(&ref);
		return ref;
	}
	virtual ULONG __stdcall Release(void) override
	{
		if (ref == 1)
		{
			delete this;
			return 0;
		}
		InterlockedDecrement(&ref);
		return ref;
	}

	// MediaSink
	virtual HRESULT STDMETHODCALLTYPE GetCharacteristics(
		/* [out] */ __RPC__out DWORD* pdwCharacteristics)
	{
		if (!pdwCharacteristics) return E_POINTER;
		*pdwCharacteristics = MEDIASINK_RATELESS;
		return S_OK;
	}

	DWORD usid = 0;
	DWORD as = (DWORD)-1;
	virtual HRESULT STDMETHODCALLTYPE AddStreamSink(
		/* [in] */ DWORD dwStreamSinkIdentifier,
		/* [in] */ __RPC__in_opt IMFMediaType* pMediaType,
		/* [out] */ __RPC__deref_out_opt IMFStreamSink** ppStreamSink)
	{
		if (as == 0)
			return MF_E_STREAMSINK_EXISTS;
		if (!pMediaType || !ppStreamSink)
			return E_POINTER;
		BOOL r = 0;
		cmt->Compare(pMediaType, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &r);
		if (!r)
			return MF_E_INVALIDMEDIATYPE;
		usid = dwStreamSinkIdentifier;
		return QueryInterface(__uuidof(IMFStreamSink), (void**)ppStreamSink);
	}

	virtual HRESULT STDMETHODCALLTYPE RemoveStreamSink(
		/* [in] */ DWORD dwStreamSinkIdentifier)
	{
		if (dwStreamSinkIdentifier != 0)
			return MF_E_INVALIDSTREAMNUMBER;
		if (as != 0)
			return MF_E_NOT_INITIALIZED;
		as = (DWORD)-1;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetStreamSinkCount(
		/* [out] */ __RPC__out DWORD* pcStreamSinkCount)
	{
		if (!pcStreamSinkCount)
			return E_POINTER;
		*pcStreamSinkCount =  as == -1 ? 0 :1;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetStreamSinkByIndex(
		/* [in] */ DWORD dwIndex,
		/* [out] */ __RPC__deref_out_opt IMFStreamSink** ppStreamSink)
	{
		if (!ppStreamSink) return E_POINTER;
		if (dwIndex != 0)
			return MF_E_INVALIDSTREAMNUMBER;
		return QueryInterface(__uuidof(IMFStreamSink), (void**)ppStreamSink);
	}

	virtual HRESULT STDMETHODCALLTYPE GetStreamSinkById(
		/* [in] */ DWORD dwStreamSinkIdentifier,
		/* [out] */ __RPC__deref_out_opt IMFStreamSink** ppStreamSink)
	{
		if (dwStreamSinkIdentifier != usid)
			return E_FAIL;
		return GetStreamSinkByIndex(0, ppStreamSink);
	}

	CComPtr<IMFPresentationClock> clock;
	virtual HRESULT STDMETHODCALLTYPE SetPresentationClock(
		/* [in] */ __RPC__in_opt IMFPresentationClock* pPresentationClock)
	{
		clock = pPresentationClock;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPresentationClock(
		/* [out] */ __RPC__deref_out_opt IMFPresentationClock** ppPresentationClock)
	{
		if (!ppPresentationClock) return E_POINTER;
		*ppPresentationClock = clock;
		if (clock)
			(*ppPresentationClock)->AddRef();
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Shutdown(void)
	{
		return S_OK;
	}

	// StreamSink
	virtual HRESULT STDMETHODCALLTYPE GetEvent(
		/* [in] */ DWORD dwFlags,
		/* [out] */ __RPC__deref_out_opt IMFMediaEvent** ppEvent)
	{
		return S_OK;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE BeginGetEvent(
		/* [in] */ IMFAsyncCallback* pCallback,
		/* [in] */ IUnknown* punkState)
	{
		return S_OK;
	}


	virtual /* [local] */ HRESULT STDMETHODCALLTYPE EndGetEvent(
		/* [in] */ IMFAsyncResult* pResult,
		/* [annotation][out] */
		_Out_  IMFMediaEvent** ppEvent)
	{
		return S_OK;
	}


	virtual HRESULT STDMETHODCALLTYPE QueueEvent(
		/* [in] */ MediaEventType met,
		/* [in] */ __RPC__in REFGUID guidExtendedType,
		/* [in] */ HRESULT hrStatus,
		/* [unique][in] */ __RPC__in_opt const PROPVARIANT* pvValue)
	{
		return S_OK;
	}


	virtual HRESULT STDMETHODCALLTYPE GetMediaSink(
		/* [out] */ __RPC__deref_out_opt IMFMediaSink** ppMediaSink)
	{
		if (!ppMediaSink) return E_POINTER;
		return QueryInterface(__uuidof(IMFMediaSink), (void**)ppMediaSink);
	}
	virtual HRESULT STDMETHODCALLTYPE GetIdentifier(
		/* [out] */ __RPC__out DWORD* pdwIdentifier)
	{
		if (!pdwIdentifier) return E_POINTER;
		*pdwIdentifier = usid;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMediaTypeHandler(
		/* [out] */ __RPC__deref_out_opt IMFMediaTypeHandler** ppHandler)
	{
		if (!ppHandler) return E_POINTER;
		return QueryInterface(__uuidof(IMFMediaTypeHandler), (void**)ppHandler);
	}

	virtual HRESULT STDMETHODCALLTYPE ProcessSample(
		/* [in] */ __RPC__in_opt IMFSample* pSample)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE PlaceMarker(
		/* [in] */ MFSTREAMSINK_MARKER_TYPE eMarkerType,
		/* [in] */ __RPC__in const PROPVARIANT* pvarMarkerValue,
		/* [in] */ __RPC__in const PROPVARIANT* pvarContextValue)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE Flush(void)
	{
		return S_OK;
	}

	// IMFMediaTypeHandler

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE IsMediaTypeSupported(
		/* [in] */ IMFMediaType* pMediaType,
		/* [annotation][out] */
		_Outptr_opt_result_maybenull_  IMFMediaType** ppMediaType)
	{
		if (!pMediaType) return E_POINTER;

		if (ppMediaType)
			*ppMediaType = 0;
		BOOL r = 0;
		cmt->Compare(pMediaType,MF_ATTRIBUTES_MATCH_OUR_ITEMS,&r);
		if (r)
			return S_OK;

		CComPtr<IMFMediaType> c2;
		MFCreateMediaType(&c2);
		cmt->CopyAllItems(c2);
		if (ppMediaType)
		{
			*ppMediaType = c2.Detach();
		}
		return MF_E_INVALIDMEDIATYPE;

	}

	virtual HRESULT STDMETHODCALLTYPE GetMediaTypeCount(
		/* [out] */ __RPC__out DWORD* pdwTypeCount)
	{
		if (!pdwTypeCount) return E_POINTER;
		*pdwTypeCount = 1;
		return S_OK;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetMediaTypeByIndex(
		/* [in] */ DWORD dwIndex,
		/* [annotation][out] */
		_Outptr_  IMFMediaType** ppType)
	{
		if (dwIndex != 0)
			return E_FAIL;
		if (!ppType) return E_POINTER;

		*ppType = cmt;
		(*ppType)->AddRef();
		return S_OK;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE SetCurrentMediaType(
		/* [in] */ IMFMediaType* pMediaType)
	{
		if (!pMediaType) return E_POINTER;

		if (SUCCEEDED(IsMediaTypeSupported(pMediaType,0)))
		{ 
			cmt = pMediaType;
			return S_OK;
		}
		return MF_E_INVALIDMEDIATYPE;
	}

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetCurrentMediaType(
		/* [annotation][out] */
		_Outptr_  IMFMediaType** ppMediaType)
	{
		return GetMediaTypeByIndex(0, ppMediaType);
	}

	virtual HRESULT STDMETHODCALLTYPE GetMajorType(
		/* [out] */ __RPC__out GUID* pguidMajorType)
	{
		if (!pguidMajorType) return E_POINTER;
		*pguidMajorType = MFMediaType_Audio;
		return S_OK;
	}



		
};

HRESULT __stdcall CreateSink(const wchar_t* file,IMFMediaSink** s) 
{
	if (!s || !file)
		return E_POINTER;
	ECDCSink* e = new ECDCSink;
	e->file = file;
	*s = e;
	return S_OK;
}