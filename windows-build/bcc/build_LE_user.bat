@echo off

echo Build script for Steem LE - BCC build

call build_bcc_set.bat

set PROGRAMNAME=Beta BCC.exe

del "%OUT%\6301.obj"
del "%OUT%\AviFile.obj"
del "%OUT%\dsp.obj"
del "%OUT%\*.exe"

rem call build_bcc_asm.bat

echo -----------------------------------------------
echo Building 3rd party code using Borland C/C++ 5.5
echo -----------------------------------------------
"%BCCPATH%make.exe" -fmake_LE.txt -DDONT_ALLOW_DEBUG -DSTEVEN_SEAGAL 3rdparty
echo ------------------------------------------
echo Building Steem SSE using Borland C/C++ 5.5
echo ------------------------------------------
"%BCCPATH%make.exe" -fmake_LE.txt -DDONT_ALLOW_DEBUG -DBCC_BUILD -DSTEVEN_SEAGAL

if exist "%OUT%\Steem.exe" (ren "%OUT%\Steem.exe" "%PROGRAMNAME%"
copy "%OUT%\%PROGRAMNAME%" "%COPYPATH%")

pause
