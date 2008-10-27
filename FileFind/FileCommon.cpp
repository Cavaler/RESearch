#include "StdAfx.h"
#include "..\RESearch.h"

CParameterSet g_FSParamSet(FileSearchExecutor, 4, 5,
	"Mask", &MaskText, "Text", &SearchText, "@Mask", &FMask, "@Text", &FText,
	"MaskAsRegExp", &FMaskAsRegExp, "CaseSensitive", &FCaseSensitive,
	"UTF8", &FUTF8, "SearchAs", &FSearchAs, "IsInverse", &FSInverse
	);
CParameterSet g_FRParamSet(FileReplaceExecutor, 6, 4,
	"Mask", &MaskText, "Text", &SearchText, "Replace", &ReplaceText,
	"@Mask", &FMask, "@Text", &FText, "@Replace", &FRReplace,
	"MaskAsRegExp", &FMaskAsRegExp, "CaseSensitive", &FCaseSensitive,
	"UTF8", &FUTF8, "SearchAs", &FSearchAs
	);
CParameterSet g_FGParamSet(FileGrepExecutor, 4, 9,
	"Mask", &MaskText, "Text", &SearchText, "@Mask", &FMask, "@Text", &FText,
	"MaskAsRegExp", &FMaskAsRegExp, "CaseSensitive", &FCaseSensitive,
	"UTF8", &FUTF8, "SearchAs", &FSearchAs, "IsInverse", &FSInverse,
	"GrepWhat", &FGrepWhat, "AddLineNumbers", &FGAddLineNumbers,
	"AddContext", &FGAddContext, "ContextLines", &FGContextLines
	);

DWORD g_dwDateAfterThis, g_dwDateBeforeThis;

bool g_bScanningLocalTime;
FILETIME FADateBeforeThisLocal;
FILETIME FADateAfterThisLocal;

CParameterSet g_FAParamSet(NULL, 2, 20,
	"FullFileName", &FAFullFileName, "DirectoryName", &FADirectoryName,

	"FullFileNameMatch", &FAFullFileNameMatch, "CaseSensitive", &FACaseSensitive, "FullFileNameInverse", &FAFullFileNameInverse,
	"DirectoryMatch", &FADirectoryMatch, "DirectoryCaseSensitive", &FADirectoryCaseSensitive, "DirectoryInverse", &FADirectoryInverse, "RecursionLevel", &FARecursionLevel,
	"DateAfter", &FADateAfter, "DateAfterThis", &g_dwDateAfterThis, "DateBefore", &FADateBefore, "DateBeforeThis", &g_dwDateBeforeThis, "ModificationDate", &FAModificationDate,
	"SizeGreater", &FASizeGreater, "SizeGreaterLimit", &FASizeGreaterLimit, "SizeLess", &FASizeLess, "SizeLessLimit", &FASizeLessLimit, "SearchHead", &FASearchHead, "SearchHeadLimit", &FASearchHeadLimit,
	"AttributesCleared", &FAAttributesCleared, "AttributesSet", &FAAttributesSet
	);

void FReadRegistry(HKEY Key) {
	#define DECLARE_PERSIST_LOAD Key
	#include "PersistVars.h"

	FSPresets = new CFSPresetCollection(g_FSParamSet);
	FRPresets = new CFRPresetCollection(g_FRParamSet);
	FGPresets = new CFGPresetCollection(g_FGParamSet);
	FAPresets = new CFAPresetCollection(g_FAParamSet);

	CharTableSet2 Table;

	while (StartupInfo.CharTable(XLatTables.size(), (char *)&Table, sizeof(CharTableSet))>=0) {
		memmove(Table.UpperDecodeTable, Table.DecodeTable, 256);
		for (int I=0;I<256;I++) Table.UpperDecodeTable[I]=UpCaseTable[Table.UpperDecodeTable[I]];

		XLatTables.push_back(Table);
	}

	// Generating "ANSI" table
	strcpy(Table.TableName, "(ANSI)");
	for (int I=0;I<256;I++) {
		Table.DecodeTable[I] = Table.EncodeTable[I] = I;
		Table.UpperTable[I] = (int)CharUpper((LPSTR)I);
		Table.LowerTable[I] = (int)CharLower((LPSTR)I);
	}
	CharToOemBuff((LPCSTR)Table.DecodeTable, (LPSTR)Table.DecodeTable, 256);
	OemToCharBuff((LPCSTR)Table.EncodeTable, (LPSTR)Table.EncodeTable, 256);
	memmove(Table.UpperDecodeTable, Table.DecodeTable, 256);
	for (int I=0;I<256;I++) Table.UpperDecodeTable[I]=UpCaseTable[Table.UpperDecodeTable[I]];

	XLatTables.push_back(Table);

	g_WhereToSearch.Append(GetMsg(MAllDrives));
	g_WhereToSearch.Append(GetMsg(MAllLocalDrives));
	g_WhereToSearch.Append(GetMsg(MFromRoot));
	g_WhereToSearch.Append(GetMsg(MFromCurrent));
	g_WhereToSearch.Append(GetMsg(MCurrentOnly));
	g_WhereToSearch.Append(GetMsg(MSelected));

	g_WhereToSearchPlugin.Append(GetMsg(MFromCurrent));
	g_WhereToSearchPlugin.Append(GetMsg(MCurrentOnly));
	g_WhereToSearchPlugin.Append(GetMsg(MSelected));
}

void FWriteRegistry(HKEY Key) {
	#define DECLARE_PERSIST_SAVE Key
	#include "PersistVars.h"
}

void FCleanup(BOOL PatternOnly) {
	if (FPattern) {pcre_free(FPattern);FPattern=NULL;}
	if (FPatternExtra) {pcre_free(FPatternExtra);FPatternExtra=NULL;}
	if (FMaskPattern) {pcre_free(FMaskPattern);FMaskPattern=NULL;}
	if (FMaskPatternExtra) {pcre_free(FMaskPatternExtra);FMaskPatternExtra=NULL;}
	if (FMaskSet) {delete FMaskSet;FMaskPatternExtra=NULL;}
	if (FASystemFoldersMask) {delete FASystemFoldersMask;FASystemFoldersMask=NULL;}

	if (!PatternOnly) {
		if (FAFullFileNamePattern) {pcre_free(FAFullFileNamePattern);FAFullFileNamePattern=NULL;}
		if (FAFullFileNamePatternExtra) {pcre_free(FAFullFileNamePatternExtra);FAFullFileNamePatternExtra=NULL;}

		delete FSPresets;
		delete FRPresets;
		delete FGPresets;
		delete FAPresets;
	}
}

int FPrepareMaskPattern() {
	FCleanup(TRUE);

	if (FAdvanced && !CompileAdvancedSettings()) return FALSE;
	if (FASkipSystemFolders) {
		if (FASystemFoldersMask) delete FASystemFoldersMask;

		FASystemFoldersMask = new CFarMaskSet(FASystemFolders.c_str());
		if (!FASystemFoldersMask->Valid()) {
			delete FASystemFoldersMask; FASystemFoldersMask = NULL;
//			return FALSE;
		}
	}

	if (FMaskAsRegExp) {
		if (!PreparePattern(&FMaskPattern,&FMaskPatternExtra,FMask,FALSE)) return FALSE;
	} else {
		FMaskSet = new CFarMaskSet(FMask.c_str());
		if (!FMaskSet->Valid()) {
			delete FMaskSet; FMaskSet = NULL;
			return FALSE;
		}
	}

	return TRUE;
}

int FPreparePattern(bool bAcceptEmpty) {
	if (!FPrepareMaskPattern()) return FALSE;

	if (FText.empty() && !bAcceptEmpty) return FALSE;

	bool bPlainText = (FSearchAs==SA_PLAINTEXT)||(FSearchAs==SA_MULTITEXT);
	if (!CheckUsage(FText, !bPlainText, FALSE/*FSearchAs == SA_SEVERALLINE*/)) return FALSE;

	if (bPlainText) {
		FTextUpcase = (FCaseSensitive) ? FText : UpCaseString(FText);
	}

	switch (FSearchAs) {
	case SA_PLAINTEXT:
		PrepareBMHSearch(FTextUpcase.data(), FTextUpcase.length());
		return TRUE;

	case SA_MULTITEXT:
	case SA_MULTIREGEXP:{
		string What=FText;
		string Word;

		FSWords.clear();
		do {
			GetStripWord(What,Word);
			if (Word.size()==0) break;
			FSWords.push_back(Word);
		} while (Word.size()&&(!g_bInterrupted));
						}
		return FALSE;

	case SA_REGEXP:
	case SA_SEVERALLINE:
	case SA_MULTILINE:
		if (FText.empty()) return TRUE;
		return PreparePattern(&FPattern, &FPatternExtra, FText, FCaseSensitive, FUTF8, OEMCharTables);
	default:
		return FALSE;
	}
}

void InitFoundPosition() {
	g_nFoundLine = 0;
	g_nFoundColumn = 1;
}

void AddFile(WIN32_FIND_DATA *FindData,PluginPanelItem **PanelItems,int *ItemsNumber) {
	*PanelItems=(PluginPanelItem *)realloc(*PanelItems,sizeof(PluginPanelItem)*((*ItemsNumber)+1));
	(*PanelItems)[*ItemsNumber].FindData=*FindData;
	(*PanelItems)[*ItemsNumber].PackSizeHigh=0;
	(*PanelItems)[*ItemsNumber].PackSize=0;
	(*PanelItems)[*ItemsNumber].Flags=0;
	(*PanelItems)[*ItemsNumber].NumberOfLinks=0;
	(*PanelItems)[*ItemsNumber].Description=NULL;
	(*PanelItems)[*ItemsNumber].Owner=NULL;
	(*PanelItems)[*ItemsNumber].CustomColumnData=NULL;
	(*PanelItems)[*ItemsNumber].CustomColumnNumber=0;
	(*PanelItems)[*ItemsNumber].UserData=(DWORD)new TempUserData(g_nFoundLine, g_nFoundColumn);
	(*ItemsNumber)++;
}

int AddSlashLen(char *Directory) {
	int Len=strlen(Directory);
	if ((Len==0)||(Directory[Len-1]!='\\')) {strcat(Directory,"\\");Len++;}
	return Len;
}

BOOL MaskApplies(string &Mask,size_t Start,char *Filename,BOOL AppendAsterisk) {
	if (Start>=Mask.size()) return AppendAsterisk||(Filename[0]==0);

	switch (Mask[Start]) {
	case '?':return (Filename[0])?MaskApplies(Mask,Start+1,Filename+1,AppendAsterisk):FALSE;
	case '*':
		if (Start==Mask.size()-1) return TRUE;
		while (Filename[0]) {
			if (MaskApplies(Mask,Start+1,Filename,AppendAsterisk)) return TRUE;
			Filename++;
		}
		return FALSE;
	default:{
		BOOL Same=(FACaseSensitive)?(Mask[Start]==Filename[0]):(_memicmp(Mask.data()+Start,Filename,1)==0);
		return (Same)?MaskApplies(Mask,Start+1,Filename+1,AppendAsterisk):FALSE;
			}
	}
}

BOOL MultipleMasksApply(const string &Masks, const char *Filename) {
	BOOL Result=FALSE,AnyPositive=FALSE;

	if (FMaskAsRegExp) {
		return do_pcre_exec(FMaskPattern,FMaskPatternExtra,Filename,strlen(Filename),0,0,NULL,NULL)>=0;
	} else {
		return (*FMaskSet)(Filename);
	}
}

void ShortenFileName(const char *szFrom, char *szTo) {
	int nLength = strlen(szFrom);
	if (nLength <= 74) {
		strcpy(szTo, szFrom);
		memset(szTo+nLength, ' ', 74-nLength);
		szTo[74] = 0;
	} else {
		const char *szName = strrchr(szFrom, '\\');
		if (!szName) szName = szFrom;
		if ((nLength = strlen(szName)) > 74) {
			strcpy(szTo, szName+nLength-74);
		} else {
			strncpy(szTo, szFrom, 74-nLength);
			szTo[71-nLength] = '.';
			szTo[72-nLength] = '.';
			szTo[73-nLength] = '.';
			strcpy(szTo+74-nLength, szName);
		}
	}
}

void ShowProgress(char *Directory, PluginPanelItem *PanelItems, int ItemsNumber) {
	char Scanned[80],Found[80];

	sprintf(Scanned,GetMsg(MFilesScanned),FilesScanned);
	sprintf(Found,GetMsg(MFilesFound),ItemsNumber);

	char szFileName[15][75];
	const char *Lines[20]={GetMsg(MRESearch), Directory, Scanned, Found, "",
		szFileName[ 0], szFileName[ 1], szFileName[ 2], szFileName[ 3], szFileName[ 4], 
		szFileName[ 5], szFileName[ 6], szFileName[ 7], szFileName[ 8], szFileName[ 9],
		szFileName[10], szFileName[11], szFileName[12], szFileName[13], szFileName[14]};

	int nMax = (ItemsNumber > 15) ? 15 : ItemsNumber;
	for (int nItem = 0; nItem < nMax; nItem++)
		ShortenFileName(PanelItems[nItem+ItemsNumber-nMax].FindData.cFileName, szFileName[nItem]);

	StartupInfo.Message(StartupInfo.ModuleNumber,0,NULL,Lines,5+nMax,0);
}

BOOL AdvancedApplies(WIN32_FIND_DATA *FindData) {
	SYSTEMTIME st1, st2;
	FileTimeToSystemTime(&FindData->ftLastWriteTime, &st1);
	FileTimeToSystemTime(&FADateAfterThis, &st2);

	if (FADateBefore&&(CompareFileTime(
		(FAModificationDate) ? &FindData->ftLastWriteTime : &FindData->ftCreationTime,
		g_bScanningLocalTime ? &FADateBeforeThisLocal : &FADateBeforeThis) > 0)) return FALSE;
	if (FADateAfter&&(CompareFileTime(
		(FAModificationDate) ? &FindData->ftLastWriteTime : &FindData->ftCreationTime,
		g_bScanningLocalTime ? &FADateAfterThisLocal : &FADateAfterThis) < 0)) return FALSE;

	if (FASizeLess && (FindData->nFileSizeLow >= FASizeLessLimit)) return FALSE;
	if (FASizeGreater && (FindData->nFileSizeLow <= FASizeGreaterLimit)) return FALSE;

	if (FindData->dwFileAttributes & FAAttributesCleared) return FALSE;
	if ((FindData->dwFileAttributes & FAAttributesSet) != FAAttributesSet) return FALSE;

	if (FAFullFileNameMatch && FAFullFileNamePattern) {
		bool bNameMatches = do_pcre_exec(FAFullFileNamePattern,FAFullFileNamePatternExtra,FindData->cFileName,strlen(FindData->cFileName),0,0,NULL,0) >= 0;
		if (FAFullFileNameInverse ? bNameMatches : !bNameMatches) return FALSE;
	}

	return TRUE;
}

int DoScanDirectory(char *Directory,PluginPanelItem **PanelItems,int *ItemsNumber,ProcessFileProc ProcessFile) {
	if (FASkipSystemFolders && FASystemFoldersMask) {
		if ((*FASystemFoldersMask)(Directory)) return TRUE;
	}

	if (FAdvanced) {
		if (FARecursionLevel && (CurrentRecursionLevel > FARecursionLevel)) return TRUE;
		if (FADirectoryMatch && FADirectoryPattern && CurrentRecursionLevel) {
			bool bNameMatches = do_pcre_exec(FADirectoryPattern,FADirectoryPatternExtra,Directory,strlen(Directory),0,0,NULL,0) >= 0;
			if (FADirectoryInverse ? bNameMatches : !bNameMatches) return TRUE;
		}
	}

	WIN32_FIND_DATA FindData;
	WIN32_FIND_DATA *FindDataArray=NULL;
	int FindDataCount=0;
	HANDLE HSearch;
	int Len=AddSlashLen(Directory);
	HANDLE hScreen=StartupInfo.SaveScreen(0,0,-1,-1);
	
	char ConsoleTitle[MAX_PATH*2];
	sprintf(ConsoleTitle,GetMsg(MConsoleTitle),Directory);
	SetConsoleTitle(ConsoleTitle);

	strcat(Directory,"*");
	if ((HSearch=FindFirstFile(Directory,&FindData))!=INVALID_HANDLE_VALUE) do {
		Sleep(0);
		g_bInterrupted|=Interrupted();
		if (FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
			if (!strcmp(FindData.cFileName,".")) continue;
			if (!strcmp(FindData.cFileName,"..")) continue;
		}
		if (!MultipleMasksApply(FMask,FindData.cFileName)) continue;
		strcpy(Directory+Len,FindData.cFileName);
		strcpy(FindData.cFileName,Directory);

		FindDataArray=(WIN32_FIND_DATA *)realloc(FindDataArray,(++FindDataCount)*sizeof(WIN32_FIND_DATA));
		FindDataArray[FindDataCount-1]=FindData;
	} while (FindNextFile(HSearch,&FindData)&&!g_bInterrupted);
	FindClose(HSearch);

	Directory[Len]=0;
	if (!FindDataCount) ShowProgress(Directory,*PanelItems,*ItemsNumber);
	for (int I=0;I<FindDataCount;I++) {
		g_bInterrupted|=Interrupted();if (g_bInterrupted) break;
		if (!FAdvanced||AdvancedApplies(&FindDataArray[I])) {
			ProcessFile(&FindDataArray[I],PanelItems,ItemsNumber);
			Sleep(0);FilesScanned++;
		}
		if ((I==0)||((FText[0]==0)?(FilesScanned%100==0):(FilesScanned%25==0))) ShowProgress(Directory,*PanelItems,*ItemsNumber);
	}

	free(FindDataArray);
	StartupInfo.RestoreScreen(hScreen);
	if (FSearchIn==SI_CURRENTONLY) return TRUE;
	if (g_bInterrupted) return FALSE;

	strcpy(Directory+Len,"*");
	if ((HSearch=FindFirstFile(Directory,&FindData))==INVALID_HANDLE_VALUE) return TRUE;
	CurrentRecursionLevel++;
	do {
		Sleep(0);
		if ((FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0) continue;
		if (strcmp(FindData.cFileName,".")==0) continue;
		if (strcmp(FindData.cFileName,"..")==0) continue;
		strcpy(Directory+Len,FindData.cFileName);
		strcpy(FindData.cFileName,Directory);
		if (!DoScanDirectory(Directory,PanelItems,ItemsNumber,ProcessFile)) {
			CurrentRecursionLevel--;
			FindClose(HSearch);return FALSE;
		}
	} while (FindNextFile(HSearch,&FindData));
	CurrentRecursionLevel--;
	FindClose(HSearch);
	Directory[Len]=0;
	return TRUE;
}

int ScanPluginDirectories(PanelInfo &Info,PluginPanelItem **PanelItems,int *ItemsNumber,ProcessFileProc ProcessFile) {
	PluginPanelItem *Items=(FSearchIn==SI_SELECTED)?Info.SelectedItems:Info.PanelItems;
	int Number=(FSearchIn==SI_SELECTED)?Info.SelectedItemsNumber:Info.ItemsNumber;
	g_bScanningLocalTime = true;

	for (int I=0;I<Number;I++) {
		if (Items[I].FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
			char CurDir[MAX_PATH];
			if (strcmp(Items[I].FindData.cFileName,"..")==0) continue;
			GetFullPathName(Items[I].FindData.cFileName,MAX_PATH,CurDir,NULL);
			if (!DoScanDirectory(CurDir,PanelItems,ItemsNumber,ProcessFile)) break;
		} else {
			if (!MultipleMasksApply(FMask,Items[I].FindData.cFileName)) continue;
			g_bInterrupted|=Interrupted();if (g_bInterrupted) break;

			WIN32_FIND_DATA CurFindData=Items[I].FindData;
			GetFullPathName(Items[I].FindData.cFileName,sizeof(CurFindData.cFileName),CurFindData.cFileName,NULL);
			ProcessFile(&CurFindData,PanelItems,ItemsNumber);
			FilesScanned++;
		}
	}
	return TRUE;
}

int ScanDirectories(PluginPanelItem **PanelItems,int *ItemsNumber,ProcessFileProc ProcessFile) {
	PanelInfo PInfo;
	StartupInfo.Control(INVALID_HANDLE_VALUE,FCTL_GETPANELINFO,&PInfo);
	*ItemsNumber=0;*PanelItems=NULL;g_bInterrupted=FALSE;FilesScanned=0;
	CurrentRecursionLevel=0;

	FADateBeforeThisLocal = FADateBeforeThis;
	SystemToLocalTime(FADateBeforeThisLocal);
	FADateAfterThisLocal = FADateAfterThis;
	SystemToLocalTime(FADateAfterThisLocal);

	g_bScanningLocalTime = LocalFileTime(PInfo.CurDir[0]);

	if (PInfo.Plugin) return ScanPluginDirectories(PInfo,PanelItems,ItemsNumber,ProcessFile);

	switch (FSearchIn) {
	case SI_ALLDRIVES:
	case SI_ALLLOCAL:{
		DWORD Drives=GetLogicalDrives();
		char RootDir[MAX_PATH]="A:\\";
		for (int I=0;I<32;I++) if (Drives&(1<<I)) {
			RootDir[0]='A'+I;
			UINT DriveType=GetDriveType(RootDir);
			if ((FSearchIn==SI_ALLLOCAL)&&(DriveType==DRIVE_REMOTE)) continue;
			if ((DriveType!=0)&&(DriveType!=1)&&(DriveType!=DRIVE_REMOVABLE)&&(DriveType!=DRIVE_CDROM))
				g_bScanningLocalTime = LocalFileTime(RootDir[0]);
				DoScanDirectory(RootDir,PanelItems,ItemsNumber,ProcessFile);
				RootDir[3]=0;
		}
		return TRUE;
					 }
	case SI_FROMROOT:PInfo.CurDir[3]=0;
	case SI_FROMCURRENT:case SI_CURRENTONLY:
		DoScanDirectory(PInfo.CurDir,PanelItems,ItemsNumber,ProcessFile);return TRUE;
	case SI_SELECTED:{
			int Len=AddSlashLen(PInfo.CurDir);
			if (PInfo.ItemsNumber==0) return FALSE;

			for (int I=0;I<PInfo.SelectedItemsNumber;I++) {
				WIN32_FIND_DATA CurFindData=PInfo.SelectedItems[I].FindData;
				if (CurFindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
					if (strcmp(CurFindData.cFileName,"..")) {
						// Test if directory itself applies
						if (MultipleMasksApply(FMask,CurFindData.cFileName)) {
							strcat(strcpy(CurFindData.cFileName,PInfo.CurDir),PInfo.SelectedItems[I].FindData.cFileName);
							ProcessFile(&CurFindData,PanelItems,ItemsNumber);FilesScanned++;
						}

						// Scan subdirectory
						strcpy(PInfo.CurDir+Len,PInfo.SelectedItems[I].FindData.cFileName);
						if (!DoScanDirectory(PInfo.CurDir,PanelItems,ItemsNumber,ProcessFile)) break;
						PInfo.CurDir[Len]=0;
					}
				} else {
					if (!MultipleMasksApply(FMask,CurFindData.cFileName)) continue;
					g_bInterrupted|=Interrupted();if (g_bInterrupted) break;
					strcat(strcpy(CurFindData.cFileName,PInfo.CurDir),PInfo.SelectedItems[I].FindData.cFileName);
					ProcessFile(&CurFindData,PanelItems,ItemsNumber);FilesScanned++;
				}
			}
			return TRUE;
					 }
	}
	return TRUE;
}

OperationResult NoFilesFound() {
	const char *Lines[]={GetMsg(MRESearch),GetMsg(MNoFilesFound),GetMsg(MOk)};
	StartupInfo.Message(StartupInfo.ModuleNumber,0,"NoFilesFound",Lines,3,1);
	return OR_OK;
}

BOOL ConfirmFile(int Title,const char *FileName) {
	if (FileConfirmed) return TRUE;
	const char *Lines[]={
		GetMsg(Title),GetMsg(MConfirmRequest),FileName,GetMsg(MOk),GetMsg(MAll),GetMsg(MSkip),GetMsg(MCancel)
	};
	switch (StartupInfo.Message(StartupInfo.ModuleNumber,0,"FRConfirmFile",Lines,7,4)) {
	case 1:FRConfirmFileThisRun=FALSE;
	case 0:return (FileConfirmed=TRUE);
	case -1:
	case 3:g_bInterrupted=TRUE;
	}
	return FALSE;
}

void SkipNoCRLF(const char *&Buffer,int *Size) {
	if (Size) {
		while ((*Size)&&(*Buffer!=0x0D)&&(*Buffer!=0x0A)) {Buffer++;(*Size)--;}
	} else {
		while ((*Buffer!=0x0D)&&(*Buffer!=0x0A)) Buffer++;
	}
}

void SkipCRLF(const char *&Buffer,int *Size) {
	if (Size) {
		if ((*Size)&&(*Buffer==0x0D)) {Buffer++;(*Size)--;}
		if ((*Size)&&(*Buffer==0x0A)) {Buffer++;(*Size)--;}
	} else {
		if (*Buffer==0x0D) Buffer++;
		if (*Buffer==0x0A) Buffer++;
	}
}

void SkipWholeLine(const char *&Buffer,int *Size) {
	SkipNoCRLF(Buffer,Size);
	SkipCRLF(Buffer,Size);
}

wchar_t LE(const char *Buffer) {
	return (wchar_t)(Buffer[0] + (Buffer[1] << 8));
}

wchar_t BE(const char *Buffer) {
	return (wchar_t)(Buffer[1] + (Buffer[0] << 8));
}

void SkipNoCRLF(const char *&Buffer,int *Size, eLikeUnicode nUni) {
	switch (nUni) {
	case UNI_NONE:
	case UNI_UTF8:
		SkipNoCRLF(Buffer, Size);
		break;
	case UNI_LE:
		if (Size) {
			while ((*Size >= 2) && (LE(Buffer)!=0x0D) && (LE(Buffer)!=0x0A)) {Buffer+=2;(*Size)-=2;}
		} else {
			while ((LE(Buffer)!=0x0D) && (LE(Buffer)!=0x0A)) Buffer+=2;
		}
		break;
	case UNI_BE:
		if (Size) {
			while ((*Size >= 2) && (BE(Buffer)!=0x0D) && (BE(Buffer)!=0x0A)) {Buffer+=2;(*Size)-=2;}
		} else {
			while ((BE(Buffer)!=0x0D) && (BE(Buffer)!=0x0A)) Buffer+=2;
		}
		break;
	}
}

void SkipCRLF(const char *&Buffer,int *Size, eLikeUnicode nUni) {
	switch (nUni) {
	case UNI_NONE:
	case UNI_UTF8:
		SkipCRLF(Buffer, Size);
		break;
	case UNI_LE:
		if (Size) {
			if ((*Size) && (LE(Buffer)==0x0D)) {Buffer+=2;(*Size)-=2;}
			if ((*Size) && (LE(Buffer)==0x0A)) {Buffer+=2;(*Size)-=2;}
		} else {
			if (LE(Buffer)==0x0D) Buffer+=2;
			if (LE(Buffer)==0x0A) Buffer+=2;
		}
		break;
	case UNI_BE:
		if (Size) {
			if ((*Size) && (BE(Buffer)==0x0D)) {Buffer+=2;(*Size)-=2;}
			if ((*Size) && (BE(Buffer)==0x0A)) {Buffer+=2;(*Size)-=2;}
		} else {
			if (BE(Buffer)==0x0D) Buffer+=2;
			if (BE(Buffer)==0x0A) Buffer+=2;
		}
		break;
	}
}

void SkipWholeLine(const char *&Buffer, int *Size, eLikeUnicode nUni) {
	SkipNoCRLF(Buffer, Size, nUni);
	SkipCRLF(Buffer, Size, nUni);
}

time_t FTtoTime_t(FILETIME &ft) {
	 return (long)((*((__int64 *)&ft) - 0x19DB1DED53E8000 ) / 10000000);
}

void Time_tToFT(time_t t, FILETIME &ft) {
	 *((__int64 *)&ft) = (__int64)t * 10000000 + 0x19DB1DED53E8000;
}

class CFarDateTimeStorage:public CFarStorage {
public:
	CFarDateTimeStorage(FILETIME *DT):ftDateTime(DT),   ttDateTime(NULL) {};
	CFarDateTimeStorage(time_t   *DT):ftDateTime(NULL), ttDateTime(DT) {};
	virtual void Get(char *pszBuffer, int nSize) const;
	virtual void Put(const char *pszBuffer);
	virtual bool Verify(const char *pszBuffer);
	virtual operator string() const {
		return "";
	}
protected:
	FILETIME *ftDateTime;
	time_t   *ttDateTime;
	FILETIME m_ftTemp;
};

void CFarDateTimeStorage::Get(char *pszBuffer, int nSize) const {
	SYSTEMTIME locTime;
	FILETIME ft;
	if (ftDateTime) {
		ft = *ftDateTime;
	} else {
		Time_tToFT(*ttDateTime, ft);
	}

	SystemToLocalTime(ft);
	FileTimeToSystemTime(&ft, &locTime);

	_snprintf(pszBuffer, nSize, "%02d.%02d.%04d %02d:%02d:%02d",
		locTime.wDay, locTime.wMonth, locTime.wYear, locTime.wHour, locTime.wMinute, locTime.wSecond);
}

int atoin(const char *Line,int First,int Last) {
	static char s_szBuffer[128];;
	if (First==-1) return 0;
	Line+=First;
	strncpy(s_szBuffer, Line, Last-First);
	s_szBuffer[Last-First] = 0;
	return atoi(s_szBuffer);
}

bool CFarDateTimeStorage::Verify(const char *pszBuffer) {
	const char *ErrPtr;
	int ErrOffset;
	pcre *Pattern=pcre_compile("^\\s*(\\d+)[./](\\d+)[./](\\d+)(\\s+(\\d+)(:(\\d+)(:(\\d+))?)?)?\\s*$",
		PCRE_CASELESS,&ErrPtr,&ErrOffset,NULL);
	if (!Pattern) return false;
	int Match[10*3];
	if (do_pcre_exec(Pattern,NULL,pszBuffer,strlen(pszBuffer),0,0,Match,sizeof(Match)/sizeof(int))>=0) {
		SYSTEMTIME locTime;
		locTime.wDay =    atoin(pszBuffer,Match[1*2],Match[1*2+1]);
		locTime.wMonth =  atoin(pszBuffer,Match[2*2],Match[2*2+1]);
		locTime.wYear =   atoin(pszBuffer,Match[3*2],Match[3*2+1]);
		locTime.wHour =   atoin(pszBuffer,Match[5*2],Match[5*2+1]);
		locTime.wMinute = atoin(pszBuffer,Match[7*2],Match[7*2+1]);
		locTime.wSecond = atoin(pszBuffer,Match[9*2],Match[9*2+1]);
		locTime.wMilliseconds = 0;

		SystemTimeToFileTime(&locTime, &m_ftTemp);
		LocalToSystemTime(m_ftTemp);

		pcre_free(Pattern);return true;
	} else {
		pcre_free(Pattern);return false;
	}
}

void CFarDateTimeStorage::Put(const char *pszBuffer) {
	if (ftDateTime) {
		*ftDateTime=m_ftTemp;
	} else {
		*ttDateTime = FTtoTime_t(m_ftTemp);
	}
}

BOOL AdvancedSettings() {
	CFarDialog Dialog(78,24,"AdvancedFileSearchDlg");
	Dialog.AddFrame(MAdvancedOptions);
	Dialog.Add(new CFarCheckBoxItem(5,2,0,MFullFileNameMatch,&FAFullFileNameMatch));
	Dialog.Add(new CFarEditItem(5,3,59,DIF_HISTORY,"FullPath", FAFullFileName));
	Dialog.Add(new CFarCheckBoxItem(50,2,0,MCaseSensitive,&FACaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(62,3,0,MInverse,&FAFullFileNameInverse));

	Dialog.Add(new CFarCheckBoxItem(5,4,0,MDirectoryMatch,&FADirectoryMatch));
	Dialog.Add(new CFarEditItem(5,5,59,DIF_HISTORY,"RecurseDirectory", FADirectoryName));
	Dialog.Add(new CFarCheckBoxItem(50,4,0,MCaseSensitive,&FADirectoryCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(62,5,0,MInverse,&FADirectoryInverse));
	Dialog.Add(new CFarTextItem(5,6,0,MRecursionLevel));
	Dialog.Add(new CFarEditItem(25,6,30,0,NULL,(int &)FARecursionLevel,new CFarIntegerRangeValidator(0,255)));

	Dialog.Add(new CFarCheckBoxItem(5,8,0,MDateAfter,&FADateAfter));
	Dialog.Add(new CFarEditItem(30,8,50,0,NULL,new CFarDateTimeStorage(&FADateAfterThis)));
	Dialog.Add(new CFarButtonItem(52,8,0,FALSE,MCurrent));
	Dialog.Add(new CFarCheckBoxItem(5,9,0,MDateBefore,&FADateBefore));
	Dialog.Add(new CFarEditItem(30,9,50,0,NULL,new CFarDateTimeStorage(&FADateBeforeThis)));
	Dialog.Add(new CFarButtonItem(52,9,0,FALSE,MCurrent));
	Dialog.Add(new CFarRadioButtonItem(5,10,0,MCreationDate,&FAModificationDate,FALSE));
	Dialog.Add(new CFarRadioButtonItem(30,10,0,MModificationDate,&FAModificationDate,TRUE));

	Dialog.Add(new CFarCheckBoxItem(5,12,0,MSizeGreater,&FASizeGreater));
	Dialog.Add(new CFarEditItem(32,12,40,0,NULL,
		CFarIntegerStorage(FASizeGreaterLimit, &CFarSizeConverter::Instance),
		new CFarIntegerRangeValidator(0,0x7FFFFFFF)));
	Dialog.Add(new CFarCheckBoxItem(42,12,0,MSizeLess,&FASizeLess));
	Dialog.Add(new CFarEditItem(56,12,64,0,NULL,
		CFarIntegerStorage(FASizeLessLimit, &CFarSizeConverter::Instance),
		new CFarIntegerRangeValidator(0,0x7FFFFFFF)));
	Dialog.Add(new CFarCheckBoxItem(5,13,0,MSearchHead,&FASearchHead));
	Dialog.Add(new CFarEditItem(32,13,40,0,NULL,
		CFarIntegerStorage(FASearchHeadLimit, &CFarSizeConverter::Instance),
		new CFarIntegerRangeValidator(0,0x7FFFFFFF)));

	Dialog.Add(new CFarTextItem(5,15,0,MAttributes));
	Dialog.Add(new CFarCheckBox3Item(7,16,DIF_3STATE,MDirectory,&FAAttributesCleared,&FAAttributesSet,FILE_ATTRIBUTE_DIRECTORY));
	Dialog.Add(new CFarCheckBox3Item(7,17,DIF_3STATE,MReadOnly,&FAAttributesCleared,&FAAttributesSet,FILE_ATTRIBUTE_READONLY));
	Dialog.Add(new CFarCheckBox3Item(7,18,DIF_3STATE,MArchive,&FAAttributesCleared,&FAAttributesSet,FILE_ATTRIBUTE_ARCHIVE));
	Dialog.Add(new CFarCheckBox3Item(27,16,DIF_3STATE,MHidden,&FAAttributesCleared,&FAAttributesSet,FILE_ATTRIBUTE_HIDDEN));
	Dialog.Add(new CFarCheckBox3Item(27,17,DIF_3STATE,MSystem,&FAAttributesCleared,&FAAttributesSet,FILE_ATTRIBUTE_SYSTEM));
	Dialog.Add(new CFarCheckBox3Item(47,16,DIF_3STATE,MCompressed,&FAAttributesCleared,&FAAttributesSet,FILE_ATTRIBUTE_COMPRESSED));
	Dialog.Add(new CFarCheckBox3Item(47,17,DIF_3STATE,MEncrypted,&FAAttributesCleared,&FAAttributesSet,FILE_ATTRIBUTE_ENCRYPTED));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.AddButton(MBtnPresets);
	Dialog.SetFocus(2);

	int Result;
	do {
		Result=Dialog.Display(4,-3,13,16,-1);
		switch (Result) {
		case 0:
			break;
		case 1:
		case 2:{
			SYSTEMTIME CurTime;
			GetSystemTime(&CurTime);
			SystemTimeToFileTime(&CurTime,(Result==2) ? &FADateBeforeThis : &FADateAfterThis);
			continue;
			  }
		case 3:
			g_dwDateAfterThis = (DWORD)FTtoTime_t(FADateAfterThis);
			g_dwDateBeforeThis = (DWORD)FTtoTime_t(FADateBeforeThis);
			if (FAPresets->ShowMenu(true) >= 0) {
				Time_tToFT(g_dwDateAfterThis, FADateAfterThis);
				Time_tToFT(g_dwDateBeforeThis, FADateBeforeThis);
			}
			continue;
		default:
			return FALSE;
		}

	} while (!CompileAdvancedSettings() || (Result != 0));
	return TRUE;
}

BOOL CompileAdvancedSettings() {
	if (FAFullFileNamePattern) {pcre_free(FAFullFileNamePattern);FAFullFileNamePattern=NULL;}
	if (FAFullFileNamePatternExtra) {pcre_free(FAFullFileNamePatternExtra);FAFullFileNamePatternExtra=NULL;}
	if (FADirectoryPattern) {pcre_free(FADirectoryPattern);FADirectoryPattern=NULL;}
	if (FADirectoryPatternExtra) {pcre_free(FADirectoryPatternExtra);FADirectoryPatternExtra=NULL;}

	if (FAFullFileNameMatch) {
		if (!PreparePattern(&FAFullFileNamePattern,&FAFullFileNamePatternExtra,FAFullFileName,FACaseSensitive)) return FALSE;
	}

	if (FADirectoryMatch) {
		if (PreparePattern(&FADirectoryPattern,&FADirectoryPatternExtra,FADirectoryName,FADirectoryCaseSensitive)) return FALSE;
	}

	return TRUE;
}

BOOL CFAPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(78,26,"FAPresetDlg");
	Dialog.AddFrame(MFAPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(20,2,70,DIF_HISTORY,"RESearch.PresetName", pPreset->Name()));

	Dialog.Add(new CFarCheckBoxItem(5,4,0,MFullFileNameMatch,&pPreset->m_mapInts["FullFileNameMatch"]));
	Dialog.Add(new CFarEditItem(5,5,59,DIF_HISTORY,"FullPath", pPreset->m_mapStrings["FullFileName"]));
	Dialog.Add(new CFarCheckBoxItem(50,4,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(62,5,0,MInverse,&pPreset->m_mapInts["FullFileNameInverse"]));

	Dialog.Add(new CFarCheckBoxItem(5,6,0,MDirectoryMatch,&pPreset->m_mapInts["DirectoryMatch"]));
	Dialog.Add(new CFarEditItem(5,7,59,DIF_HISTORY,"RecurseDirectory", pPreset->m_mapStrings["DirectoryName"]));
	Dialog.Add(new CFarCheckBoxItem(50,6,0,MCaseSensitive,&pPreset->m_mapInts["DirectoryCaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(62,7,0,MInverse,&pPreset->m_mapInts["DirectoryInverse"]));
	Dialog.Add(new CFarTextItem(5,8,0,MRecursionLevel));
	Dialog.Add(new CFarEditItem(32,8,40,0,NULL,&pPreset->m_mapInts["RecursionLevel"],new CFarIntegerRangeValidator(0,255)));

	Dialog.Add(new CFarCheckBoxItem(5,10,0,MDateAfter,&pPreset->m_mapInts["DateAfter"]));
	Dialog.Add(new CFarEditItem(30,10,50,0,NULL,new CFarDateTimeStorage((time_t *)&pPreset->m_mapInts["DateAfterThis"])));
	Dialog.Add(new CFarButtonItem(52,10,0,FALSE,MCurrent));
	Dialog.Add(new CFarCheckBoxItem(5,11,0,MDateBefore,&pPreset->m_mapInts["DateBefore"]));
	Dialog.Add(new CFarEditItem(30,11,50,0,NULL,new CFarDateTimeStorage((time_t *)&pPreset->m_mapInts["DateBeforeThis"])));
	Dialog.Add(new CFarButtonItem(52,11,0,FALSE,MCurrent));
	Dialog.Add(new CFarRadioButtonItem(5,12,0,MCreationDate,&pPreset->m_mapInts["ModificationDate"],FALSE));
	Dialog.Add(new CFarRadioButtonItem(30,12,0,MModificationDate,&pPreset->m_mapInts["ModificationDate"],TRUE));

	Dialog.Add(new CFarCheckBoxItem(5,14,0,MSizeGreater,&pPreset->m_mapInts["SizeGreater"]));
	Dialog.Add(new CFarEditItem(32,14,40,0,NULL,
		CFarIntegerStorage(FASizeGreaterLimit, &CFarSizeConverter::Instance),
		new CFarIntegerRangeValidator(0,0x7FFFFFFF)));
	Dialog.Add(new CFarCheckBoxItem(42,14,0,MSizeLess,&pPreset->m_mapInts["SizeLess"]));
	Dialog.Add(new CFarEditItem(56,14,64,0,NULL,
		CFarIntegerStorage(FASizeLessLimit, &CFarSizeConverter::Instance),
		new CFarIntegerRangeValidator(0,0x7FFFFFFF)));
	Dialog.Add(new CFarCheckBoxItem(5,15,0,MSearchHead,&pPreset->m_mapInts["SearchHead"]));
	Dialog.Add(new CFarEditItem(32,15,40,0,NULL,
		CFarIntegerStorage(FASearchHeadLimit, &CFarSizeConverter::Instance),
		new CFarIntegerRangeValidator(0,0x7FFFFFFF)));

	Dialog.Add(new CFarTextItem(5,17,0,MAttributes));
	Dialog.Add(new CFarCheckBox3Item(7,18,DIF_3STATE,MDirectory,	(DWORD *)&pPreset->m_mapInts["AttributesCleared"],(DWORD *)&pPreset->m_mapInts["AttributesSet"],FILE_ATTRIBUTE_DIRECTORY));
	Dialog.Add(new CFarCheckBox3Item(7,19,DIF_3STATE,MReadOnly,		(DWORD *)&pPreset->m_mapInts["AttributesCleared"],(DWORD *)&pPreset->m_mapInts["AttributesSet"],FILE_ATTRIBUTE_READONLY));
	Dialog.Add(new CFarCheckBox3Item(7,20,DIF_3STATE,MArchive,		(DWORD *)&pPreset->m_mapInts["AttributesCleared"],(DWORD *)&pPreset->m_mapInts["AttributesSet"],FILE_ATTRIBUTE_ARCHIVE));
	Dialog.Add(new CFarCheckBox3Item(27,18,DIF_3STATE,MHidden,		(DWORD *)&pPreset->m_mapInts["AttributesCleared"],(DWORD *)&pPreset->m_mapInts["AttributesSet"],FILE_ATTRIBUTE_HIDDEN));
	Dialog.Add(new CFarCheckBox3Item(27,19,DIF_3STATE,MSystem,		(DWORD *)&pPreset->m_mapInts["AttributesCleared"],(DWORD *)&pPreset->m_mapInts["AttributesSet"],FILE_ATTRIBUTE_SYSTEM));
	Dialog.Add(new CFarCheckBox3Item(47,18,DIF_3STATE,MCompressed,	(DWORD *)&pPreset->m_mapInts["AttributesCleared"],(DWORD *)&pPreset->m_mapInts["AttributesSet"],FILE_ATTRIBUTE_COMPRESSED));
	Dialog.Add(new CFarCheckBox3Item(47,19,DIF_3STATE,MEncrypted,	(DWORD *)&pPreset->m_mapInts["AttributesCleared"],(DWORD *)&pPreset->m_mapInts["AttributesSet"],FILE_ATTRIBUTE_ENCRYPTED));
	Dialog.AddButtons(MOk,MCancel);
	Dialog.SetFocus(2);

	do {
		int Result=Dialog.Display(3,-2,15,18);
		switch (Result) {
		case 0:
			return TRUE;
		case 1:
		case 2:{
			SYSTEMTIME stCurTime;
			FILETIME ftCurTime;
			GetLocalTime(&stCurTime);
			SystemTimeToFileTime(&stCurTime,&ftCurTime);
			if (Result==1) pPreset->m_mapInts["DateAfterThis"] = (int)FTtoTime_t(ftCurTime);
					else pPreset->m_mapInts["DateBeforeThis"] = (int)FTtoTime_t(ftCurTime);
			continue;
			  }
		default:
			return FALSE;
		}
	} while (true);
}

BOOL MaskCaseHere() {
	switch (FMaskCase) {
	case MC_SENSITIVE:return TRUE;
	case MC_INSENSITIVE:return FALSE;
	case MC_VOLUME:{
		DWORD Flags;
		char szFSName[64];
		if (!GetVolumeInformation(NULL,NULL,0,NULL,NULL,&Flags,szFSName,sizeof(szFSName))) return FALSE;
		return _stricmp(szFSName, "NTFS") && (Flags&FS_CASE_SENSITIVE);
				   }
	}
	return FALSE;
}

bool LocalFileTime(char cDrive) {
	char szRoot[] = {cDrive, ':', '\\', 0};
	DWORD Flags;
	char szFSName[64];
	if (!GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, &Flags, szFSName, sizeof(szFSName))) return false;

	if (_stricmp(szFSName, "NTFS") == 0) return false;
	if (_strnicmp(szFSName, "FAT", 3) == 0) return true;
	if (_stricmp(szFSName, "CDFS") == 0) return true;
	if (_stricmp(szFSName, "UDF") == 0) return true;
	return true;
}
