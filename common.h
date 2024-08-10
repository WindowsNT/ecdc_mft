
class PushPopDir
{
	std::vector<wchar_t> cd;
public:

	PushPopDir(const wchar_t* f)
	{
		cd.resize(1000);
		GetCurrentDirectory(1000, cd.data());
		SetCurrentDirectory(f);
	}
	~PushPopDir()
	{
		SetCurrentDirectory(cd.data());
	}
};



class PriLowtemp
{
	DWORD c1 = 0;
public:
	PriLowtemp()
	{
		c1 = GetPriorityClass(GetCurrentProcess());
		SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
	}
	~PriLowtemp()
	{
		SetPriorityClass(GetCurrentProcess(), c1);
	}

};


template <typename T = char>
inline bool PutFile(const wchar_t* f, std::vector<T>& d, bool Fw = false)
{
	HANDLE hX = CreateFile(f, GENERIC_WRITE, 0, 0, Fw ? CREATE_ALWAYS : CREATE_NEW, 0, 0);
	if (hX == INVALID_HANDLE_VALUE)
		return false;
	DWORD A = 0;
	WriteFile(hX, d.data(), (DWORD)(d.size() * sizeof(T)), &A, 0);
	CloseHandle(hX);
	if (A != d.size())
		return false;
	return true;
}

template <typename T = char>
inline bool LoadFile(const wchar_t* f, std::vector<T>& d)
{
	HANDLE hX = CreateFile(f, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	if (hX == INVALID_HANDLE_VALUE)
		return false;
	LARGE_INTEGER sz = { 0 };
	GetFileSizeEx(hX, &sz);
	d.resize((size_t)(sz.QuadPart / sizeof(T)));
	DWORD A = 0;
	ReadFile(hX, d.data(), (DWORD)sz.QuadPart, &A, 0);
	CloseHandle(hX);
	if (A != sz.QuadPart)
		return false;
	return true;
}


unsigned long long a2vi(unsigned long long A, int SR)
{
	unsigned long long X = (unsigned long long)(1000 * 10 * 1000 * A) / (unsigned long long)SR;
	return X;
};

std::wstring TempFile()
{
	std::vector<wchar_t> td(1000);
	GetTempPathW(1000, td.data());
	std::vector<wchar_t> x(1000);
	GetTempFileNameW(td.data(), L"tmp", 0, x.data());
	DeleteFile(x.data());
	return x.data();
}

DWORD crc32(DWORD crc, const unsigned char* ptr, size_t buf_len)
{
#define MZ_CRC32_INIT (0)
	static const DWORD s_crc32[16] = { 0, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
		0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c };
	DWORD crcu32 = (DWORD)crc;
	if (!ptr) return MZ_CRC32_INIT;
	crcu32 = ~crcu32; while (buf_len--) { DWORD b = *ptr++; crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b & 0xF)]; crcu32 = (crcu32 >> 4) ^ s_crc32[(crcu32 & 0xF) ^ (b >> 4)]; }
	return ~crcu32;
}

static int CudaAv = 0;
#include <dxgi.h>
#pragma comment(lib,"dxgi.lib")
bool IsCudaAvailable()
{
	if (CudaAv == 1)
		return true;
	if (CudaAv == -1)
		return false;


	CudaAv = -1;
	CComPtr<IDXGIFactory> pFactory = NULL;
	CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
	for (UINT ia = 0; ; ia++)
	{
		CComPtr<IDXGIAdapter> ad;
		pFactory->EnumAdapters(ia, &ad);
		if (!ad)
			break;

		DXGI_ADAPTER_DESC desc = {};
		ad->GetDesc(&desc);
		if (desc.VendorId == 4318)
			CudaAv = 1;
		for (UINT io = 0; ; io++)
		{
			CComPtr<IDXGIOutput> out;
			ad->EnumOutputs(io, &out);
			if (!out)
				break;

			DXGI_OUTPUT_DESC desc2 = {};
			out->GetDesc(&desc2);
		}
	}

	if (CudaAv == 1)
		return true;
	return false;
}


HRESULT ExtractResource(HINSTANCE hXX, const TCHAR* Name, const TCHAR* ty, std::vector<char>& data)
{
	HRSRC R = FindResource(hXX, Name, ty);
	if (!R)
		return E_NOINTERFACE;
	HGLOBAL hG = LoadResource(hXX, R);
	if (!hG)
		return E_FAIL;
	DWORD S = SizeofResource(hXX, R);
	char* p = (char*)LockResource(hG);
	if (!p)
	{
		FreeResource(R);
		return E_FAIL;
	}
	data.resize(S);
	memcpy(data.data(), p, S);
	FreeResource(R);
	return S_OK;
}


 HRESULT ExtractResourceToFile(HINSTANCE hXX, const TCHAR* Name, const TCHAR* ty, const wchar_t* putfile, bool skipifexists = false)
{
	HRSRC R = FindResource(hXX, Name, ty);
	if (!R)
		return E_NOINTERFACE;
	HGLOBAL hG = LoadResource(hXX, R);
	if (!hG)
		return E_FAIL;
	DWORD S = SizeofResource(hXX, R);
	char* p = (char*)LockResource(hG);
	if (!p)
	{
		FreeResource(R);
		return E_FAIL;
	}
	HANDLE w = CreateFile(putfile, GENERIC_WRITE, 0, 0, skipifexists ? CREATE_NEW : CREATE_ALWAYS, 0, 0);

	if (w == INVALID_HANDLE_VALUE)
	{
		FreeResource(R);
		return E_FAIL;
	}
	DWORD A;
	WriteFile(w, p, S, &A, 0);
	CloseHandle(w);
	FreeResource(R);
	return S_OK;
}






 INT_PTR CALLBACK MESSAGE_DP(HWND hh, UINT mm, WPARAM ww, LPARAM ll)
 {
	 return 0;
 }


 DWORD Run(HINSTANCE hX,const wchar_t* y, bool W, DWORD flg, const wchar_t* msg)
 {
	 PriLowtemp plt;
	 PROCESS_INFORMATION pInfo = { 0 };
	 STARTUPINFO sInfo = { 0 };

	 sInfo.cb = sizeof(sInfo);
	 wchar_t yy[1000] = {};
	 swprintf_s(yy, 1000, L"%s", y);


	 HWND hT = 0;
	 DWORD tid = 0;
	 std::thread* t = 0;
	 if (msg)
	 {
		 t = new std::thread([&]
			 {
				 tid = GetCurrentThreadId();
				 hT = CreateDialog(hX, L"DIALOG_MESSAGE", 0, MESSAGE_DP);
				 SetWindowText(GetDlgItem(hT, 101), msg);
				 MSG msg;
				 while (GetMessage(&msg, 0, 0, 0))
				 {
					 TranslateMessage(&msg);
					 DispatchMessageW(&msg);
				 }
			 });
		 Sleep(100);
	 }

	 CreateProcess(0, yy, 0, 0, 0, flg, 0, 0, &sInfo, &pInfo);
	 SetPriorityClass(pInfo.hProcess, IDLE_PRIORITY_CLASS);
	 SetThreadPriority(pInfo.hThread, THREAD_PRIORITY_IDLE);
	 if (W)
		 WaitForSingleObject(pInfo.hProcess, INFINITE);

	 if (msg)
	 {
		 SendMessage(hT, WM_CLOSE, 0, 0);
		 PostThreadMessage(tid, WM_QUIT, 0, 0);
		 t->join();
		 delete t;
		 t = 0;
	 }
	 DWORD ec = 0;
	 GetExitCodeProcess(pInfo.hProcess, &ec);
	 CloseHandle(pInfo.hProcess);
	 CloseHandle(pInfo.hThread);
	 return ec;
 }

#include "rkey.h"


