@echo off
set V1=%@EXECSTR[grep PLUGIN_VERSION_MAJOR Version.h]
set V1=%@EXECSTR[cscript //nologo E:\USES\RegReplace.js "%V1%" ".*(\d+)" $1]

set V2=%@EXECSTR[grep PLUGIN_VERSION_MINOR Version.h]
set V2=%@EXECSTR[cscript //nologo E:\USES\RegReplace.js "%V2%" ".*(\d+)" $1]

set V3=%@EXECSTR[grep PLUGIN_VERSION_REVISION Version.h]
set V3=%@EXECSTR[cscript //nologo E:\USES\RegReplace.js "%V3%" ".*(\d+)" $1]

set VERSION=%V1%.%V2%%V3%
echo %VERSION%

ren /q File_Id.Diz File_Id.Old
cscript //nologo E:\USES\Replace2.js "version \d\.\d\d" "version %VERSION%" File_Id.Old File_Id.Diz
ren /q RESearchEng.hlf RESearchEng.Old
cscript //nologo E:\USES\Replace2.js "version \d\.\d\d#" "version %VERSION%#" RESearchEng.Old RESearchEng.hlf
ren /q RESearchRus.hlf RESearchRus.Old
cscript //nologo E:\USES\Replace2.js "\d\.\d\d#" "%VERSION%#" RESearchRus.Old RESearchRus.hlf
del /q *.Old
