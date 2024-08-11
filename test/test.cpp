// test.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <atlbase.h>
#include <vector>
#include <strmif.h>
#include <thread>
#include <string>
#include "..\\dll\\guids.hpp"
#include "..\\log.hpp"

#pragma comment(lib,"Mfplat.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"Mfreadwrite.lib")

#include "..\\common.h"

std::vector<char> FillPCM(int sr,int nch,int br,int secs)
{
	std::vector<char> pcm;
	pcm.resize(br / 8 * nch * sr * secs);

	// Fill with sine wave
	size_t ni = 0;
	for (int i = 0; i < secs; i++)
	{
		float hz = (float)(440.0f + i * 20.0f);
		for (int s = 0; s < sr; s++)
		{
			int mch = nch;
			float sample = 1.0f * (float)sin(2.0f * 3.1415f * (float)s / (float)sr * hz);
			if (br == 16)
			{
				short* p = (short*)pcm.data();
				short s = (short)(sample * 32767.0f);
				while (mch > 0)
				{
					p[ni++] = s;
					mch--;
				}
			}
			if (br == 8)
			{
				uint8_t* p = (uint8_t*)pcm.data();
				uint8_t s = (uint8_t)(sample * 127);
				while (mch > 0)
				{
					p[ni++] = s;
					mch--;
				}

			}
		}
	}
	return pcm;
}

int main()
{
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);
	MFStartup(MF_VERSION, MFSTARTUP_FULL);
	HRESULT hr = 0;

	// Check registration of the encoder
	if (1)
	{
		MFT_REGISTER_TYPE_INFO to = {};
		to.guidMajorType = MFMediaType_Audio;
		to.guidSubtype = MFAudioFormat_ECDC;
		CLSID* too = 0;
		UINT32 tooc = 0;
		hr = MFTEnum(MFT_CATEGORY_AUDIO_ENCODER, 0, 0, &to, 0, &too, &tooc);
		if (!tooc)
			return 0;
		CComPtr<IMFTransform> trs1;
		hr = CoCreateInstance(too[0], 0, CLSCTX_ALL, __uuidof(IMFTransform), (void**)&trs1);
		trs1 = 0;
		CoTaskMemFree(too);
	}

	// Check registration of the decoder
	if (1)
	{
		MFT_REGISTER_TYPE_INFO from = {};
		from.guidMajorType = MFMediaType_Audio;
		from.guidSubtype = MFAudioFormat_ECDC;
		CLSID* too = 0;
		UINT32 tooc = 0;
		hr = MFTEnum(MFT_CATEGORY_AUDIO_DECODER, 0, &from, 0, 0, &too, &tooc);
		if (!tooc)
			return 0;
		CComPtr<IMFTransform> trs1;
		hr = CoCreateInstance(too[0], 0, CLSCTX_ALL, __uuidof(IMFTransform), (void**)&trs1);
		trs1 = 0;
		CoTaskMemFree(too);
	}

	// Encode
	CComPtr<IMFSinkWriter> wr;

// https://developer.apple.com/documentation/quicktime-file-format/sound_sample_description_version_1

UINT8 cz[100] = {
0x0,0x0,0x0,100, // len
0x73,0x74,0x73,0x64, // stsd
0x0,0x0,0x0,0x0, // verflag
0x0,0x0,0x0,0x1, // num
0x0,0x0,0x0,84, // len
'e','c','d','c', // code
0x0,0x0,0x0,0x0,0x0,0x0, // 6-null
// index
0x0,0x1, 
// version
0x0,0x1, 
// Revision Level
0x0,0x0,
// vendor
0x0,0x0,0x0,0x0,
// number channels
0x0,0x2,
// sample size
0x0,0x10,
// Compression ID
0x0,0x0,
// Packet Size,
0x0,0x0,
// Sample rate
0xBB,0x80,0x0,0x0,


0x0,0x0,0x0,48, // 48 bytes
0x65,0x73,0x64,0x73, // 'esds'
0x0,0x0,0x0,0x0, // verflags
0x3,0x80,0x80,0x80,0x1F,0x0,0x0,0x0,0x4,0x80,0x80,0x80,0x14,0x40,0x15,0x0,0x6,0x0,0x0,0x2,0xEE,0x0,0x0,0x2,0xEE,0x0,0x5,0x80,0x80,0x80,0x2,0x11,0x90,0x6,0x1,0x2

};

//AAC mpeg descriptor

UINT8 caac[100] = {
0x0,0x0,0x0,0x64, // len
0x73,0x74,0x73,0x64, // stsd
0x0,0x0,0x0,0x0, // verflags
0x0,0x0,0x0,0x1, // num of dscriptiors
0x0,0x0,0x0,0x54, // len
0x6D,0x70,0x34,0x61, // 'mp4a' AAC 
0x0,0x0,0x0,0x0,0x0,0x0, //6-null
0x0,0x1, //	index
0x0,0x0, // version
0x0,0x0, // revision
0x0,0x0,0x0,0x0, // vendor
0x0,0x2, // channels
0x0,0x10, // sample size
0x0,0x0, // compression ID
0x0,0x0, // packet size
// Sample rate
0xBB,0x80,0x0,0x0,
0x0,0x0,0x0,0x30, // 48 bytes
0x65,0x73,0x64,0x73, // 'esds'
0x0,0x0,0x0,0x0, // verflags
0x3,0x80,0x80,0x80,0x1F,0x0,0x0,0x0,0x4,0x80,0x80,0x80,0x14,0x40,0x15,0x0,0x6,0x0,0x0,0x2,0xEE,0x0,0x0,0x2,0xEE,0x0,0x5,0x80,0x80,0x80,0x2,0x11,0x90,0x6,0x1,0x2
	};

	std::wstring fi = L"1.mp4";
	// These stay fixed
	int nch = 2;
	int sr = 48000;
	int br = 16;

	// These can change
	double bw = 3.0;
	bool AlsoVideo = 0;



	if (1)
	{
		DeleteFile(fi.c_str());
		MFCreateSinkWriterFromURL(fi.c_str(), 0, 0, &wr);

		DWORD sidx = 0, vidx = 0;
		if (AlsoVideo)
		{
			CComPtr<IMFMediaType> pMediaTypeOutVideo;
			CComPtr<IMFMediaType> pMediaTypeVideoIn;
			hr = MFCreateMediaType(&pMediaTypeVideoIn);
			hr = MFCreateMediaType(&pMediaTypeOutVideo);

			pMediaTypeVideoIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			pMediaTypeVideoIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
			MFSetAttributeSize(pMediaTypeVideoIn, MF_MT_FRAME_SIZE, 1920, 1080);
			MFSetAttributeRatio(pMediaTypeVideoIn, MF_MT_FRAME_RATE, 30, 1);
			MFSetAttributeRatio(pMediaTypeVideoIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

			pMediaTypeOutVideo->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
			pMediaTypeOutVideo->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
			MFSetAttributeSize(pMediaTypeOutVideo, MF_MT_FRAME_SIZE, 1920, 1080);
			MFSetAttributeRatio(pMediaTypeOutVideo, MF_MT_FRAME_RATE, 30, 1);
			MFSetAttributeRatio(pMediaTypeOutVideo, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
			pMediaTypeOutVideo->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
			pMediaTypeOutVideo->SetUINT32(MF_MT_AVG_BITRATE, 80000000);

			hr = wr->AddStream(pMediaTypeOutVideo, &vidx);
			hr = wr->SetInputMediaType(vidx, pMediaTypeVideoIn, NULL);
		}

		CComPtr<IMFMediaType> ecdc;
		MFCreateMediaType(&ecdc);
		if (wr)
		{
			// Add the audio stream
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

			ecdc->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
			ecdc->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_ECDC);

			ecdc->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, nch);
			ecdc->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, sr);

			hr = wr->AddStream(ecdc, &sidx);
			LogMediaType(ecdc);

			hr = wr->SetInputMediaType(sidx, ina, 0);
		}
		if (wr)
		{
			hr = wr->BeginWriting();

			// Set the bandwidth and show
			CComPtr<ICodecAPI> ca;
			auto hr = wr->GetServiceForStream(sidx, GUID_NULL, __uuidof(ICodecAPI), (void**)&ca);
			if (ca)
			{
				VARIANT v = {};
				v.vt = VT_R8;
				v.dblVal = bw;
				ca->SetValue(&MFEHDC_BANDWIDTH, &v);
				VARIANT v2 = {};
				v2.vt = VT_I4;
				v2.intVal = 1;
				ca->SetValue(&MFEHDC_VISIBLE, &v2);
			}

			// Set the sample descriptor
			if (1)
			{
				CComPtr<IMFMediaSink> pSink;
				CComPtr<IMFStreamSink> s2;
				CComPtr<IMFMediaTypeHandler> mth;
				CComPtr<IMFMediaType> mt2;
				wr->GetServiceForStream(MF_SINK_WRITER_MEDIASINK, GUID_NULL, IID_PPV_ARGS(&pSink));
				pSink->GetStreamSinkByIndex(sidx, &s2);
				s2->GetMediaTypeHandler(&mth);
				mth->GetCurrentMediaType(&mt2);
				LogMediaType(mt2);
				mt2->SetBlob(MF_MT_MPEG4_SAMPLE_DESCRIPTION, (UINT8*)cz, sizeof(cz));
				mt2->SetUINT32(MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY, 0);
				mth->SetCurrentMediaType(mt2);
				mt2 = 0;
				mth->GetCurrentMediaType(&mt2);
				LogMediaType(mt2);
			}

			int testseconds = 60;
			std::vector<char> pcm = FillPCM(sr, nch, br, testseconds);

			for (int i = 0; i < testseconds; i++)
			{
				if (sidx > 0)
				{
					// Also video
					unsigned int need = 1920 * 1080 * 4;
					CComPtr<IMFSample> s;
					MFCreateSample(&s);
					CComPtr<IMFMediaBuffer> b;
					MFCreateMemoryBuffer(need, &b);
					b->SetCurrentLength(need);
					s->AddBuffer(b);

					s->SetSampleTime(a2vi(sr * i, sr));
					s->SetSampleDuration(a2vi(sr, sr));
					hr = wr->WriteSample(vidx, s);
				}

				// 1 second of data
				CComPtr<IMFSample> s;
				MFCreateSample(&s);
				CComPtr<IMFMediaBuffer> b;
				MFCreateMemoryBuffer(sr * nch * (br / 8), &b);
				b->SetCurrentLength(sr * nch * (br / 8));
				BYTE* pb = 0;
				DWORD ml = 0, cl = 0;

				b->Lock(&pb, &ml, &cl);
				memcpy(pb, pcm.data() + i * sr * nch * (br / 8), sr * nch * (br / 8));
				b->Unlock();
				s->AddBuffer(b);


				s->SetSampleTime(a2vi(sr * i, sr));
				s->SetSampleDuration(a2vi(sr, sr));
				hr = wr->WriteSample(sidx, s);
			}

			hr = wr->Finalize();

			wr = 0;
		}
	}

	if (1)
	{
		CComPtr<IMFSourceReader> srr;
		hr = MFCreateSourceReaderFromURL(fi.c_str(), 0, &srr);
		if (srr)
		{
			CComPtr<IMFMediaType> c;
			srr->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &c);
			LogMediaType(c);

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

			LogMediaType(ina);
			hr = srr->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, ina);


			// Set the show
			CComPtr<ICodecAPI> ca;
			hr = srr->GetServiceForStream(MF_SOURCE_READER_FIRST_AUDIO_STREAM, GUID_NULL, __uuidof(ICodecAPI), (void**)&ca);
			if (ca)
			{
				VARIANT v2 = {};
				v2.vt = VT_I4;
				v2.intVal = 1;
				ca->SetValue(&MFEHDC_VISIBLE, &v2);
			}


		}
		if (SUCCEEDED(hr))
		{
			for (;;)
			{
				DWORD asidx = 0;
				DWORD flags = 0;
				LONGLONG gs = 0;
				CComPtr<IMFSample> s;
				srr->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &asidx, &flags, &gs, &s);

				if (!s)
				{
					if (flags & MF_SOURCE_READERF_ERROR)
						break;
					if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
						break;
				}


				CComPtr<IMFMediaBuffer> b;
				s->ConvertToContiguousBuffer(&b);
				if (!b)
					continue;

				DWORD ml = 0, cl = 0;
				BYTE* bb = 0;
				b->Lock(&bb, &ml, &cl);
				if (!bb)
					continue;

				std::vector<char> d(cl);
				memcpy(d.data(), bb, cl);
				//			PutFile(L"out.pcm", d, true);

				b->Unlock();
			}
		}
	}

	if (0)
	{
		// Media Sink
		auto hL = LoadLibrary(L"ecdc_mft.dll");
		typedef HRESULT(__stdcall* ii)(const wchar_t* file,IMFMediaSink**);
		ii I = (ii)GetProcAddress(hL, "CreateSink");
		if (I)
		{
			std::wstring fi2 = L"1.ecdc";
			DeleteFile(fi2.c_str());
			CComPtr<IMFMediaSink> ecdcsink;
			I(fi2.c_str(),&ecdcsink);
			if (ecdcsink)
			{
				hr = MFCreateSinkWriterFromMediaSink(ecdcsink,0, &wr);

				DWORD sidx = 0;
				if (wr)
				{
					// Add the audio stream
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

					ina->SetDouble(MFEHDC_BANDWIDTH, bw);
					ina->SetUINT32(MFEHDC_VISIBLE, 1);

					hr = wr->AddStream(ina, &sidx);
					hr = wr->SetInputMediaType(sidx, ina, 0);
				}
				if (wr)
				{
					hr = wr->BeginWriting();

					int testseconds = 60;
					std::vector<char> pcm = FillPCM(sr, nch, br, testseconds);

					for (int i = 0; i < testseconds; i++)
					{
						// 1 second of data
						CComPtr<IMFSample> s;
						MFCreateSample(&s);
						CComPtr<IMFMediaBuffer> b;
						MFCreateMemoryBuffer(sr * nch * (br / 8), &b);
						b->SetCurrentLength(sr * nch * (br / 8));
						BYTE* pb = 0;
						DWORD ml = 0, cl = 0;

						b->Lock(&pb, &ml, &cl);
						memcpy(pb, pcm.data() + i * sr * nch * (br / 8), sr * nch * (br / 8));
						b->Unlock();
						s->AddBuffer(b);


						s->SetSampleTime(a2vi(sr * i, sr));
						s->SetSampleDuration(a2vi(sr, sr));
						hr = wr->WriteSample(sidx, s);
					}

					hr = wr->Finalize();
					wr = 0;
					ecdcsink->Shutdown();
				}
			}
		}
		if (hL)
			FreeLibrary(hL);
	}

	return 0;
}
