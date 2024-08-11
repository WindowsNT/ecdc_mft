@echo off

msbuild ecdc_mft.sln /clp:ErrorsOnly /m /p:Configuration="Release" /p:Platform=x64
msbuild ecdc_mft.sln /clp:ErrorsOnly /m /p:Configuration="Release" /p:Platform=x86
copy .\x64\Release\ecdc_mft_installer.exe ecdc_mft_installer64.exe /y
copy .\Release\ecdc_mft_installer.exe ecdc_mft_installer32.exe /y
