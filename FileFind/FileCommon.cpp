#include "FileFind.h"

CParameterBatch g_FSBatch(2, 5,
	 "Mask", &MaskText, "Text", &SearchText,
	"MaskAsRegExp", &FMaskAsRegExp, "CaseSensitive", &FCaseSensitive,
	"UTF8", &FUTF8, "SearchAs", &FSearchAs, "IsInverse", &FSInverse
	);
CParameterBatch g_FRBatch(3, 4,
	"Mask", &MaskText, "Text", &SearchText, "Replace", &ReplaceText,
	"MaskAsRegExp", &FMaskAsRegExp, "CaseSensitive", &FCaseSensitive,
	"UTF8", &FUTF8, "SearchAs", &FSearchAs
	);

DWORD g_dwDateAfterThis, g_dwDateBeforeThis;

CParameterBatch g_FABatch(2, 20,
	"FullFileName", &FAFullFileName, "DirectoryName", &FADirectoryName,

	"FullFileNameMatch", &FAFullFileNameMatch, "CaseSensitive", &FACaseSensitive, "FullFileNameInverse", &FAFullFileNameInverse,
	"DirectoryMatch", &FADirectoryMatch, "DirectoryCaseSensitive", &FADirectoryCaseSensitive, "DirectoryInverse", &FADirectoryInverse, "RecursionLevel", &FARecursionLevel,
	"DateAfter", &FADateAfter, "DateAfterThis", &g_dwDateAfterThis, "DateBefore", &FADateBefore, "DateBeforeThis", &g_dwDateBeforeThis, "ModificationDate", &FAModificationDate,
	"SizeGreater", &FASizeGreater, "SizeGreaterLimit", &FASizeGreaterLimit, "SizeLess", &FASizeLess, "SizeLessLimit", &FASizeLessLimit, "SearchHead", &FASearchHead, "SearchHeadLimit", &FASearchHeadLimit,
	"AttributesCleared", &FAAttributesCleared, "AttributesSet", &FAAttributesSet
	);

void FReadRegistry(HKEY Key) {
	QueryRegIntValue(Key,"FCaseSensitive",&FCaseSensitive,0,0,1);
	QueryRegIntValue(Key,"FSearchAs",(int *)&FSearchAs,0,SA_PLAINTEXT,SA_MULTIREGEXP);
	QueryRegIntValue(Key,"FSearchIn",(int *)&FSearchIn,2,SI_ALLDRIVES,SI_SELECTED);
	QueryRegStringValue(Key,"FMask",FMask,"*.*");
	QueryRegIntValue(Key,"FMaskAsRegExp",&FMaskAsRegExp,0,0,1);
	QueryRegIntValue(Key,"FMaskCase",(int *)&FMaskCase,MC_VOLUME,0,2);
	QueryRegIntValue(Key,"FRReplaceReadonly",(int *)&FRReplaceReadonly,1,0,2);
	QueryRegStringValue(Key,"FText",FText,"");

	QueryRegIntValue(Key,"FSInverse",&FSInverse,0,0,1);
	QueryRegIntValue(Key,"FAllCharTables",&FAllCharTables,0,0,1);
	QueryRegIntValue(Key,"FROpenModified",&FROpenModified,0,0,1);
	QueryRegIntValue(Key,"FRConfirmFile",&FRConfirmFile,0,0,1);
	QueryRegIntValue(Key,"FRConfirmLine",&FRConfirmLine,0,0,1);
	QueryRegIntValue(Key,"FRSaveOriginal",&FRSaveOriginal,0,0,1);
	QueryRegIntValue(Key,"FROverwriteBackup",&FROverwriteBackup,0,0,1);
	QueryRegStringValue(Key,"FRReplace",FRReplace,"");
	QueryRegIntValue(Key,"FRepeating",&FRepeating,0,0,1);

	QueryRegBoolValue(Key,"FAFullFileNameMatch",&FAFullFileNameMatch,FALSE);
	QueryRegStringValue(Key,"FAFullFileName",FAFullFileName,"");
	QueryRegBoolValue(Key,"FAFullFileNameInverse",&FAFullFileNameInverse,FALSE);
	QueryRegBoolValue(Key,"FADirectoryMatch",&FADirectoryMatch,FALSE);
	QueryRegStringValue(Key,"FADirectoryName",FADirectoryName,"");
	QueryRegBoolValue(Key,"FADirectoryInverse",&FADirectoryInverse,FALSE);
	QueryRegIntValue(Key,"FARecursionLevel",&FARecursionLevel,0,0,255);

	QueryRegIntValue(Key,"FADateBefore",&FADateBefore,0,0,1);
	QueryRegIntValue(Key,"FADateBeforeLo",(int *)&FADateBeforeThis.dwLowDateTime,0xE1D58000);
	QueryRegIntValue(Key,"FADateBeforeHi",(int *)&FADateBeforeThis.dwHighDateTime,0x01A8E79F);
	QueryRegIntValue(Key,"FADateAfter",&FADateAfter,0,0,1);
	QueryRegIntValue(Key,"FADateAfterLo",(int *)&FADateAfterThis.dwLowDateTime,0xE1D58000);
	QueryRegIntValue(Key,"FADateAfterHi",(int *)&FADateAfterThis.dwHighDateTime,0x01A8E79F);
	QueryRegIntValue(Key,"FAModificationDate",&FAModificationDate,1,0,1);
	QueryRegIntValue(Key,"FASizeLess",&FASizeLess,0,0,1);
	QueryRegIntValue(Key,"FASizeLessLimit",(int *)&FASizeLessLimit,0,0);
	QueryRegIntValue(Key,"FASizeGreater",&FASizeGreater,0,0,1);
	QueryRegIntValue(Key,"FASizeGreaterLimit",(int *)&FASizeGreaterLimit,0,0);
	QueryRegIntValue(Key,"FASearchHead",&FASearchHead,0,0,1);
	QueryRegIntValue(Key,"FASearchHeadLimit",(int *)&FASearchHeadLimit,0,0);
	QueryRegIntValue(Key,"FAAttributesCleared",&FAAttributesCleared,0,0);
	QueryRegIntValue(Key,"FAAttributesSet",&FAAttributesSet,0,0);

	FSPresets=new CFSPresetCollection();
	FRPresets=new CFRPresetCollection();
	FAPresets=new CFAPresetCollection();
	FRBatch=new CPresetBatchCollection(FRPresets);

	CharTableSet2 Table;

	while (StartupInfo.CharTable(XLatTables.size(),(char *)&Table,sizeof(Table))>=0) {
		memmove(Table.UpperDecodeTable, Table.DecodeTable, 256);
		for (int I=0;I<256;I++) Table.UpperDecodeTable[I]=UpCaseTable[Table.UpperDecodeTable[I]];

		XLatTables.push_back(Table);
	}
}

void FWriteRegistry(HKEY Key) {
	SetRegIntValue(Key,"FCaseSensitive",FCaseSensitive);
	SetRegIntValue(Key,"FSearchAs",FSearchAs);
	SetRegIntValue(Key,"FSearchIn",FSearchIn);
	SetRegStringValue(Key,"FMask",FMask);
	SetRegIntValue(Key,"FMaskAsRegExp",FMaskAsRegExp);
	SetRegIntValue(Key,"FMaskCase",FMaskCase);
	SetRegIntValue(Key,"FRReplaceReadonly",FRReplaceReadonly);
	SetRegStringValue(Key,"FText",FText);

	SetRegIntValue(Key,"FSInverse",FSInverse);
	SetRegIntValue(Key,"FAllCharTables",FAllCharTables);
	SetRegIntValue(Key,"FROpenModified",FROpenModified);
	SetRegIntValue(Key,"FRConfirmFile",FRConfirmFile);
	SetRegIntValue(Key,"FRConfirmLine",FRConfirmLine);
	SetRegIntValue(Key,"FRSaveOriginal",FRSaveOriginal);
	SetRegIntValue(Key,"FROverwriteBackup",FROverwriteBackup);
	SetRegStringValue(Key,"FRReplace",FRReplace);
	SetRegIntValue(Key,"FRepeating",FRepeating);

	SetRegIntValue(Key,"FAFullFileNameMatch",FAFullFileNameMatch);
	SetRegStringValue(Key,"FAFullFileName",FAFullFileName);
	SetRegIntValue(Key,"FADateBefore",FADateBefore);
	SetRegIntValue(Key,"FADateBeforeLo",FADateBeforeThis.dwLowDateTime);
	SetRegIntValue(Key,"FADateBeforeHi",FADateBeforeThis.dwHighDateTime);
	SetRegIntValue(Key,"FADateAfter",FADateAfter);
	SetRegIntValue(Key,"FADateAfterLo",FADateAfterThis.dwLowDateTime);
	SetRegIntValue(Key,"FADateAfterHi",FADateAfterThis.dwHighDateTime);
	SetRegIntValue(Key,"FAModificationDate",FAModificationDate);
	SetRegIntValue(Key,"FASizeLess",FASizeLess);
	SetRegIntValue(Key,"FASizeLessLimit",FASizeLessLimit);
	SetRegIntValue(Key,"FASizeGreater",FASizeGreater);
	SetRegIntValue(Key,"FASizeGreaterLimit",FASizeGreaterLimit);
	SetRegIntValue(Key,"FASearchHead",FASearchHead);
	SetRegIntValue(Key,"FASearchHeadLimit",FASearchHeadLimit);
	SetRegIntValue(Key,"FAAttributesCleared",FAAttributesCleared);
	SetRegIntValue(Key,"FAAttributesSet",FAAttributesSet);
}

void FCleanup(BOOL PatternOnly) {
	if (FPattern) {pcre_free(FPattern);FPattern=NULL;}
	if (FPatternExtra) {pcre_free(FPatternExtra);FPatternExtra=NULL;}
	if (FMaskPattern) {pcre_free(FMaskPattern);FMaskPattern=NULL;}
	if (FMaskPatternExtra) {pcre_free(FMaskPatternExtra);FMaskPatternExtra=NULL;}
	if (FMaskSet) {delete FMaskSet;FMaskPatternExtra=NULL;}
	if (!PatternOnly) {
		if (FAFullFileNamePattern) {pcre_free(FAFullFileNamePattern);FAFullFileNamePattern=NULL;}
		if (FAFullFileNamePatternExtra) {pcre_free(FAFullFileNamePatternExtra);FAFullFileNamePatternExtra=NULL;}

		delete FRBatch;
		delete FSPresets;
		delete FRPresets;
		delete FAPresets;
	}
}

int FPreparePattern() {
	FCleanup(TRUE);
	if (FMaskAsRegExp) {
		if (!PreparePattern(&FMaskPattern,&FMaskPatternExtra,FMask,FALSE)) return FALSE;
	} else {
		FMaskSet = new CFarMaskSet(FMask.c_str());
		if (!FMaskSet->Valid()) {
			delete FMaskSet; FMaskSet = NULL;
			return FALSE;
		}
	}

	if ((FSearchAs==SA_PLAINTEXT)||(FSearchAs==SA_MULTITEXT)) {
		FTextUpcase=FText;
		if (!FCaseSensitive) {
			for (size_t I=0; I<FText.size(); I++)
				FTextUpcase[I] = UpCaseTable[(unsigned char)FText[I]];
		}
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
		} while (Word.size()&&(!Interrupt));
						}
		return FALSE;

	case SA_REGEXP:
	case SA_SEVERALLINE:
	case SA_MULTILINE:
	default:
		return PreparePattern(&FPattern,&FPatternExtra,FText,FCaseSensitive,FUTF8);
	}
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
	(*PanelItems)[*ItemsNumber].UserData=0;
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
	if (FADateBefore&&(CompareFileTime(
		(FAModificationDate)?&FindData->ftLastWriteTime:&FindData->ftCreationTime,
		&FADateBeforeThis)>0)) return FALSE;
	if (FADateAfter&&(CompareFileTime(
		(FAModificationDate)?&FindData->ftLastWriteTime:&FindData->ftCreationTime,
		&FADateAfterThis)<0)) return FALSE;

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
		Interrupt|=Interrupted();
		if (FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
			if (!strcmp(FindData.cFileName,".")) continue;
			if (!strcmp(FindData.cFileName,"..")) continue;
		}
		if (!MultipleMasksApply(FMask,FindData.cFileName)) continue;
		strcpy(Directory+Len,FindData.cFileName);
		strcpy(FindData.cFileName,Directory);

		FindDataArray=(WIN32_FIND_DATA *)realloc(FindDataArray,(++FindDataCount)*sizeof(WIN32_FIND_DATA));
		FindDataArray[FindDataCount-1]=FindData;
	} while (FindNextFile(HSearch,&FindData)&&!Interrupt);
	FindClose(HSearch);

	Directory[Len]=0;
	if (!FindDataCount) ShowProgress(Directory,*PanelItems,*ItemsNumber);
	for (int I=0;I<FindDataCount;I++) {
		Interrupt|=Interrupted();if (Interrupt) break;
		if (!FAdvanced||AdvancedApplies(&FindDataArray[I])) {
			ProcessFile(&FindDataArray[I],PanelItems,ItemsNumber);
			Sleep(0);FilesScanned++;
		}
		if ((I==0)||((FText[0]==0)?(FilesScanned%100==0):(FilesScanned%25==0))) ShowProgress(Directory,*PanelItems,*ItemsNumber);
	}

	free(FindDataArray);
	StartupInfo.RestoreScreen(hScreen);
	if (FSearchIn==SI_CURRENTONLY) return TRUE;
	if (Interrupt) return FALSE;

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

	for (int I=0;I<Number;I++) {
		if (Items[I].FindData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
			char CurDir[MAX_PATH];
			if (strcmp(Items[I].FindData.cFileName,"..")==0) continue;
			GetFullPathName(Items[I].FindData.cFileName,MAX_PATH,CurDir,NULL);
			if (!DoScanDirectory(CurDir,PanelItems,ItemsNumber,ProcessFile)) break;
		} else {
			if (!MultipleMasksApply(FMask,Items[I].FindData.cFileName)) continue;
			Interrupt|=Interrupted();if (Interrupt) break;

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
	*ItemsNumber=0;*PanelItems=NULL;Interrupt=FALSE;FilesScanned=0;
	CurrentRecursionLevel=0;

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
					Interrupt|=Interrupted();if (Interrupt) break;
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
	case 3:Interrupt=TRUE;
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
	SkipNoCRLF(Buffer,Size);SkipCRLF(Buffer,Size);
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
	virtual void Get(char *pszBuffer, int nSize) const {
		SYSTEMTIME Time;
		if (ftDateTime) {
			FileTimeToSystemTime(ftDateTime,&Time);
		} else {
			FILETIME ft;
			Time_tToFT(*ttDateTime, ft);
			FileTimeToSystemTime(&ft,&Time);
		}
		_snprintf(pszBuffer,nSize,"%02d.%02d.%04d %02d:%02d:%02d",Time.wDay,Time.wMonth,Time.wYear,Time.wHour,Time.wMinute,Time.wSecond);
	}
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
	Dialog.Add(new CFarEditItem(32,6,40,0,NULL,(int &)FARecursionLevel,new CFarIntegerRangeValidator(0,255)));

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
	Dialog.AddButton(MPresets);
	Dialog.SetFocus(2);

	do {
		int Result=Dialog.Display(4,-3,13,16,-1);
		switch (Result) {
		case 0:
			break;
		case 1:
		case 2:{
			SYSTEMTIME CurTime;
			GetLocalTime(&CurTime);
			SystemTimeToFileTime(&CurTime,(Result==2) ? &FADateBeforeThis : &FADateAfterThis);
			continue;
			  }
		case 3:
			g_dwDateAfterThis = FTtoTime_t(FADateAfterThis);
			g_dwDateBeforeThis = FTtoTime_t(FADateBeforeThis);
			if (FAPresets->ShowMenu(g_FABatch) >= 0) {
				Time_tToFT(g_dwDateAfterThis, FADateAfterThis);
				Time_tToFT(g_dwDateBeforeThis, FADateBeforeThis);
			}
			continue;
		default:
			return FALSE;
		}

		if (FAFullFileNamePattern) {pcre_free(FAFullFileNamePattern);FAFullFileNamePattern=NULL;}
		if (FAFullFileNamePatternExtra) {pcre_free(FAFullFileNamePatternExtra);FAFullFileNamePatternExtra=NULL;}
		if (FADirectoryPattern) {pcre_free(FADirectoryPattern);FADirectoryPattern=NULL;}
		if (FADirectoryPatternExtra) {pcre_free(FADirectoryPatternExtra);FADirectoryPatternExtra=NULL;}

		if (FAFullFileNameMatch) {
			if (!PreparePattern(&FAFullFileNamePattern,&FAFullFileNamePatternExtra,FAFullFileName,FACaseSensitive)) continue;
		}
		if (!FADirectoryMatch) break;
		if (PreparePattern(&FADirectoryPattern,&FADirectoryPatternExtra,FADirectoryName,FADirectoryCaseSensitive)) break;
	} while (TRUE);
	return TRUE;
}

BOOL CFAPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(78,26,"FAPresetDlg");
	Dialog.AddFrame(MFAPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(20,2,70,DIF_HISTORY,"RESearch.PresetName", pPreset->m_strName));

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
			if (Result==1) pPreset->m_mapInts["DateAfterThis"] = FTtoTime_t(ftCurTime);
					else pPreset->m_mapInts["DateBeforeThis"] = FTtoTime_t(ftCurTime);
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
		char szFSName[32];
		if (!GetVolumeInformation(NULL,NULL,0,NULL,NULL,&Flags,szFSName,sizeof(szFSName))) return FALSE;
		return stricmp(szFSName, "NTFS") && (Flags&FS_CASE_SENSITIVE);
				   }
	}
	return FALSE;
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
		SYSTEMTIME Time;
		Time.wDay=   atoin(pszBuffer,Match[1*2],Match[1*2+1]);
		Time.wMonth= atoin(pszBuffer,Match[2*2],Match[2*2+1]);
		Time.wYear=  atoin(pszBuffer,Match[3*2],Match[3*2+1]);
		Time.wHour=  atoin(pszBuffer,Match[5*2],Match[5*2+1]);
		Time.wMinute=atoin(pszBuffer,Match[7*2],Match[7*2+1]);
		Time.wSecond=atoin(pszBuffer,Match[9*2],Match[9*2+1]);
		Time.wMilliseconds=0;
		SystemTimeToFileTime(&Time,&m_ftTemp);
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
