@echo off
call UpdateVersion.bat
rar f -r %@FINDFIRST[RESearch8*.rar]
rar u -r %@FINDFIRST[RESearch8*.rar] Add-ons
rar f RESearchPDB.rar
