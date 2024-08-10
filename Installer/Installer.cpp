// Installer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//



#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")



#include <Windows.h>
#include <string>
#include <ShlObj.h>
#include <vector>
#include <thread>
#include <atlbase.h>
#include "..\\common.h"

std::wstring InstallECDC()
{
	auto hX = GetModuleHandle(0);

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

	SHCreateDirectory(0, Where.c_str());
	PushPopDir ppd(Where.c_str());
	// Install python
	auto tf = Where + L"\\python.7z";
	ExtractResourceToFile(hX, L"PYTHON", L"DATA", tf.c_str(), false);

	auto tf2 = Where + L"\\7zr.exe";
	ExtractResourceToFile(hX, L"_7Z", L"DATA", tf2.c_str(), false);

	Run(hX,L"7zr x python.7z -y", true, 0, L"Installing Python...");
	Run(hX,L"scripts\\pip.exe install numpy<2", true, 0, L"Installing Numpy...");
	if (IsCudaAvailable())
		Run(hX,L"scripts\\pip.exe install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118", true, 0, L"Installing Torch + CUDA ...");
	else
		Run(hX,L"scripts\\pip.exe install torch torchvision torchaudio", true, 0, L"Installing Torch...");
	Run(hX,L"scripts\\pip.exe install soundfile encodec", true, 0, L"Installing Encodec...");

	return Where;
}



int RemoveDir(LPCTSTR dir, bool Show) // Fully qualified name of the directory being   deleted,   without trailing backslash
{
	std::vector<wchar_t> td(10000);
	wcscpy_s(td.data(), 10000, dir);

	// Delete entirely
	td.data()[wcslen(td.data())] = 0;
	td.data()[wcslen(td.data()) + 1] = 0;
	SHFILEOPSTRUCT shf = {};
	shf.wFunc = FO_DELETE;
	shf.pFrom = td.data();
	shf.pTo = 0;
	shf.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI;
	if (Show)
		shf.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI;
	auto sh = SHFileOperation(&shf);
	RemoveDirectory(dir);
	return sh;
}



void Remove()
{
	// Get AppData
	PWSTR p = 0;
	SHGetKnownFolderPath(FOLDERID_ProgramData, 0, 0, &p);
	if (p)
	{
		std::wstring Where = p;
		Where += L"\\EncodecMFT{BE60AF19-1EFD-4130-8D0F-43EBAC9D6C8F}";
		CoTaskMemFree(p);
		RemoveDir(Where.c_str(), true);
	}

	RKEY r2(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall)", KEY_ALL_ACCESS);
	r2.DeleteSingle(L"{AB87DC03-AACF-4A75-8739-D83CF14BE9C5}");

	wchar_t wd[1000] = {};
	GetSystemDirectory(wd, 1000);
	wchar_t wdt[1000] = {};
	swprintf_s(wdt, L"%s\\ecdc_mft.dll", wd);


	typedef HRESULT(__stdcall* x1)();
	auto hX = LoadLibrary(wdt);
	x1 X1 = (x1)GetProcAddress(hX, "DllUnregisterServer");
	if (X1)
		X1();
	if (hX)
		FreeLibrary(hX);
	DeleteFile(wdt);
}

HRESULT Install()
{
	wchar_t wd[1000] = {};
	GetSystemDirectory(wd, 1000);
	wchar_t me[1000] = {};
	GetModuleFileName(0,me,1000);
	wchar_t wdt[1000] = {};
	swprintf_s(wdt, L"%s\\ecdc_mft.dll", wd);
	wchar_t wdte[1000] = {};
	swprintf_s(wdte, L"%s\\ecdc_mft{AB87DC03-AACF-4A75-8739-D83CF14BE9C5}.exe", wd);

	RKEY r(HKEY_LOCAL_MACHINE, LR"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{AB87DC03-AACF-4A75-8739-D83CF14BE9C5})", KEY_ALL_ACCESS);
	if (r.operator HKEY())
	{
		if (GetFileAttributes(wdt) != 0xFFFFFFFF)
		{
			Remove();
			return E_FAIL;
		}

		if (FAILED(ExtractResourceToFile(GetModuleHandle(0), L"DLL", L"DATA", wdt, false)))
		{
			Remove();
			return E_FAIL;
		}

		if (!CopyFile(me, wdte, false))
		{
			Remove();
			return E_FAIL;
		}

		auto hX = LoadLibrary(wdt);
		typedef HRESULT(__stdcall* x1)();
		x1 X1 = (x1)GetProcAddress(hX, "DllRegisterServer");
		if (!hX || !X1)
		{
			Remove();
			return E_FAIL;
		}
		auto fo = InstallECDC();
		if (fo.empty())
		{
			FreeLibrary(hX);
			Remove();
			return E_FAIL;
		}


		wchar_t d[1000] = {};
		r[L"DisplayName"] = L"ECDC MFT";
		r[L"Publisher"] = L"TURBO-PLAY.COM";
		SYSTEMTIME sT;
		GetLocalTime(&sT);

		swprintf_s(d, 1000, L"%04u%02u%02u", sT.wYear, sT.wMonth, sT.wDay);
		r[L"InstallDate"] = d;
		r[L"UninstallString"] = wdte;
		r[L"QuietUninstallString"] = wdte;
		swprintf_s(d, 1000, L"%s", wdt);
		r[L"DisplayIcon"] = d;
		r[L"DisplayVersion"].operator=(L"1.0");
		r[L"URLInfoAbout"].operator=(L"https://www.turbo-play.com/ecdcmft");

		auto hr = X1();
		FreeLibrary(hX);
		return hr;
	}
	return E_FAIL;
}



bool IsInstalled()
{
	RKEY k(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{AB87DC03-AACF-4A75-8739-D83CF14BE9C5}", KEY_READ | KEY_WOW64_64KEY, true);
	return k.Valid();
}



int __stdcall WinMain(HINSTANCE,HINSTANCE,LPSTR,int)
{
	if (IsInstalled())
	{
		// Remove
		if (MessageBox(0, L"ECDC MFT is installed.\r\nRemove it?", L"ECDC MFT", MB_YESNO) == IDNO)
			return 0;
		Remove();
	}
	else
	{
		// Install
		if (MessageBox(0, L"Install ECDC MFT?", L"ECDC MFT", MB_YESNO) == IDNO)
			return 0;
		Install();
	}
}

