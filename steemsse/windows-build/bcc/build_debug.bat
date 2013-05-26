@echo off

echo Build script for Steem SSE - BCC build with boiler

rem local:

set NASMPATH=D:\Console\nasm\
set BCCROOT=D:\Console\bcc\
set BCCPATH=D:\Console\bcc\bin\
set COPYPATH=G:\emu\st\bin\steem\
rem set ROOT=c:\data\prg\st\
set ROOT=G:\Downloads\steemsse-code\steemsse\
rem the rest should be the same for all systems:

set PROGRAMNAME=BoilerBeta BCC.exe
rem manually create obj directory if necessary
set OUT=obj
rem set ROOT=..\..
del "%OUT%\*.exe"

rem delete objects if you want to remake them (rare)
if NOT EXIST obj\asm_draw.obj (
@echo ON
"%NASMPATH%nasm" -o obj\asm_draw.obj -fobj -dWIN32 -w+macro-params -w+macro-selfref -w+orphan-labels -i%ROOT%\steem\asm\ %ROOT%\steem\asm\asm_draw.asm
)
if NOT EXIST obj\asm_osd_draw.obj (
@echo ON
"%NASMPATH%nasm" -o obj\asm_osd_draw.obj -fobj -dWIN32 -w+macro-params -w+macro-selfref -w+orphan-labels -i%ROOT%\steem\asm\ %ROOT%\steem\asm\asm_osd_draw.asm
)
if NOT EXIST obj\asm_portio.obj (
@echo ON
"%NASMPATH%nasm" -o obj\asm_portio.obj -fobj -dWIN32 %ROOT%\include\asm\asm_portio.asm
)
@echo OFF

echo -----------------------------------------------
echo Building 3rd party code using Borland C/C++ 5.5
echo -----------------------------------------------
"%BCCPATH%make.exe" -fmakefile.txt -DFORCE_DEBUG_BUILD 3rdparty
echo ---------------------------------------------------
echo Building Steem SSE (Boiler) using Borland C/C++ 5.5
echo ---------------------------------------------------

"%BCCPATH%make.exe" -fmakefile.txt -B -DFORCE_DEBUG_BUILD -DBCC_BUILD -DSTEVEN_SEAGAL

ren "%OUT%\Steem.exe" "%PROGRAMNAME%"
copy "%OUT%\%PROGRAMNAME%" "%COPYPATH%"

pause
:END_BUILD
