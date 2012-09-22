@echo off

rem For convenience, this sript is supposed to be run from the place
rem where the executable will be copied. An obj folder must exist.

echo Build script for Steem SSE - BCC build with boiler

rem set those variables to the correct directories if needed
rem ROOT is the folder above 3rdparty & steem, it is the top folder 
rem for source files, C_ST on my computer
rem schema:
rem from wherever on your disk
rem  |
rem  C_ST
rem   |
rem   steem
rem    |
rem    code
rem    ...
rem   3rdparty
rem   include
rem   windows-build
rem   X-build
rem  NASM (folder containing the NASM exe)
rem  BCC (Borland C++ 5.5 folder)
rem   |
rem   bin
rem  emu
rem   |
rem   Steem (WE ARE HERE: makefile & bat scripts)
rem    |
rem    BCCDebugBuild       you must create this folder
rem    BCCNormalBuild      you must create this folder
rem    obj                 you must create this folder
rem  ...

set ROOT=..\..\C_ST\
set NASMPATH=..\..\nasm\
set BCCROOT=..\..\bcc\
set BCCPATH=..\..\bcc\Bin\

set OUT=BCCDebugBuild
del %OUT%\*.exe

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
"%BCCPATH%make.exe" -DSS_DEBUG 3rdparty
echo ---------------------------------------------------
echo Building Steem SSE (Boiler) using Borland C/C++ 5.5
echo ---------------------------------------------------
set SS_BOILER=1
"%BCCPATH%make.exe" -B -D_FORCE_DEBUG_BUILD -DBCC_BUILD -DSTEVEN_SEAGAL

ren %OUT%\Steem.exe Boiler_Beta.exe
del Boiler_Beta.exe
copy %OUT%\Boiler_Beta.exe
pause
:END_BUILD
