@echo off

echo Build script for Steem SSE - BCC build with boiler

call build_bcc_set.bat

set PROGRAMNAME=BoilerBeta BCC.exe

del "%OUT%\6301.obj"
del "%OUT%\AviFile.obj"
del "%OUT%\dsp.obj"
del "%OUT%\*.exe"

rem call build_bcc_asm.bat

echo -----------------------------------------------
echo Building 3rd party code using Borland C/C++ 5.5
echo -----------------------------------------------
"%BCCPATH%make.exe" -fmakefile.txt -DFORCE_DEBUG_BUILD 3rdparty
rem pause
echo ---------------------------------------------------
echo Building Steem SSE (Boiler) using Borland C/C++ 5.5
echo ---------------------------------------------------

"%BCCPATH%make.exe" -fmakefile.txt -DFORCE_DEBUG_BUILD -DBCC_BUILD  

rem "%BCCPATH%make.exe" -fmakefile.txt -DFORCE_DEBUG_BUILD -DBCC_BUILD link  


if exist "%OUT%\Steem.exe" (ren "%OUT%\Steem.exe" "%PROGRAMNAME%"
copy "%OUT%\%PROGRAMNAME%" "%COPYPATH%")

pause

