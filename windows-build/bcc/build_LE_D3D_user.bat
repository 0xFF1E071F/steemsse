@echo off

echo Build script for Steem LE - BCC build

call build_bcc_set.bat

set PROGRAMNAME=Beta BCC LE.exe

del "%OUT%\6301.obj"
del "%OUT%\AviFile.obj"
del "%OUT%\dsp.obj"
del "%OUT%\*.exe"

rem call build_bcc_asm.bat

echo -----------------------------------------------
echo Building 3rd party code using Borland C/C++ 5.5
echo -----------------------------------------------
rem "%BCCPATH%make.exe" -fmake_LE.txt -DDONT_ALLOW_DEBUG -DSTEVEN_SEAGAL  -DSSE_D3D 3rdparty
"%BCCPATH%make.exe" -fmake_LE.txt -DDONT_ALLOW_DEBUG -DSTEVEN_SEAGAL -DSSE_D3D 3rdparty >build_user_3rdparty.log
echo ------------------------------------------
echo Building Steem SSE using Borland C/C++ 5.5
echo ------------------------------------------
rem "%BCCPATH%make.exe" -fmake_LE.txt -DDONT_ALLOW_DEBUG -DBCC_BUILD -DSTEVEN_SEAGAL  -DSSE_D3D
"%BCCPATH%make.exe" -fmake_LE.txt -DDONT_ALLOW_DEBUG -DBCC_BUILD -DSTEVEN_SEAGAL -DSSE_D3D >build_user_steem.log

if not exist "%OUT%\Steem.exe" echo ERROR
if exist "%OUT%\Steem.exe" (ren "%OUT%\Steem.exe" "%PROGRAMNAME%"
copy "%OUT%\%PROGRAMNAME%" "%COPYPATH%") 

pause
