# Microsoft Developer Studio Project File - Name="RESearch" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=RESearch - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RESearch.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RESearch.mak" CFG="RESearch - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RESearch - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RESearch - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RESearch - Win32 Release Intel" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RESearch - Win32 Release NET" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/WINDOWS/FAR/RESearch", YBAAAAAA"
# PROP Scc_LocalPath "."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "RESearch - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir ".\Release"
# PROP BASE Intermediate_Dir ".\Release"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G6 /Zp2 /MD /W3 /GX /Ox /Ot /Oa /Og /Oi /Oy- /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "PCRE_STATIC" /D "FAR_USE_NAMESPACE" /FD /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 PavelMD.lib pcreMD.lib kernel32.lib user32.lib advapi32.lib /nologo /version:4.2 /subsystem:windows /dll /machine:I386 /out:"RESearch.dll" /force:multiple /FILEALIGN:0x200
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "RESearch - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir ".\Debug"
# PROP BASE Intermediate_Dir ".\Debug"
# PROP BASE Target_Dir "."
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir "."
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /G5 /Zp1 /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "PCRE_STATIC" /D "FAR_USE_NAMESPACE" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386
# ADD LINK32 PavelMDd.lib pcreMDd.lib kernel32.lib user32.lib advapi32.lib /nologo /version:4.0 /subsystem:windows /dll /debug /machine:I386 /out:"C:\Program Files\Far\Plugins\_Debug\RESearch\RESearch.dll"
# SUBTRACT LINK32 /profile /incremental:no /map

!ELSEIF  "$(CFG)" == "RESearch - Win32 Release Intel"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release Intel"
# PROP BASE Intermediate_Dir "Release Intel"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_Intel"
# PROP Intermediate_Dir "Release_Intel"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G5 /Zp1 /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "PCRE_STATIC" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G6 /Zp1 /MD /W3 /GX /Ox /Ot /Oa /Og /Oi /Oy /Ob2 /X /I "\CPP\STLport-4.5.3\stlport" /I "D:\MSDEV\VC98\Include" /I "\CPP\Include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "PCRE_STATIC" /D "FAR_USE_NAMESPACE" /D _USE_COMPILER=INTEL /FD /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 LibCTiny.lib Pavel.lib PCRegEx.lib kernel32.lib user32.lib advapi32.lib /nologo /version:4.0 /subsystem:windows /dll /machine:I386 /force /out:"RESearch.dll"
# SUBTRACT BASE LINK32 /pdb:none /incremental:yes
# ADD LINK32 LibCTinyMDI.lib PavelMDI.lib pcreMDI.lib kernel32.lib user32.lib advapi32.lib /nologo /version:4.0 /subsystem:windows /dll /machine:I386 /out:"RESearchI.dll" /force:multiple /D_USE_INTEL_COMPILER
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "RESearch - Win32 Release NET"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "RESearch___Win32_Release_NET"
# PROP BASE Intermediate_Dir "RESearch___Win32_Release_NET"
# PROP BASE Ignore_Export_Lib 1
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_NET"
# PROP Intermediate_Dir "Release_NET"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /G6 /Zp2 /MD /W3 /GX /Oy /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "PCRE_STATIC" /D "FAR_USE_NAMESPACE" /FD /c
# SUBTRACT BASE CPP /YX
# ADD CPP /nologo /G6 /Zp2 /MD /W3 /GX /Ox /Ot /Oa /Og /Oi /Oy- /Ob2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "PCRE_STATIC" /D "FAR_USE_NAMESPACE" /D _USE_COMPILER=VC7 /FD /GF /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 PavelMD.lib pcreMD.lib kernel32.lib user32.lib advapi32.lib /nologo /version:4.2 /subsystem:windows /dll /machine:I386 /out:"RESearch.dll" /force:multiple
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 PavelMD7.lib pcreMD7.lib kernel32.lib user32.lib advapi32.lib /nologo /version:4.2 /subsystem:windows /dll /machine:I386 /out:"RESearch7.dll" /FILEALIGN:0x200 /D_USE_COMPILER=VC7 /force:multiple
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "RESearch - Win32 Release"
# Name "RESearch - Win32 Debug"
# Name "RESearch - Win32 Release Intel"
# Name "RESearch - Win32 Release NET"
# Begin Group "EditFind Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\EditFind\EditCommon.cpp
# End Source File
# Begin Source File

SOURCE=.\EditFind\EditFilter.cpp
# End Source File
# Begin Source File

SOURCE=.\EditFind\EditFind.h
# End Source File
# Begin Source File

SOURCE=.\EditFind\EditReplace.cpp
# End Source File
# Begin Source File

SOURCE=.\EditFind\EditSearch.cpp
# End Source File
# End Group
# Begin Group "FileFind Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FileFind\FileCommon.cpp
# End Source File
# Begin Source File

SOURCE=.\FileFind\FileFind.h
# End Source File
# Begin Source File

SOURCE=.\FileFind\FileReplace.cpp
# End Source File
# Begin Source File

SOURCE=.\FileFind\FileSearch.cpp
# End Source File
# Begin Source File

SOURCE=.\FileFind\TmpPanel.cpp
# End Source File
# End Group
# Begin Group "FileTools Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\FileTools\FileTools.cpp
# End Source File
# Begin Source File

SOURCE=.\FileTools\FileTools.h
# End Source File
# End Group
# Begin Group "ViewFind Sources"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\ViewFind\ViewCommon.cpp
# End Source File
# Begin Source File

SOURCE=.\ViewFind\ViewFind.h
# End Source File
# Begin Source File

SOURCE=.\ViewFind\ViewSearch.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Presets.h
# End Source File
# Begin Source File

SOURCE=.\RESearch.h
# End Source File
# End Group
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Common.cpp
# End Source File
# Begin Source File

SOURCE=.\Presets.cpp
# End Source File
# Begin Source File

SOURCE=.\RESearch.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\File_Id.Diz
# End Source File
# Begin Source File

SOURCE=.\History.txt
# End Source File
# Begin Source File

SOURCE=.\RESearch.def
# End Source File
# Begin Source File

SOURCE=.\RESearch.rc
# End Source File
# Begin Source File

SOURCE=.\RESEng.hlf
# End Source File
# Begin Source File

SOURCE=.\RESEng.lng
# End Source File
# Begin Source File

SOURCE=.\RESRus.lng
# End Source File
# End Target
# End Project
