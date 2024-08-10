@echo off

msbuild ecdc_mft.sln /clp:ErrorsOnly /m /p:Configuration="Release" /p:Platform=x64
msbuild ecdc_mft.sln /clp:ErrorsOnly /m /p:Configuration="Release" /p:Platform=x86
 