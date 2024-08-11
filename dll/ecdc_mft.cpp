#include <Windows.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfobjects.h>
#include <codecapi.h>
#include <mfreadwrite.h>
#include <atlbase.h>
#include <strmif.h>
#include <vector>
#include <thread>
#include <memory>
#include <set>
#include <string>
#include <Mferror.h>
#include <shlobj.h>
#include <map>
#include "rkey.h"
#include "..\\log.hpp"
#pragma comment(lib,"Mfplat.lib")
#pragma comment(lib,"mfuuid.lib")

#include "..\\common.h"
#include "guids.hpp"

#include "sink.hpp"

HINSTANCE hDLL = 0;




HKEY kr = HKEY_LOCAL_MACHINE;
BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID)
{
	if (ulReason == DLL_PROCESS_ATTACH)
	{
		hDLL = hInstance;
	}

	if (ulReason == DLL_PROCESS_DETACH)
	{

	}
	return TRUE;
}


std::wstring InstallECDC()
{
	// Get AppData
	PWSTR p = 0;
	SHGetKnownFolderPath(FOLDERID_ProgramData, 0, 0, &p);
	if (!p)
		return {};
	std::wstring Where = p;
	Where += L"\\EncodecMFT{BE60AF19-1EFD-4130-8D0F-43EBAC9D6C8F}";
	CoTaskMemFree(p);

#ifdef _DEBUG
//	Where = L"f:\\TP2Modules\\Turbo Play encodec";
#endif

	// Check there
	std::wstring fi = Where + L"\\scripts\\encodec.exe";
	if (GetFileAttributes(fi.c_str()) != 0xFFFFFFFF)
		return Where;

	return {};
}



class EcdcTransform : public IMFTransform, public ICodecAPI
{
	ULONG ref = 1;
	CComPtr<IMFAttributes> attrs;
	CComPtr<IMFMediaType> pcm;
	CComPtr<IMFMediaType> ecdc;
	int mode = 0; // 1 decoding, 0 encoding
	int br = 16;
	int nch = 2;
	int sr = 48000;
	bool RelaxedIn = true;


	typedef struct WAV_HEADER {
		/* RIFF Chunk Descriptor */
		uint8_t RIFF[4] = { 'R', 'I', 'F', 'F' }; // RIFF Header Magic header
		uint32_t ChunkSize;                     // RIFF Chunk Size
		uint8_t WAVE[4] = { 'W', 'A', 'V', 'E' }; // WAVE Header
		/* "fmt" sub-chunk */
		uint8_t fmt[4] = { 'f', 'm', 't', ' ' }; // FMT header
		uint32_t Subchunk1Size = 16;           // Size of the fmt chunk
		uint16_t AudioFormat = 1; // Audio format 1=PCM,6=mulaw,7=alaw,     257=IBM
		// Mu-Law, 258=IBM A-Law, 259=ADPCM
		uint16_t NumOfChan = 2;   // Number of channels 1=Mono 2=Sterio
		uint32_t SamplesPerSec = 48000;   // Sampling Frequency in Hz
		uint32_t bytesPerSec = 192000; // bytes per second
		uint16_t blockAlign = 4;          // 2=16-bit mono, 4=16-bit stereo
		uint16_t bitsPerSample = 16;      // Number of bits per sample
		/* "data" sub-chunk */
		uint8_t Subchunk2ID[4] = { 'd', 'a', 't', 'a' }; // "data"  string
		uint32_t Subchunk2Size;                        // Sampled data length
	} wav_hdr;


public:

	EcdcTransform(int m)
	{
		mode = m;
		ref = 1;
		MFCreateAttributes(&attrs,0);

		folder = InstallECDC();

		CComPtr<IMFMediaType> ina;
		MFCreateMediaType(&ina);
		ina->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		ina->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);

		if (RelaxedIn == false || mode == 1)
		{
			ina->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, nch);
			ina->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sr);
			int BA = (int)((br / 8) * nch);
			ina->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, (UINT32)(sr * BA));
			ina->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, BA);
			ina->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, br);
		}
		pcm = ina;

		MFCreateMediaType(&ecdc);
		ecdc->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		ecdc->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_ECDC);
		ecdc->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
		ecdc->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 48000);
	}

	// Inherited via IMFTransform
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override
	{
		if (riid == IID_EcdcTransform1)
		{
			*ppvObject = (EcdcTransform*)this;
			AddRef();
			return S_OK;
		}
		if (riid == IID_EcdcTransform2)
		{
			*ppvObject = (EcdcTransform*)this;
			AddRef();
			return S_OK;
		}
		if (riid == __uuidof(IUnknown))
		{
			*ppvObject = (IMFTransform*)this;
			AddRef();
			return S_OK;
		}
		if (riid == __uuidof(IMFTransform))
		{
			*ppvObject = (IMFTransform*)this;
			AddRef();
			return S_OK;
		}
		if (riid == __uuidof(ICodecAPI))
		{
			*ppvObject = (ICodecAPI*)this;
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
	HRESULT __stdcall GetStreamLimits(DWORD* pdwInputMinimum, DWORD* pdwInputMaximum, DWORD* pdwOutputMinimum, DWORD* pdwOutputMaximum) override
	{
		if ((pdwInputMinimum == NULL) ||
			(pdwInputMaximum == NULL) ||
			(pdwOutputMinimum == NULL) ||
			(pdwOutputMaximum == NULL))
		{
			return E_POINTER;
		}
		*pdwInputMinimum = 1;
		*pdwInputMaximum = 1;
		*pdwOutputMinimum = 1;
		*pdwOutputMaximum = 1;
		return S_OK;
	}
	HRESULT __stdcall GetStreamCount(DWORD* pcInputStreams, DWORD* pcOutputStreams) override
	{
		if ((pcInputStreams == NULL) || (pcOutputStreams == NULL))
		{
			return E_POINTER;
		}
		*pcInputStreams = 1;
		*pcOutputStreams = 1;
		return S_OK;
	}
	HRESULT __stdcall GetStreamIDs(DWORD dwInputIDArraySize, DWORD* pdwInputIDs, DWORD dwOutputIDArraySize, DWORD* pdwOutputIDs) override
	{
		return E_NOTIMPL;
	}
	HRESULT __stdcall GetInputStreamInfo(DWORD dwInputStreamID, MFT_INPUT_STREAM_INFO* pStreamInfo) override
	{
		if (dwInputStreamID != 0 || pStreamInfo == 0)
			return E_FAIL;
		*pStreamInfo = {};
		UINT32 wi2 = 0;
		wi2 = MFGetAttributeUINT32(pcm, MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 0);
		pStreamInfo->cbSize = wi2;
		pStreamInfo->dwFlags = MFT_INPUT_STREAM_WHOLE_SAMPLES | MFT_INPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_INPUT_STREAM_FIXED_SAMPLE_SIZE | MFT_INPUT_STREAM_DOES_NOT_ADDREF;
		return S_OK;
	}
	HRESULT __stdcall GetOutputStreamInfo(DWORD dwOutputStreamID, MFT_OUTPUT_STREAM_INFO* pStreamInfo) override
	{
		if (dwOutputStreamID != 0 || pStreamInfo == 0)
			return E_FAIL;
		*pStreamInfo = {};
		pStreamInfo->dwFlags = MFT_OUTPUT_STREAM_WHOLE_SAMPLES | MFT_OUTPUT_STREAM_SINGLE_SAMPLE_PER_BUFFER | MFT_OUTPUT_STREAM_PROVIDES_SAMPLES;
		return S_OK;

	}
	HRESULT __stdcall GetAttributes(IMFAttributes** pAttributes) override
	{
		if (!pAttributes)
			return E_FAIL;
		*pAttributes = attrs;
		(*pAttributes)->AddRef();
		return S_OK;
	}
	HRESULT __stdcall GetInputStreamAttributes(DWORD dwInputStreamID, IMFAttributes** pAttributes) override
	{
		if (!pAttributes)
			return E_FAIL;
		*pAttributes = attrs;
		(*pAttributes)->AddRef();
		return S_OK;
	}
	HRESULT __stdcall GetOutputStreamAttributes(DWORD dwOutputStreamID, IMFAttributes** pAttributes) override
	{
		if (!pAttributes)
			return E_FAIL;
		*pAttributes = attrs;
		(*pAttributes)->AddRef();
		return S_OK;
	}
	HRESULT __stdcall DeleteInputStream(DWORD dwStreamID) override
	{
		return E_NOTIMPL;
	}
	HRESULT __stdcall AddInputStreams(DWORD cStreams, DWORD* adwStreamIDs) override
	{
		return E_NOTIMPL;
	}
	HRESULT __stdcall GetInputAvailableType(DWORD dwInputStreamID, DWORD dwTypeIndex, IMFMediaType** ppType) override
	{
		if (dwInputStreamID != 0)
			return MF_E_INVALIDSTREAMNUMBER;
		if (dwTypeIndex > 0)
			return MF_E_NO_MORE_TYPES;
		CComPtr<IMFMediaType> mt;
		MFCreateMediaType(&mt);
		if (mode == 0)
			pcm->CopyAllItems(mt);
		if (mode == 1)
			ecdc->CopyAllItems(mt);
		*ppType = mt;
		mt.Detach();
		return S_OK;
	}
	HRESULT __stdcall GetOutputAvailableType(DWORD dwOutputStreamID, DWORD dwTypeIndex, IMFMediaType** ppType) override
	{
		if (dwOutputStreamID != 0)
			return MF_E_INVALIDSTREAMNUMBER;
		if (dwTypeIndex > 0)
			return MF_E_NO_MORE_TYPES;
		CComPtr<IMFMediaType> mt;
		MFCreateMediaType(&mt);
		if (mode == 1)
			pcm->CopyAllItems(mt);
		if (mode == 0)
			ecdc->CopyAllItems(mt);
		LogMediaType(mt);
		*ppType = mt;
		mt.Detach();
		return S_OK;
	}
	HRESULT __stdcall SetInputType(DWORD dwInputStreamID, IMFMediaType* pType, DWORD dwFlags) override
	{
		if (!pType)
			return MF_E_INVALIDTYPE;
		BOOL r1 = 0;
		BOOL r2 = 0;
		LogMediaType(pType);
		LogMediaType(pcm);
		LogMediaType(ecdc);
		if (mode == 1)
		{
			// Input must be ECDC
			ecdc->Compare(pType, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &r2);
		}
		else
		{
			// Input must be PCM
			pcm->Compare(pType, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &r1);
		}
		if (!r1 && !r2)
			return E_FAIL;

		if (mode == 0 && RelaxedIn)
		{
			UINT32 v = 0;
			pType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &v);
			if (v)
				sr = v;
			pType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &v);
			if (v)
				nch = v;
			pType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &v);
			if (v)
				br = v;
		}

		return S_OK;
	}

	double bw = 0.;
	int vis = 1;
	HRESULT __stdcall SetOutputType(DWORD dwOutputStreamID, IMFMediaType* pType, DWORD dwFlags) override
	{
		if (!pType)
			return MF_E_INVALIDTYPE;
		BOOL r1 = 0;
		BOOL r2 = 0;
		pcm->Compare(pType, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &r1);
		ecdc->Compare(pType, MF_ATTRIBUTES_MATCH_OUR_ITEMS, &r2);
		if (!r1 && !r2)
			return E_FAIL;
		return S_OK;
	}
	HRESULT __stdcall GetInputCurrentType(DWORD dwInputStreamID, IMFMediaType** ppType) override
	{
		return GetInputAvailableType(dwInputStreamID, 0, ppType);
	}
	HRESULT __stdcall GetOutputCurrentType(DWORD dwOutputStreamID, IMFMediaType** ppType) override
	{
		return GetOutputAvailableType(dwOutputStreamID, 0, ppType);
	}
	HRESULT __stdcall GetInputStatus(DWORD dwInputStreamID, DWORD* pdwFlags) override
	{
		return E_NOTIMPL;
	}
	HRESULT __stdcall GetOutputStatus(DWORD* pdwFlags) override
	{
		return E_NOTIMPL;
	}
	HRESULT __stdcall SetOutputBounds(LONGLONG hnsLowerBound, LONGLONG hnsUpperBound) override
	{
		return E_NOTIMPL;
	}

	HRESULT __stdcall ProcessEvent(DWORD dwInputStreamID, IMFMediaEvent* pEvent) override
	{
		return E_NOTIMPL;
	}
	std::wstring folder;
	std::wstring TempInPCM;

	HRESULT __stdcall ProcessMessage(MFT_MESSAGE_TYPE eMessage, ULONG_PTR ulParam) override
	{
		if (eMessage == MFT_MESSAGE_NOTIFY_BEGIN_STREAMING)
		{
			return S_OK;
		}
		if (eMessage == MFT_MESSAGE_NOTIFY_START_OF_STREAM)
		{
			// Init library
			return S_OK;
		}
		if (eMessage == MFT_MESSAGE_NOTIFY_END_STREAMING)
		{
			return S_OK;
		}
		if (eMessage == MFT_MESSAGE_COMMAND_DRAIN)
		{
			// fix wav
			if (mode == 0)
			{
				auto hX = CreateFile(TempInPCM.c_str(), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
				unsigned long long fsize = 0;
				wav_hdr wav;
				if (hX != INVALID_HANDLE_VALUE)
				{
					LARGE_INTEGER fx;
					GetFileSizeEx(hX, &fx);
					fsize = fx.QuadPart;


					if (RelaxedIn)
					{
						wav.NumOfChan = nch;
						wav.bitsPerSample = br;
						wav.blockAlign = wav.bitsPerSample / 8 * wav.NumOfChan;
						wav.SamplesPerSec = sr;
						wav.bytesPerSec = wav.SamplesPerSec * wav.blockAlign;
					}
					wav.ChunkSize = (uint32_t)(fsize + sizeof(wav_hdr) - 8);
					wav.Subchunk2Size = (uint32_t)(fsize + sizeof(wav_hdr) - 44);
					DWORD A = 0;
					WriteFile(hX, &wav, sizeof(wav), &A, 0);
					CloseHandle(hX);
				}

				// Run ECDC
				// encodec.exe -f -r input output
				std::vector<wchar_t> ru(10000);
				PushPopDir ppd(folder.c_str());



				std::wstring out = TempFile();
				std::vector<wchar_t> x(1000);
				out += L".ecdc";
				swprintf_s(ru.data(), 10000, L"\"%s\\scripts\\encodec.exe\" -q -r -f \"%s\" \"%s\"", folder.c_str(), TempInPCM.c_str(), out.c_str());
				if (bw >= 3.0)
					swprintf_s(ru.data(), 10000, L"\"%s\\scripts\\encodec.exe\" -b %.1f -q -r -f \"%s\" \"%s\"", folder.c_str(), (float)bw, TempInPCM.c_str(), out.c_str());

				swprintf_s(x.data(), 1000, L"Encoding ECDC audio [SR = %i,NCH = %i,BR = %i,BW = %.1f]...",sr,nch,br,(float)bw);


				Run(hDLL, ru.data(), true, CREATE_NO_WINDOW, vis ? x.data() : 0);

				std::vector<char> u;
				LoadFile(out.c_str(), u);
				if (u.size())
				{
					S = 0;
					MFCreateSample(&S);

					CComPtr<IMFMediaBuffer> b;
					MFCreateMemoryBuffer((DWORD)u.size(), &b);
					b->SetCurrentLength((DWORD)u.size());
					BYTE* pb = 0;
					DWORD ml = 0, cl = 0;

					b->Lock(&pb, &ml, &cl);
					memcpy(pb, u.data(), u.size());
					b->Unlock();
					S->AddBuffer(b);

					S->SetSampleTime(0);
					auto ts = wav.Subchunk2Size;
					ts /= nch; // nCh
					ts /= br/8; 
					S->SetSampleDuration(a2vi(ts, sr));

				}
			}
			return S_OK;
		}
		return S_OK;
	}
	CComPtr<IMFSample> S = 0;

	std::map<DWORD, std::wstring> CacheDecodings;
	bool UseCaches = true;

	HRESULT __stdcall ProcessInput(DWORD dwInputStreamID, IMFSample* pSample, DWORD dwFlags) override
	{
		if (dwInputStreamID != 0)
			return E_FAIL;
		if (S)
			return MF_E_NOTACCEPTING;
		if (!pSample)
			return E_FAIL;

		CComPtr<IMFMediaBuffer> b;
		pSample->ConvertToContiguousBuffer(&b);
		if (!b)
			return E_FAIL;

		DWORD ml = 0, cl = 0;
		BYTE* bb = 0;
		b->Lock(&bb, &ml, &cl);
		if (!bb)
			return E_FAIL;

		if (mode == 1)
		{
			std::wstring TempInE = TempFile();
			TempInE += L".ecdc";
			std::wstring TempInW = TempFile();
			TempInW += L".wav";

			std::vector<char> dx(cl);
			memcpy(dx.data(), bb, cl);

			// Check cache
			DWORD c = crc32(0, (unsigned char*)dx.data(), dx.size());
			std::vector<char> u;
			if (UseCaches)
			{
				if (CacheDecodings[c].length())
					LoadFile(CacheDecodings[c].c_str(), u);
			}

			if (u.size())
			{

			}
			else
			{
				PutFile(TempInE.c_str(), dx);
				if (UseCaches)
					CacheDecodings[c] = TempInW;

				std::vector<wchar_t>ru(10000);
				PushPopDir ppd(folder.c_str());
				swprintf_s(ru.data(), 10000, L"\"%s\\scripts\\encodec.exe\" -q -r -f \"%s\" \"%s\"", folder.c_str(), TempInE.c_str(), TempInW.c_str());
				Run(hDLL, ru.data(), true, CREATE_NO_WINDOW, vis ? L"Decoding ECDC audio..." : 0);
				LoadFile(TempInW.c_str(), u);
			}

			wav_hdr* w = (wav_hdr*)u.data();

			if (u.size())
			{
				S = 0;
				MFCreateSample(&S);

				auto us = u.size() - sizeof(wav_hdr);
				CComPtr<IMFMediaBuffer> b;
				MFCreateMemoryBuffer((DWORD)us, &b);
				b->SetCurrentLength((DWORD)us);
				BYTE* pb = 0;
				DWORD ml = 0, cl = 0;

				b->Lock(&pb, &ml, &cl);
				memcpy(pb, u.data() + sizeof(wav_hdr), us);
				b->Unlock();
				S->AddBuffer(b);

				S->SetSampleTime(0);
				auto ts = w->Subchunk2Size;
				ts /= nch; // nCh
				ts /= br/8; 

				// ts = samples
				S->SetSampleDuration(a2vi(ts, sr));

			}
			b->Unlock();
			return S_OK;
		}

		if (TempInPCM.empty())
		{
			TempInPCM = TempFile();
			TempInPCM += L".wav";
			auto hX = CreateFile(TempInPCM.c_str(), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
			if (hX != INVALID_HANDLE_VALUE)
			{
				wav_hdr wav = {};
				DWORD A = 0;
				WriteFile(hX, &wav, sizeof(wav), &A, 0);
				CloseHandle(hX);
			}
		}
		auto hX = CreateFile(TempInPCM.c_str(), GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);
		if (hX != INVALID_HANDLE_VALUE)
		{
			SetFilePointer(hX, 0, 0, FILE_END);
			DWORD A = 0;
			WriteFile(hX, bb, cl, &A, 0);
			CloseHandle(hX);
		}

		b->Unlock();
		return S_OK;

	}
	HRESULT __stdcall ProcessOutput(DWORD dwFlags, DWORD cOutputBufferCount, MFT_OUTPUT_DATA_BUFFER* pOutputSamples, DWORD* pdwStatus) override
	{
		if (!S)
			return MF_E_TRANSFORM_NEED_MORE_INPUT;
		if (cOutputBufferCount != 1)
			return E_FAIL;
		if (!pOutputSamples || !pdwStatus)
			return E_FAIL;
		*pdwStatus = 0;

		pOutputSamples->dwStreamID = 0;
		pOutputSamples->dwStatus = 0;
		pOutputSamples->pSample = S.Detach();
		return S_OK;
	}

	// Inherited via ICodecAPI
	virtual HRESULT __stdcall IsSupported(const GUID* Api) override
	{
		if (!Api)
			return E_POINTER;
		if (*Api == MFEHDC_BANDWIDTH)
			return S_OK;
		if (*Api == MFEHDC_VISIBLE)
			return S_OK;
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall IsModifiable(const GUID* Api) override
	{
		if (!Api)
			return E_POINTER;
		if (*Api == MFEHDC_BANDWIDTH)
			return S_OK;
		if (*Api == MFEHDC_VISIBLE)
			return S_OK;
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall GetParameterRange(const GUID* Api, VARIANT* ValueMin, VARIANT* ValueMax, VARIANT* SteppingDelta) override
	{
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall GetParameterValues(const GUID* Api, VARIANT** Values, ULONG* ValuesCount) override
	{
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall GetDefaultValue(const GUID* Api, VARIANT* Value) override
	{
		if (!Api)
			return E_POINTER;
		if (*Api == MFEHDC_BANDWIDTH)
		{
			Value->vt = VT_R8;
			Value->dblVal = 0;
			return S_OK;
		}
		if (*Api == MFEHDC_VISIBLE)
		{
			Value->vt = VT_I4;
			Value->intVal = 1;
			return S_OK;
		}
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall GetValue(const GUID* Api, VARIANT* Value) override
	{
		if (!Api)
			return E_POINTER;
		if (*Api == MFEHDC_BANDWIDTH)
		{
			Value->vt = VT_R8;
			Value->dblVal = bw;
			return S_OK;
		}
		if (*Api == MFEHDC_VISIBLE)
		{
			Value->vt = VT_I4;
			Value->intVal = vis;
			return S_OK;
		}
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall SetValue(const GUID* Api, VARIANT* Value) override
	{
		if (!Api || !Value)
			return E_POINTER;
		if (*Api == MFEHDC_BANDWIDTH && Value->vt == VT_R8)
		{
			bw = Value->dblVal;
			return S_OK;
		}
		if (*Api == MFEHDC_VISIBLE && Value->vt == VT_I4)
		{
			vis = Value->intVal;
			return S_OK;
		}
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall RegisterForEvent(const GUID* Api, LONG_PTR userData) override
	{
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall UnregisterForEvent(const GUID* Api) override
	{
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall SetAllDefaults(void) override
	{
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall SetValueWithNotify(const GUID* Api, VARIANT* Value, GUID** ChangedParam, ULONG* ChangedParamCount) override
	{
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall SetAllDefaultsWithNotify(GUID** ChangedParam, ULONG* ChangedParamCount) override
	{
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall GetAllSettings(IStream* __MIDL__ICodecAPI0000) override
	{
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall SetAllSettings(IStream* __MIDL__ICodecAPI0001) override
	{
		return E_NOTIMPL;
	}
	virtual HRESULT __stdcall SetAllSettingsWithNotify(IStream* __MIDL__ICodecAPI0002, GUID** ChangedParam, ULONG* ChangedParamCount) override
	{
		return E_NOTIMPL;
	}

};


// DllRegisterServer: Creates the registry entries.
extern "C"  HRESULT __stdcall DllRegisterServer()
{
	HRESULT hr = S_OK;

	// Array of media types.
	MFT_REGISTER_TYPE_INFO aMediaTypesP[] = { {   MFMediaType_Audio, MFAudioFormat_PCM }, };
	MFT_REGISTER_TYPE_INFO aMediaTypesE[] = { {   MFMediaType_Audio, MFAudioFormat_ECDC }, };

	// Size of the array.
	const DWORD cNumMediaTypesP = 1;
	DWORD cNumMediaTypesE = 1;

	hr = MFTRegister(
		IID_EcdcTransform1,     // CLSID.
		MFT_CATEGORY_AUDIO_ENCODER,  // Category.
		(LPWSTR)L"ECDC Encoder",         // Friendly name.
		0,                          // Reserved, must be zero.
		cNumMediaTypesP,             // Number of input types.
		aMediaTypesP,                // Input types.
		cNumMediaTypesE,             // Number of output types.
		aMediaTypesE,                // Output types.
		NULL                        // Attributes (optional).
	);
	
	if (SUCCEEDED(hr))
	{
		hr = MFTRegister(
			IID_EcdcTransform2,     // CLSID.
			MFT_CATEGORY_AUDIO_DECODER,  // Category.
			(LPWSTR)L"ECDC Decoder",         // Friendly name.
			0,                          // Reserved, must be zero.
			cNumMediaTypesE,             // Number of input types.
			aMediaTypesE,                // Input types.
			cNumMediaTypesP,             // Number of output types.
			aMediaTypesP,                // Output types.
			NULL                        // Attributes (optional).
		);

	}

	std::wstring TheFolder = InstallECDC();

	if (SUCCEEDED(hr))
	{
		std::vector<wchar_t> T(1000);
		GetModuleFileName(hDLL, T.data(), 1000);
		RKEY k(kr, L"Software\\Classes\\CLSID\\{24E223B6-57F3-4CA3-AB9F-A1F669937346}", KEY_ALL_ACCESS);
		k = L"ECDC Encoder";
		RKEY k3(kr, L"Software\\Classes\\CLSID\\{24E223B6-57F3-4CA3-AB9F-A1F669937346}\\InProcServer32", KEY_ALL_ACCESS);
		k3 = T.data();
		k3[L"ThreadingModel"] = L"Both";
		k[L"EncodecFolder"] = TheFolder.c_str();

	}
	
	if (SUCCEEDED(hr))
	{
		std::vector<wchar_t> T(1000);
		GetModuleFileName(hDLL, T.data(), 1000);
		RKEY k(kr, L"Software\\Classes\\CLSID\\{24E223B6-57F3-4CA3-AB9F-A1F669937347}", KEY_ALL_ACCESS);
		k = L"ECDC Decoder";
		RKEY k3(kr, L"Software\\Classes\\CLSID\\{24E223B6-57F3-4CA3-AB9F-A1F669937347}\\InProcServer32", KEY_ALL_ACCESS);
		k3 = T.data();
		k3[L"ThreadingModel"] = L"Both";
		k[L"EncodecFolder"] = TheFolder.c_str();
	}


	return hr;
}

// DllUnregisterServer: Removes the registry entries.
STDAPI DllUnregisterServer()
{
	RKEY k(kr, L"Software\\Classes\\CLSID", KEY_ALL_ACCESS);
	k.Delete(L"{24E223B6-57F3-4CA3-AB9F-A1F669937346}");
	k.Delete(L"{24E223B6-57F3-4CA3-AB9F-A1F669937347}");
	HRESULT hr = MFTUnregister(IID_EcdcTransform2);
	hr = MFTUnregister(IID_EcdcTransform1);
	return hr;
}


class MyCF : public IClassFactory
{
private:
	ULONG r;

public:

	MyCF()
	{
		r = 1;
	}

	// IUnknown
	virtual ULONG __stdcall AddRef()
	{
		InterlockedIncrement(&r);
		return r;
	}

	virtual ULONG __stdcall Release()
	{
		InterlockedDecrement(&r);
		if (r == 0)
		{
			delete this;
			return 0;
		}
		return r;
	}
	virtual STDMETHODIMP QueryInterface(REFIID iid, void** ppvObject)
	{
		if (!ppvObject)
			return E_POINTER;

		if (iid == __uuidof(IUnknown) || iid == __uuidof(IClassFactory))
		{
			AddRef();
			*ppvObject = this;
			return S_OK;
		}
		return E_NOINTERFACE;

	}

	int mode = 0;

	// IClassFactory
	virtual STDMETHODIMP LockServer(BOOL fLock)
	{
		return S_OK;
	}
	virtual STDMETHODIMP CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObject)
	{
		if (riid == IID_EcdcTransform1 || riid == IID_EcdcTransform2 || riid == __uuidof(IMFTransform) || riid == __uuidof(ICodecAPI))
		{
			*ppvObject = (EcdcTransform*)new EcdcTransform(mode);
			return S_OK;
		}
		return E_FAIL;
	}

};



STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
	if (!IsEqualIID(riid, IID_IUnknown)
		&& !IsEqualIID(riid, IID_IClassFactory))
	{
		return ResultFromScode(E_NOINTERFACE);
	}
	MyCF* mycf = new MyCF();
	if (rclsid == IID_EcdcTransform1)
		mycf->mode = 0;
	if (rclsid == IID_EcdcTransform2)
		mycf->mode = 1;
	*ppv = (void**)mycf;
	return S_OK;
}

STDAPI DllCanUnloadNow(void)
{
	return S_OK;
}


