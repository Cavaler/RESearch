@echo off
call UpdateVersion.bat
ren %@FINDFIRST[RESearch%V1%*.7z] RESearch%V1%%V2%%V3%.7z
7z u -ur0 %@FINDFIRST[RESearch%V1%*.7z]
7z u -ur0 RESearchPDB.7z
