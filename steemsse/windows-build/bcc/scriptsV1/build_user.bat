@echo off
echo These are the BCC (Borland C++ Compiler) build scripts
echo for Steem (user / release build).
echo You need BCC from https://downloads.embarcadero.com/item/24778
echo and NASM from http://www.nasm.us/

rem local:
cd c:\data\prg\st\windows-build\bcc

set NASMPATH=D:\Console\nasm
set BCCROOT=D:\Console\bcc\
set BCCPATH=D:\Console\bcc\bin\
set OUT=_UserRelease
set ROOT=..\..
rem rmdir /S /Q "%OUT%"
rem mkdir "%OUT%"
del "%OUT%\*.exe"

rem "%NASMPATH%nasm" -o obj\asm_draw.obj -fobj -dWIN32 -w+macro-params -w+macro-selfref -w+orphan-labels -i%ROOT%\steem\asm\ %ROOT%\steem\asm\asm_draw.asm
rem "%NASMPATH%nasm" -o obj\asm_osd_draw.obj -fobj -dWIN32 -w+macro-params -w+macro-selfref rem -w+orphan-labels -i%ROOT%\steem\asm\ %ROOT%\steem\asm\asm_osd_draw.asm
"%NASMPATH%nasm" -o obj\asm_portio.obj -fobj -dWIN32 %ROOT%\include\asm\asm_portio.asm

"%NASMPATH%nasm" -o obj\asm_draw.obj -fobj -dWIN32 -w+macro-params -w+macro-selfref -w+orphan-labels -i%ROOT%\steem\asm\ %ROOT%\steem\asm\asm_draw.asm
"%NASMPATH%nasm" -o obj\asm_osd_draw.obj -fobj -dWIN32 -w+macro-params -w+macro-selfref -w+orphan-labels -i%ROOT%\steem\asm\ %ROOT%\steem\asm\asm_osd_draw.asm
"%NASMPATH%nasm" -o obj\asm_portio.obj -fobj -dWIN32 %ROOT%\include\asm\asm_portio.asm

"%BCCPATH%make.exe" 3rdparty
"%BCCPATH%make.exe" -B -DDONT_ALLOW_DEBUG -DBCC_BUILD -DSTEVEN_SEAGAL

del "%OUT%\*.tds"
ren "%OUT%\Steem.exe" "SteemBetaBCC.exe"
copy "%OUT%\SteemBetaBCC.exe" G:\emu\st\bin\steem\"
pause
:END_BUILD