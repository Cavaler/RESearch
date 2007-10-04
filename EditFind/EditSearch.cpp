#include "StdAfx.h"
#include "..\RESearch.h"

void EditorSearchOK(int FirstLine,int StartPos,int LastLine,int EndPos) {
	EditorInfo EdInfo;
	StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);

	EditorSelect Select={BTYPE_STREAM,FirstLine,StartPos,EndPos-StartPos,LastLine-FirstLine+1};
	if ((Select.BlockWidth!=0)||(Select.BlockHeight>1)) StartupInfo.EditorControl(ECTL_SELECT,&Select);

	EditorSetPosition Position={(EReverse)?FirstLine:LastLine,(EReverse)?StartPos:EndPos,-1,
		TopLine(FirstLine,EdInfo.WindowSizeY,EdInfo.TotalLines),
		LeftColumn((EReverse)?StartPos:EndPos,EdInfo.WindowSizeX),-1};
	StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);
}

void PatchEditorInfo(EditorInfo &EdInfo) {
	// Skipping over selection - for "Search Again inverse"
	if (ESearchAgainCalled && (EdInfo.BlockType == BTYPE_STREAM)) {
		EditorGetString String = {EdInfo.BlockStartLine};
		StartupInfo.EditorControl(ECTL_GETSTRING, &String);
		int BlockStartPos = String.SelStart;
		while (String.SelEnd == -1) {
			String.StringNumber++;
			StartupInfo.EditorControl(ECTL_GETSTRING, &String);
		}
		int BlockEndLine = String.StringNumber;
		int BlockEndPos = String.SelEnd;

		if (EReverse) {
			if ((EdInfo.CurLine == BlockEndLine) && (EdInfo.CurPos == BlockEndPos)) {
				EdInfo.CurLine = EdInfo.BlockStartLine;
				EdInfo.CurPos = BlockStartPos;
			}
		} else {
			if ((EdInfo.CurLine == EdInfo.BlockStartLine) && (EdInfo.CurPos == BlockStartPos)) {
				EdInfo.CurLine = BlockEndLine;
				EdInfo.CurPos = BlockEndPos;
			}
		}
	}
}

BOOL EditorSearchAgain() {
	EditorInfo EdInfo;
	StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);
	PatchEditorInfo(EdInfo);

	int FirstLine,StartPos,LastLine,EndPos;

	if (EInSelection) {		// ***************** SEARCH IN SELECTION
		if (ESearchAgainCalled) {
			if (EReverse) {
				FirstLine = SelStartLine;
				StartPos = SelStartPos;

				EditorGetString String;
				String.StringNumber = LastLine = EdInfo.BlockStartLine;
				StartupInfo.EditorControl(ECTL_GETSTRING,&String);

				EndPos = String.SelEnd-1;
			} else {
				FirstLine = EdInfo.CurLine;
				StartPos = EdInfo.CurPos;
				LastLine = SelEndLine;
				EndPos = SelEndPos;
			}
		} else {
			SaveSelection();
			FirstLine = SelStartLine;
			StartPos = SelStartPos;
			LastLine = SelEndLine;
			EndPos = SelEndPos;
		}
	} else {
		if (EReverse) {
			FirstLine=0;StartPos=0;
			LastLine=EdInfo.CurLine;EndPos=EdInfo.CurPos;
		} else {
			FirstLine=EdInfo.CurLine;StartPos=EdInfo.CurPos;
			LastLine=EdInfo.TotalLines-1;EndPos=-1;
		}
	}

	if (ESeveralLine) {
		if (SearchInText(FirstLine,StartPos,LastLine,EndPos,FALSE)) {
			EditorSearchOK(FirstLine,StartPos,LastLine,EndPos);
			return TRUE;
		}
	} else {
		if (EReverse) {
			for (int Line = LastLine; !g_bInterrupted && (Line >= FirstLine); Line--) {
				int CurLastLine = Line;
				int CurStartPos = (Line == FirstLine) ? StartPos : 0;
				int CurEndPos = (Line == LastLine) ? EndPos : -1;
				if (SearchInText(Line, CurStartPos, CurLastLine, CurEndPos,FALSE)) {
					EditorSearchOK(Line, CurStartPos, CurLastLine, CurEndPos);
					return TRUE;
				}
			}
		} else {
			for (int Line = FirstLine; !g_bInterrupted && (Line<=LastLine); Line++) {
				int CurLastLine = Line;
				int CurStartPos = (Line == FirstLine) ? StartPos : 0;
				int CurEndPos = (Line == LastLine) ? EndPos : -1;
				if (SearchInText(Line, CurStartPos, CurLastLine, CurEndPos,FALSE)) {
					EditorSearchOK(Line, CurStartPos, CurLastLine, CurEndPos);
					return TRUE;
				}
			}
		}
	}

	EditorSetPosition Position;
	Position.CurLine=EdInfo.CurLine;
	Position.CurPos=EdInfo.CurPos;
	Position.CurTabPos=EdInfo.CurTabPos;
	Position.TopScreenLine=EdInfo.TopScreenLine;
	Position.LeftPos=EdInfo.LeftPos;
	Position.Overtype=EdInfo.Overtype;
	StartupInfo.EditorControl(ECTL_SETPOSITION,&Position);

	if (!g_bInterrupted) {
		const char *Lines[]={GetMsg(MRESearch),GetMsg(MCannotFind),EText.c_str(),GetMsg(MOk)};
		StartupInfo.Message(StartupInfo.ModuleNumber,FMSG_WARNING,"ECannotFind",Lines,4,1);
	}

	if (EInSelection) RestoreSelection();
	return FALSE;
}

BOOL EditorSearch() {
	EditorInfo EdInfo;
	StartupInfo.EditorControl(ECTL_GETINFO,&EdInfo);
	EInSelection=(EdInfo.BlockType!=BTYPE_NONE);

	CFarDialog Dialog(76,13,"SearchDlg");
	Dialog.AddFrame(MRESearch);
	Dialog.Add(new CFarTextItem(5,2,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,3,65,DIF_HISTORY,"SearchText",SearchText));
	Dialog.Add(new CFarButtonItem(67,3,0,0,"&\\"));

	Dialog.Add(new CFarTextItem(5,4,DIF_BOXCOLOR|DIF_SEPARATOR,""));
	Dialog.Add(new CFarCheckBoxItem(5,5,0,MRegExp,&ERegExp));
	Dialog.Add(new CFarCheckBoxItem(30,5,0,MSeveralLine,&ESeveralLine));
	Dialog.Add(new CFarCheckBoxItem(5,6,0,MCaseSensitive,&ECaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(30,6,0,"",&EUTF8));
	Dialog.Add(new CFarButtonItem(34,6,0,0,MUTF8));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MReverseSearch,&EReverse));
	if (EInSelection) Dialog.Add(new CFarCheckBoxItem(30,7,0,MInSelection,&EInSelection));
	Dialog.AddButtons(MOk, MShowAll); Dialog.AddButton(MCancel);
	Dialog.Add(new CFarButtonItem(60,5,0,0,MPresets));

	SearchText=PickupText();
	if (SearchText.empty()) SearchText=EText;
	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(5,-4,-3,3,-1,-6)) {
		case 0:
			break;
		case 1:
			// Show All
			break;
		case 2:
			if (ERegExp) QuoteRegExpString(SearchText);
			break;
		case 3:
			ESPresets->ShowMenu(g_ESBatch);
			break;
		case 4:
			UTF8Converter(SearchText);
			break;
		case -1:
			return FALSE;
		}
	} while ((ExitCode>=2)||!EPreparePattern(SearchText));

	EText=SearchText;
	g_bInterrupted=FALSE;
	if (!EText.empty()) (ExitCode == 0) ? EditorSearchAgain() : EditorListAllAgain();
	return TRUE;
}

BOOL CESPresetCollection::EditPreset(CPreset *pPreset) {
	CFarDialog Dialog(76,14,"ESPresetDlg");
	Dialog.AddFrame(MESPreset);
	Dialog.Add(new CFarTextItem(5,2,0,MPresetName));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"RESearch.PresetName",pPreset->Name()));

	Dialog.Add(new CFarTextItem(5,4,0,MSearchFor));
	Dialog.Add(new CFarEditItem(5,5,70,DIF_HISTORY,"SearchText", pPreset->m_mapStrings["Text"]));

	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&pPreset->m_mapInts["IsRegExp"]));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MCaseSensitive,&pPreset->m_mapInts["CaseSensitive"]));
	Dialog.Add(new CFarCheckBoxItem(30,7,0,MSeveralLine,&pPreset->m_mapInts["SeveralLine"]));
	Dialog.Add(new CFarCheckBoxItem(30,8,0,"",&pPreset->m_mapInts["UTF8"]));
	Dialog.Add(new CFarButtonItem(34,8,0,0,MUTF8));
	Dialog.AddButtons(MOk,MCancel);

	do {
		switch (Dialog.Display(2, -2, -3)) {
		case 0:
			return TRUE;
		case 1:{		// avoid Internal Error for icl
			string str = pPreset->m_mapStrings["SearchText"];
			UTF8Converter(str);
			break;
			  }
		default:
			return FALSE;
		}
	} while (true);
}
