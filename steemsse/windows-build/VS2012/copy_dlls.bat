@echo off
echo copying required dll(s) to %1 directory
if not exist %1\CAPSImg.dll (copy /b /y ..\..\3rdparty\caps\CAPSImg.dll %1)
if not exist %1\pasti.dll (copy /b /y ..\..\3rdparty\pasti\pasti.dll %1)
if not exist %1\unrar.dll (copy /b /y ..\..\3rdparty\UnRARDLL\unrar.dll %1)
del /q %1\*.ilk
del /q %1\*.pdb