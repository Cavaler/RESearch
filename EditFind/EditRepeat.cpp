#include "StdAfx.h"
#include "..\RESearch.h"

extern bool NoAsking;
extern bool bCachedReplace;

bool EditorRepeatAgain()
{
	int FirstLine = EdInfo.CurLine;
	int StartPos  = EdInfo.CurPos;

	EInSelection   = false;
	bCachedReplace = false;
	NoAsking       = true;

	EditorStartUndo();

	for (size_t nCount = 0; nCount < ERRepeatCount; nCount++)
	{
		REParam.AddSingleCharParam('N', nCount);
#ifdef UNICODE
		tstring Replace = CSO::CreateReplaceString(ERReplace.c_str(), _T("\n"), ScriptEngine(EREvaluate), REParam);
#else
		string  Replace = CSO::CreateReplaceString(ERReplace_O2E.c_str(), "\n", ScriptEngine(EREvaluate), REParam);
#endif
		if (g_bInterrupted) return false;	// Script failed

		int LastLine = FirstLine;
		int EndPos   = StartPos;
		DoEditReplace(FirstLine, StartPos, LastLine, EndPos, Replace);
		FirstLine = LastLine;
		StartPos  = EndPos;
	}

	EditorEndUndo();

	EditorSetPosition Position = {ITEM_SS(EditorSetPosition) FirstLine, StartPos, -1, TopLine(FirstLine), LeftColumn(StartPos), -1};
	EctlForceSetPosition(&Position);

	return true;
}

LONG_PTR WINAPI EditorRepeatDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2)
{
	ReplaceEOLDialogProc(nMsg, lParam2);

	int nCtlID = pDlg->GetID(nParam1);

	switch (nMsg) {
	case DN_INITDIALOG:
		break;
	}

	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

bool EditorRepeatText()
{
	EditorFillNamedParameters();

	CFarDialog Dialog(80, 13, _T("RepeatDlg"));
	Dialog.SetWindowProc(EditorRepeatDialogProc, 0);
	Dialog.SetUseID(true);

	Dialog.AddFrame(MRepeatText);
	Dialog.Add(new CFarTextItem(5, 2, 0, MTextToRepeat));
	Dialog.Add(new CFarEditItem(5, 3, 69, DIF_HISTORY|DIF_VAREDIT,_T("ReplaceText"), ReplaceText));
	Dialog.Add(new CFarButtonItem(71, 3, 0, 0, MQuoteReplace));

	Dialog.Add(new CFarTextItem(5, 4, DIF_BOXCOLOR|DIF_SEPARATOR, _T("")));

	Dialog.Add(new CFarCheckBoxItem(5, 5, 0, MEvaluateAsScript, &EREvaluate));
	Dialog.Add(new CFarComboBoxItem(35, 5, 60, 0, new CFarListData(m_lstEngines, false), new CFarEngineStorage(EREvaluateScript)));
	Dialog.Add(new CFarButtonItem(64, 5, 0, FALSE, MRunEditor));

	Dialog.Add(new CFarTextItem(5,  7, 0, MRepeatTimes));
	Dialog.Add(new CFarEditItem(20, 7, 25, 0, NULL, (int &)ERRepeatCount, new CFarIntegerRangeValidator(1, 16384)));
	Dialog.Add(new CFarTextItem(27, 7, 0, MTimes));

	Dialog.AddButtons(MOk, MCancel, MBtnClose);

	ReplaceText = PickupMultilineSelection();
	if (ReplaceText.empty()) ReplaceText = ERReplace;

	int ExitCode;
	do {
		switch (ExitCode = Dialog.Display()) {
		case MOk:
		case MBtnClose:
			break;
		case MQuoteReplace:
			CSO::QuoteReplaceString(ReplaceText);
			break;
		case MRunEditor:
			RunExternalEditor(ReplaceText);
			break;
		case -1:
			return false;
		}
	} while (((ExitCode != MOk) && (ExitCode != MBtnClose))
		|| !CompileLUAString(ReplaceText, ScriptEngine(EREvaluate)));

#ifdef UNICODE
	ERReplace = ReplaceText;
#else
	ERReplace = ERReplace_O2E = ReplaceText;
	OEMToEditor(ERReplace_O2E);
#endif

	g_bInterrupted = false;
	REParam.m_setInitParam.clear();

	if ((ExitCode != MBtnClose) && !ERReplace.empty()) EditorRepeatAgain();

	return true;
}
