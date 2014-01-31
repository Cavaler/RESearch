@echo off
call UpdateVersion.bat
ren %@FINDFIRST[RESearch%V1%*.rar] RESearch%V1%%V2%%V3%.rar
rar f -r %@FINDFIRST[RESearch%V1%*.rar]
rar u -r %@FINDFIRST[RESearch%V1%*.rar] Add-ons
rar f RESearchPDB.rar
