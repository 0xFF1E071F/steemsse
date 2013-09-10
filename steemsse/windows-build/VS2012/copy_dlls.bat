@echo off
echo copy all dll to the %1 output directory
copy /b /y ..\..\3rdparty\caps\CAPSImg.dll %1
copy /b /y ..\..\3rdparty\pasti\pasti.dll %1
copy /b /y ..\..\3rdparty\UnRARDLL\unrar.dll %1