@echo off

echo Build script for Steem SSE - MinGW build 

call build_mingw_set.bat

del "%OUT%\6301.o"
del "%OUT%\AviFile.o"
del "%OUT%\dsp.o"
del "%OUT%\*.exe"

echo -----------------------------------------------
echo Building 3rd party code using MinGW
echo -----------------------------------------------
mingw32-make.exe -fmakefile_MinGW.txt USER=1 3rdparty MAKEFILE_PATH=makefile_MinGW.txt

echo -----------------------------------------------
echo Building Steem SSE using MinGW
echo -----------------------------------------------
rem STEVEN_SEAGAL is defined in the makefile
mingw32-make.exe -fmakefile_MinGW.txt USER=1 MAKEFILE_PATH=makefile_MinGW.txt

call build_mingw_user_link.bat

pause



