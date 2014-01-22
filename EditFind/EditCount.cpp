#include "StdAfx.h"
#include "..\RESearch.h"

bool EditorCountAllAgain()
{
	RefreshEditorInfo();
	RefreshEditorColorInfo();
	EctlForceSetPosition(NULL);
	StartEdInfo = EdInfo;

	CDebugTimer tm(_T("EditCountAll() took %d ms"));

	int FirstLine = 0, StartPos = 0, LastLine = EdInfo.TotalLines-1, EndPos = -1;
	int LastFoundLine = -1;

	int nFoundMatches = 0, nFoundLines = 0, nFirstLine = -1, nLastLine = -1;

	do {
		int MatchFirstLine = FirstLine, MatchStartPos = StartPos;
		int MatchLastLine = LastLine, MatchEndPos = EndPos;
		bool bNotBOL = (LastFoundLine == MatchFirstLine);

		if (!SearchInText(MatchFirstLine, MatchStartPos, MatchLastLine, MatchEndPos, false, bNotBOL)) break;

		bool ZeroMatch = (MatchFirstLine == MatchLastLine) && (MatchStartPos == MatchEndPos);

		nFoundMatches++;
		if (!bNotBOL) nFoundLines++;
		if (nFirstLine < 0) nFirstLine = MatchFirstLine;
		nLastLine = MatchFirstLine;

		FirstLine = LastFoundLine = MatchLastLine;
		StartPos = MatchEndPos + ((ZeroMatch)?1:0);
	} while (true);

	RestorePosition(StartEdInfo);
	tm.Stop();

	if (g_bInterrupted) return false;

	tstring strTotalMatches   = FormatStr(GetMsg(MTotalMatches),   nFoundMatches, nFoundLines);
	tstring strFirstMatchLine = FormatStr(GetMsg(MFirstMatchLine), nFirstLine+1);
	tstring strLastMatchLine  = FormatStr(GetMsg(MLastMatchLine),  nLastLine+1);

	if (nFoundMatches > 0) {
		const TCHAR *Lines[] = { GetMsg(MCountAllLines), strTotalMatches.c_str(), strFirstMatchLine.c_str(), strLastMatchLine.c_str(), GetMsg(MOk)};
		StartupInfo.Message(0, _T("ECountAll"), Lines, 5, 1);
	} else {
		const TCHAR *Lines[] = {GetMsg(MCountAllLines), GetMsg(MCannotFind), EText.c_str(), GetMsg(MOk)};
		StartupInfo.Message(FMSG_WARNING, _T("ECannotFind"), Lines, 4, 1);
	}

	return true;
}

