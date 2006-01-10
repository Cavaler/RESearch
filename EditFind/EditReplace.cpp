#include "EditFind.h"

BOOL NoAsking;
int  ReplaceStartLine,ReplaceNumber;
enum eReplaceResult {RR_OK,RR_SKIP,RR_CANCEL};

int LastReplaceLine,LastReplacePos;

void DoReplace(int FirstLine,int StartPos,int &LastLine,int &EndPos,char *Replace,int ReplaceLength) {
	EditorSetPosition Position={-1,-1,-1,-1,-1,-1};
	EditorGetString GetString={-1};
	int I;

	if (FirstLine < LastLine) {
		// Delete lines to be fully replaced
		Position.CurLine=FirstLine+1;
		StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
		for (I=LastLine-1; I>FirstLine; I--)
			StartupInfo.EditorControl(ECTL_DELETESTRING,NULL);
		if (LastLine>FirstLine+1) LastLine=FirstLine+1;
	}

	Position.CurLine=FirstLine;
	StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
	StartupInfo.EditorControl(ECTL_GETSTRING,&GetString);
	char *DefEOL=_strdup(GetString.StringEOL);

	// Creating full replace line
	int NewLength,OriginalEndLength;
	char *NewString;
	if (LastLine==FirstLine) {
		OriginalEndLength=GetString.StringLength-EndPos;
		NewLength=ReplaceLength+GetString.StringLength-(EndPos-StartPos);
		NewString=(char *)malloc(NewLength);
		memmove(NewString,GetString.StringText,StartPos);
		memmove(NewString+StartPos,Replace,ReplaceLength);
		memmove(NewString+StartPos+ReplaceLength,GetString.StringText+EndPos,
			GetString.StringLength-EndPos);
	} else {
		EditorGetString GetString2={-1};
		Position.CurLine=LastLine;
		StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
		StartupInfo.EditorControl(ECTL_GETSTRING,&GetString2);

		OriginalEndLength=GetString2.StringLength-EndPos;
		NewLength=ReplaceLength+StartPos+(GetString2.StringLength-EndPos);
		NewString=(char *)malloc(NewLength);
		memmove(NewString,GetString.StringText,StartPos);
		memmove(NewString+StartPos,Replace,ReplaceLength);
		memmove(NewString+StartPos+ReplaceLength,GetString2.StringText+EndPos,
			GetString2.StringLength-EndPos);

		StartupInfo.EditorControl(ECTL_DELETESTRING,NULL);
		Position.CurLine=FirstLine;
		StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
	}

	EditorInfo EdInfo;
	StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);
	EditorSetString SetString={-1,NewString,NULL,NULL};
//	Position.CurLine=EdInfo.CurLine;
	while (TRUE) {
		char *CR=(char *)memchr(SetString.StringText,'\r',NewLength);
		char *LF=(char *)memchr(SetString.StringText,'\n',NewLength);
		char *EOL,*NextLine;
		if (CR) {
			EOL=(LF==CR+1)?"\x0D\x0A":"\x0D";
			NextLine=CR+((LF==CR+1)?2:1);
		} else {
			if (!LF) break;
			CR=LF;EOL=DefEOL;
			NextLine=CR+1;
		}

		SetString.StringLength=CR-SetString.StringText;
		SetString.StringEOL=EOL;
		StartupInfo.EditorControl(ECTL_SETSTRING,&SetString);
		Position.CurPos=SetString.StringLength;
		StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
		Position.CurLine++;
		StartupInfo.EditorControl(ECTL_INSERTSTRING,NULL);
		NewLength-=NextLine-SetString.StringText;
		SetString.StringText=NextLine;
	}
	SetString.StringLength=NewLength;
	SetString.StringEOL=DefEOL;
	StartupInfo.EditorControl(ECTL_SETSTRING,&SetString);
	free(NewString);free(DefEOL);
	if (EInSelection) {
		SelEndLine+=Position.CurLine-LastLine;
		if ((SelType==BTYPE_STREAM)&&(Position.CurLine==SelEndLine)) {
			SelEndPos+=NewLength-OriginalEndLength-EndPos;
		}
	}
	LastReplaceLine=LastLine=Position.CurLine;
	LastReplacePos=EndPos=NewLength-OriginalEndLength;

	Position.CurPos=(EReverse)?StartPos:EndPos;
	Position.LeftPos=(EReverse)?-1:LeftColumn(Position.CurPos,EdInfo.WindowSizeX);
	StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
	if (!NoAsking) {
		EditorSelect Select={BTYPE_NONE};
		StartupInfo.EditorControl(ECTL_REDRAW,NULL);
		StartupInfo.EditorControl(ECTL_SELECT,&Select);
	}
	ReplaceNumber++;
}

void QuoteString(char *Source,char **&Dest,int &Count,int MaxWidth) {
	int Length=strlen(Source);
	Dest=(char **)realloc(Dest,(Count+1)*sizeof(char *));
	if (Length>MaxWidth) {
		Dest[Count]=(char *)malloc(MaxWidth+3);
		Dest[Count][0]='"';
		strncpy(Dest[Count]+1,Source,(MaxWidth-5)/2);
		strcpy(Dest[Count]+(MaxWidth-5)/2+1," ... ");
		strcat(Dest[Count]+(MaxWidth-5)/2+6,Source+Length-(MaxWidth-5)/2);
		strcat(Dest[Count],"\"");
	} else {
		Dest[Count]=(char *)malloc(Length+3);
		strcpy(Dest[Count]+1,Source);
		Dest[Count][0]=Dest[Count][Length+1]='"';Dest[Count][Length+2]=0;
	}
	Count++;
}

void QuoteStrings(char *Source,char **&Dest,int &Count,int MaxWidth) {
	do {
		char *Pos=strchr(Source,'\n');
		if (Pos) {
			*Pos=0;QuoteString(Source,Dest,Count,MaxWidth);
			*Pos='\n';Source=Pos+1;
		} else break;
	} while (TRUE);
	QuoteString(Source,Dest,Count,MaxWidth);
}

eReplaceResult EditorReplaceOK(int FirstLine,int StartPos,int &LastLine,int &EndPos,char *Original,char *Replace,char *Replace_O2E,int ReplaceLength) {
	EditorInfo EdInfo;
	StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);

	LastReplaceLine=LastLine;LastReplacePos=EndPos;
	EditorSetPosition Position={(EReverse)?FirstLine:LastLine,(EReverse)?StartPos:EndPos,-1,
		TopLine(FirstLine,EdInfo.WindowSizeY,EdInfo.TotalLines),
		LeftColumn((EReverse)?StartPos:EndPos,EdInfo.WindowSizeX),-1};
	EditorSelect Select={BTYPE_STREAM,FirstLine,StartPos,EndPos-StartPos,LastLine-FirstLine+1};

	StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
	if (!NoAsking) {
		if ((Select.BlockWidth==0)&&(Select.BlockHeight==1)) {
			Select.BlockHeight++;
			if (Select.BlockStartPos>0) {Select.BlockStartPos--;Select.BlockHeight++;}
		}
		StartupInfo.EditorControl(ECTL_SELECT,&Select);
		StartupInfo.EditorControl(ECTL_REDRAW,NULL);
	}

	int Result;
	if (NoAsking) Result=0; else {
		int Width=0;
		if (FirstLine!=LastLine) {
			char *NewOriginal=Original+StartPos;
			for (int I=0;I<LastLine-FirstLine;I++) {
				char *CR=strchr(NewOriginal,'\n');
				if (CR) {
					Width+=(CR-NewOriginal)+1;
					NewOriginal=CR+1;
				} else {
					Width+=strlen(NewOriginal)-EndPos;
					break;
				}
			}
			Width+=EndPos;
		} else Width=EndPos-StartPos;

		char **Found=NULL;
		int  FoundCount=0;
		char Save=Original[StartPos+Width];
		Original[StartPos+Width]=0;
		QuoteStrings(Original+StartPos,Found,FoundCount,EdInfo.WindowSizeX-12);
		Original[StartPos+Width]=Save;
		
		char **Replaced=NULL;
		int  ReplacedCount=0;
		QuoteStrings(Replace,Replaced,ReplacedCount,EdInfo.WindowSizeX-12);

		int Len=30,L,H,I,TotalCount=FoundCount+ReplacedCount;					// Calclulate dialog width
		for (I=0;I<FoundCount;I++) {L=strlen(Found[I]);if (L>Len) Len=L;}
		for (I=0;I<ReplacedCount;I++) {L=strlen(Replaced[I]);if (L>Len) Len=L;}
		if (Len>EdInfo.WindowSizeX-2) Len=EdInfo.WindowSizeX-2;

		L=Position.CurLine-Position.TopScreenLine;		// Calclulate dialog position
		if (L<1+EdInfo.WindowSizeY/2) {
			H=(EdInfo.WindowSizeY+L-9)/2;
		} else {
			H=(L-9)/2;
		}

		CFarDialog Dialog(-1,H+1,Len+8,H+8+TotalCount,"ERAskReplace");
		Dialog.AddFrame(MREReplace);
		Dialog.Add(new CFarTextItem(-1,2,0,MAskReplace));
		for (I=0;I<FoundCount;I++) Dialog.Add(new CFarTextItem(-1,3+I,0,Found[I]));
		Dialog.Add(new CFarTextItem(-1,3+FoundCount,0,MAskWith));
		for (I=0;I<ReplacedCount;I++) Dialog.Add(new CFarTextItem(-1,4+FoundCount+I,0,Replaced[I]));
		Dialog.Add(new CFarButtonItem(0,5+TotalCount,DIF_CENTERGROUP|DIF_NOBRACKETS,TRUE,MReplace));
		Dialog.Add(new CFarButtonItem(0,5+TotalCount,DIF_CENTERGROUP|DIF_NOBRACKETS,FALSE,MAll));
		Dialog.Add(new CFarButtonItem(0,5+TotalCount,DIF_CENTERGROUP|DIF_NOBRACKETS,FALSE,MSkip));
		Dialog.Add(new CFarButtonItem(0,5+TotalCount,DIF_CENTERGROUP|DIF_NOBRACKETS,FALSE,MCancel));
		Result=Dialog.Display(4,-4,-3,-2,-1);

		for (I=0;I<FoundCount;I++) free(Found[I]);
		for (I=0;I<ReplacedCount;I++) free(Replaced[I]);
		free(Found);free(Replaced);
	}

	switch (Result) {
	case 1:
		NoAsking=TRUE;
		Select.BlockType=BTYPE_NONE;
		StartupInfo.EditorControl(ECTL_SELECT,&Select);
	case 0:
		DoReplace(FirstLine,StartPos,LastLine,EndPos,Replace_O2E,ReplaceLength);
		return RR_OK;
	case 2:return RR_SKIP;
	default:
		NoAsking=TRUE;
		return RR_CANCEL;
	}
}

BOOL ReplaceInText(int FirstLine,int StartPos,int LastLine,int EndPos) {
	EditorInfo EdInfo;
	do {
		if (Interrupted()) return FALSE;
		int MatchFirstLine=FirstLine,MatchStartPos=StartPos;
		int MatchLastLine=LastLine,MatchEndPos=EndPos;
		if (!SearchInText(MatchFirstLine,MatchStartPos,MatchLastLine,MatchEndPos,TRUE)) return FALSE;

		// Assuming that MatchedLine starts from the needed line
		StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);

		int Numbers[3]={MatchFirstLine,MatchFirstLine-ReplaceStartLine,ReplaceNumber};
		int ReplaceLength,FoundLastLine=MatchLastLine;
		BOOL ZeroMatch=(MatchFirstLine==MatchLastLine)&&(MatchStartPos==MatchEndPos);
		char *Replace_O2E=CreateReplaceString(MatchedLine,Match,MatchCount,ERReplace_O2E.c_str(),"\n",Numbers,(EREvaluate ? EREvaluateScript : -1),ReplaceLength);
		if (Interrupt) {	// Script failed
			if (Replace_O2E) free(Replace_O2E);
			break;
		}

		char *Replace=_strdup(Replace_O2E);
		EditorToOEM(Replace, ReplaceLength);
		EditorToOEM(MatchedLine,MatchedLineLength);

		eReplaceResult Result=EditorReplaceOK(MatchFirstLine,MatchStartPos,MatchLastLine,MatchEndPos,MatchedLine,Replace,Replace_O2E,ReplaceLength);
		free(Replace);free(Replace_O2E);
		DeleteMatchInfo();

		if (Result==RR_CANCEL) return TRUE;
		if (!EReverse) LastLine+=MatchLastLine-FoundLastLine;

		if (EReverse) {
			LastLine=MatchFirstLine;EndPos=MatchStartPos-((ZeroMatch)?1:0);
		} else {
			FirstLine=MatchLastLine;StartPos=MatchEndPos+((ZeroMatch)?1:0);
		}

		int PrevLineCount=EdInfo.TotalLines;
		StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);
//		LastLine+=EdInfo.TotalLines-PrevLineCount;
	} while (TRUE);
	return FALSE;
}

BOOL ReplaceInTextByLine(int FirstLine,int StartPos,int LastLine,int EndPos,BOOL EachLineLimited) {
	int Line;
	EditorInfo EdInfo;

	Line=(EReverse)?LastLine:FirstLine;
	do {
		BOOL Matched=FALSE;
		int MatchFirstLine=Line,MatchStartPos=(Line==FirstLine)||EachLineLimited?StartPos:0;
		int MatchLastLine=Line,MatchEndPos=(Line==LastLine)||EachLineLimited?EndPos:-1;
		int FoundStartPos=MatchStartPos,FoundEndPos=MatchEndPos;

		while (SearchInText(MatchFirstLine,FoundStartPos,MatchLastLine,FoundEndPos,TRUE)) {
			if (Interrupted()) return FALSE;
			Matched=TRUE;
			// Assuming that MatchedLine starts from the needed line
			StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);

			int Numbers[3]={MatchFirstLine,MatchFirstLine-ReplaceStartLine,ReplaceNumber};
			int ReplaceLength;
			char *Replace_O2E=CreateReplaceString(MatchedLine,Match,MatchCount,ERReplace_O2E.c_str(),"\n",Numbers,(EREvaluate ? EREvaluateScript : -1),ReplaceLength);
			if (Interrupt) {	// Script failed
				if (Replace_O2E) free(Replace_O2E);
				break;
			}

			char *Replace=_strdup(Replace_O2E);
			EditorToOEM(Replace, ReplaceLength);
			EditorToOEM(MatchedLine,MatchedLineLength);

			int TailLength=MatchEndPos-FoundEndPos;
			int FoundLastLine=MatchLastLine;
			BOOL ZeroMatch=(FoundStartPos==FoundEndPos);
			eReplaceResult Result=EditorReplaceOK(MatchFirstLine,FoundStartPos,MatchLastLine,FoundEndPos,MatchedLine,Replace,Replace_O2E,ReplaceLength);
			free(Replace);free(Replace_O2E);
			DeleteMatchInfo();

			if (Result==RR_CANCEL) return TRUE;
			if (!EReverse) LastLine+=MatchLastLine-FoundLastLine;

			if (ERRemoveEmpty&&(Result==RR_OK)&&(MatchFirstLine==MatchLastLine)) {
				EditorSetPosition Position={MatchFirstLine,-1,-1,-1,-1,-1};
				StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
				EditorSetString String={-1};
				StartupInfo.EditorControl(ECTL_GETSTRING,&String);
				if (String.StringLength==0) {
					StartupInfo.EditorControl(ECTL_DELETESTRING,NULL);
					if (!EReverse) {Line--;LastLine--;}
					break;
				}
			}

			if (EReverse) {
				Line=MatchLastLine=MatchFirstLine;
				FoundEndPos=MatchEndPos=FoundStartPos-((ZeroMatch)?1:0);
				FoundStartPos=MatchStartPos-((ZeroMatch)?1:0);
			} else {
				Line=MatchFirstLine=MatchLastLine;
				FoundStartPos=MatchStartPos=FoundEndPos+((ZeroMatch)?1:0);
				FoundEndPos=MatchEndPos=(MatchEndPos==-1)?-1:FoundEndPos+TailLength;
			}
		}

		if (ERRemoveNoMatch&&!Matched) {
			EditorSetPosition Position={Line,-1,-1,-1,-1,-1};
			StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
			StartupInfo.EditorControl(ECTL_DELETESTRING,NULL);
			if (EReverse) Line--; else LastLine--;
		} else (EReverse)?Line--:Line++;
	} while (!Interrupt && ((EReverse)?Line>=FirstLine:Line<=LastLine));
	return FALSE;
}

BOOL _EditorReplaceAgain() {
	NoAsking=FALSE;
	return EditorReplaceAgain();
}

BOOL EditorReplaceAgain() {
	EditorInfo EdInfo;

	StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);
	LastReplaceLine=EdInfo.CurLine;
	LastReplacePos=EdInfo.CurPos;
	if (EInSelection) {		// ***************** REPLACE IN SELECTION
		SaveSelection();
		ReplaceStartLine=SelStartLine;
		if ((!ESeveralLine)||ERRemoveEmpty||ERRemoveNoMatch) {
			ReplaceInTextByLine(SelStartLine,SelStartPos,SelEndLine,SelEndPos, SelType == BTYPE_COLUMN);
		} else {
			ReplaceInText(SelStartLine,SelStartPos,SelEndLine,SelEndPos);
		}
		RestoreSelection();
	} else {				// ***************** PLAIN REPLACE
		int FirstLine,StartPos,LastLine,EndPos;
		if (EReverse) {
			FirstLine=0;StartPos=0;
			LastLine=EdInfo.CurLine;EndPos=EdInfo.CurPos;
			ReplaceStartLine=0;
		} else {
			FirstLine=EdInfo.CurLine;StartPos=EdInfo.CurPos;
			LastLine=EdInfo.TotalLines-1;EndPos=-1;
			ReplaceStartLine=EdInfo.CurLine;
		}

		if ((!ESeveralLine)||ERRemoveEmpty||ERRemoveNoMatch) {
			ReplaceInTextByLine(FirstLine,StartPos,LastLine,EndPos,FALSE);
		} else {
			ReplaceInText(FirstLine,StartPos,LastLine,EndPos);
		}
	}

	StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);
	EditorSetPosition Position={LastReplaceLine,LastReplacePos,-1,
		TopLine(LastReplaceLine,EdInfo.WindowSizeY,EdInfo.TotalLines),
		LeftColumn(LastReplacePos,EdInfo.WindowSizeX),-1};
	StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);

	if (NoAsking||Interrupt) return TRUE;
	ShowErrorMsg(GetMsg(MCannotFind), EText.c_str(), "ECannotFind");
	return FALSE;
}

BOOL EditorReplaceExecutor(CParameterBatch &Batch) {
	if (!EPreparePattern(SearchText)) return FALSE;

	EText = SearchText;
	ERReplace = ReplaceText;

	NoAsking = TRUE;
	ReplaceNumber = 0;

	if (EReverse) {
		EditorInfo EdInfo;
		StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);

		EditorSetPosition Position={EdInfo.TotalLines, 0, -1, -1, -1, -1};
		StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);

		EditorSetString String={-1};
		StartupInfo.EditorControl(ECTL_GETSTRING,&String);

		Position.CurPos = String.StringLength;
		StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
	} else {
		EditorSetPosition Position={0, 0, 0, -1, -1, -1};
		StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
	}

	EditorReplaceAgain();
	return TRUE;
}

BOOL EditorReplace() {
	EditorInfo EdInfo;
	StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);
	EInSelection=(EdInfo.BlockType!=BTYPE_NONE);

	CFarDialog Dialog(76,17,"ReplaceDlg");
	Dialog.AddFrame(MREReplace);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,65,DIF_HISTORY,"SearchText",SearchText));

	Dialog.Add(new CFarTextItem(5,4,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,5,65,DIF_HISTORY,"ReplaceText",ReplaceText));

	Dialog.Add(new CFarButtonItem(67,3,0,0,"&\\"));
	Dialog.Add(new CFarButtonItem(67,5,0,0,"&/"));

	Dialog.Add(new CFarTextItem(5,6,DIF_BOXCOLOR|DIF_SEPARATOR,(char *)NULL));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&ERegExp));
	Dialog.Add(new CFarCheckBoxItem(30,7,0,MSeveralLine,&ESeveralLine));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(30,8,0,"",&EUTF8));
	Dialog.Add(new CFarButtonItem(34,8,0,0,MUTF8));
	Dialog.Add(new CFarCheckBoxItem(5,9,0,MReverseSearch,&EReverse));
	if (EInSelection) Dialog.Add(new CFarCheckBoxItem(30,9,0,MInSelection,&EInSelection));
	Dialog.Add(new CFarCheckBoxItem(5,10,0,MRemoveEmpty,&ERRemoveEmpty));
	Dialog.Add(new CFarCheckBoxItem(30,10,0,MRemoveNoMatch,&ERRemoveNoMatch));
	Dialog.Add(new CFarCheckBoxItem(5,11,0,MEvaluateAsScript,&EREvaluate));
	Dialog.Add(new CFarComboBoxItem(30,11,60,0,new CFarListData(m_lstEngines, false),EREvaluateScript));

	Dialog.Add(new CFarButtonItem(0,13,DIF_CENTERGROUP,TRUE,MReplace));
	Dialog.Add(new CFarButtonItem(0,13,DIF_CENTERGROUP,FALSE,MAll));
	Dialog.Add(new CFarButtonItem(0,13,DIF_CENTERGROUP,FALSE,MCancel));
	Dialog.Add(new CFarButtonItem(60,13,0,0,MBatch));
	Dialog.Add(new CFarButtonItem(60,7,0,FALSE,MPresets));

	SearchText=PickupText();
	if (SearchText.empty()) SearchText=EText;
	ReplaceText=ERReplace;

	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(7,-5,-4,5,6,-2,-1,12)) {
		case 0:
		case 1:
			break;
		case 2:
			if (ERegExp) QuoteRegExpString(SearchText);
			break;
		case 3:
			QuoteReplaceString(ReplaceText);
			break;
		case 4:
			if (ERBatch->ShowMenu(EditorReplaceExecutor, g_ERBatch) >= 0)
				return TRUE;
			break;
		case 5:
			ERPresets->ShowMenu(g_ERBatch);
			break;
		case 6:
			UTF8Converter(SearchText);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=2)||!EPreparePattern(SearchText));
	EText=SearchText;
	ERReplace=ERReplace_O2E=ReplaceText;
	OEMToEditor(ERReplace_O2E);

	NoAsking=(ExitCode==1);ReplaceStartLine=-1;ReplaceNumber=0;
	Interrupt=FALSE;
	if (!EText.empty()) EditorReplaceAgain();
	return TRUE;
}

BOOL CERPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,17,"ERPresetDlg");
	Dialog.AddFrame(MERPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"RESearch.PresetName",pPreset->m_strName));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"SearchText",pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarTextItem(5,6,0,MReplaceWith));
	Dialog.Add(new CFarEditItem(5,7,70,DIF_HISTORY,"ReplaceText",pPreset->m_mapStrings["Replace"]));

	Dialog.Add(new CFarCheckBoxItem(5,9,0,MRegExp,&pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(5,10,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(30,9,0,MSeveralLine,&pPreset->m_mapInts["SeveralLine"]));
	Dialog.Add(new CFarCheckBoxItem(30,10,0,"",&pPreset->m_mapInts["UTF8"]));
	Dialog.Add(new CFarButtonItem(34,10,0,0,MUTF8));
	Dialog.Add(new CFarCheckBoxItem(5,11,0,MRemoveEmpty,&pPreset->m_mapInts["RemoveEmpty"]));
	Dialog.Add(new CFarCheckBoxItem(30,11,0,MRemoveNoMatch,&pPreset->m_mapInts["RemoveNoMatch"]));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(2, -2, -5)) {
		case 0:
			return TRUE;
		case 1:{		// avoid Internal Error for icl
			string str = pPreset->m_mapStrings["Text"];
			UTF8Converter(str);
			break;
			  }
		default:
			return FALSE;
		}
	} while (true);
}
