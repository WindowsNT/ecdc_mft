# ECDC MFT
Conversion of Facebook's Encodec to a Media Foundation DLL so be used in Media Foundation Applications

## Introduction

[FacebookResearch Encodec](https://github.com/facebookresearch/encodec) is a state-of-the-art AI audio compression and decompression high extremely high compression ratios.

## Building
A VS solution is provided to build the installer, the DLL and a test application automatically. Just load ecdc_mft.sln. Alternatively, if you just want to install it, run the prebuilt, digitally signed executables ecdc_mft_installer64.exe and ecdc_mft_installer32.exe.

ECDC MFT requires x64. `ecdc_mft_installer32.exe` will install the 32-bit DLL for 32-bit applications all right, but only under 64-bit WOW.

## Installation

Installer will:

	1. Install the dll and uninstaller in System directory
	2. Install to c:\ProgramData\EncodecMFT{BE60AF19-1EFD-4130-8D0F-43EBAC9D6C8F}:
		* Python
		* Torch for CUDA if you have a NVidia card (really recommended) or plain Torch
		* Encodec
	
# Using in your own applications

If you want to produce MP4 files that use Encodec then check the test.cpp. You can use the IMFSinkWriter with :

* Source media type PCM 48000Hz, 16-bit, 2 channels
* Target media type with the provided MFAudioFormat_ECDC
* With ICodecAPI you can set `MFEHDC_BANDWIDTH` and `MFEHDC_VISIBLE`
* Before Finalize, pass the provided sample descriptor with MF_MT_MPEG4_CURRENT_SAMPLE_ENTRY

My sequencer [Turbo Play](https://www.turbo-play.com) can create also .ecdc files automatically.
Note that the Encodec MFT processes the entire input at once, that means that you will see the Encodec running only when calling `Finalize()`.

# Using the decompressor

If you have installed Encodec MFT, then Media-Foundation capable appliiocations will automatically be able to open mp4 files compressed with encodec MFT. In your application you can

* Create the IMFSourceReader
* Set the current media type to PCM 48000Hz, 16-bit, 2 channels
* Read samples with `ReadSample`
* With ICodecAPI you can set `MFEHDC_VISIBLE`

Note that the Encodec MFT processes the entire input at once, that means that you will see the Encodec running when trying to read the first sample with `ReadSample()`.



	


 