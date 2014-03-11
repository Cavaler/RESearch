#include "StdAfx.h"
#include "RESearch.h"

void BadCmdLine() {
	const TCHAR *Lines[]={GetMsg(MRESearch),GetMsg(MInvalidCmdLine),GetMsg(MOk)};
	StartupInfo.Message(FMSG_WARNING,_T("REInvalidCmdLine"),Lines,3,1);
}

bool ProcessFFLine(const TCHAR *Line, bool *ShowDialog, INT_PTR *Item)
{
	*Item = 0;
	TCHAR Switch = Line[0];
	if (Switch == 0) { *ShowDialog = true; return true; }

	Line++;
	*ShowDialog = false;

	if ((Switch==' ')||(Switch=='\t')) {
		FText=Line;
		*Item=0;
		return true;
	}

	const TCHAR *NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return false; }

	FMask = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch = _tcsrchr(Line,Switch);
	if (NextSwitch) {
		FText = tstring(Line, NextSwitch-Line);
		Line  = NextSwitch+1;
	} else {
		FText=Line;
	}

	while (NextSwitch&&*Line) {
		switch (*Line) {
		case 'c':FCaseSensitive=true;break;
		case 'C':FCaseSensitive=false;break;
		case 'i':FSInverse=true;break;
		case 'I':FSInverse=false;break;
		case 'p':case 'P':FSearchAs=SA_PLAINTEXT;break;
		case 'r':case 'R':FSearchAs=SA_REGEXP;break;
		case 's':case 'S':FSearchAs=SA_SEVERALLINE;break;
		case 'm':case 'M':FSearchAs=SA_MULTILINE;break;
		case 'a':case 'A':FSearchAs=SA_MULTITEXT;break;
		case 'l':case 'L':FSearchAs=SA_MULTIREGEXP;break;
		case 'd':*ShowDialog=true;break;
		case 'D':*ShowDialog=false;break;
		case '0':FSearchIn=SI_ALLDRIVES;break;
		case '1':FSearchIn=SI_ALLLOCAL;break;
		case '2':FSearchIn=SI_FROMROOT;break;
		case '3':FSearchIn=SI_FROMCURRENT;break;
		case '4':FSearchIn=SI_CURRENTONLY;break;
		case '5':FSearchIn=SI_SELECTED;break;
		default:BadCmdLine();return false;
		}
		Line++;
	}
	return true;
}

bool ProcessFRLine(const TCHAR *Line,bool *ShowDialog,INT_PTR *Item)
{
	*Item = 1;
	TCHAR Switch = Line[0];
	if (!Switch) { *ShowDialog = true; return true; }

	Line++;
	*ShowDialog = false;

	if ((Switch==' ')||(Switch=='\t')) {
		FText=Line;*Item=0;return true;
	}

	const TCHAR *NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return false; }
	FMask = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return false; }
	FText = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch=_tcsrchr(Line,Switch);
	if (NextSwitch) {
		FRReplace = tstring(Line, NextSwitch-Line);
		Line = NextSwitch+1;
	} else {
		FRReplace = Line;
	}

	while (NextSwitch&&*Line) {
		switch (*Line) {
		case 'c':FCaseSensitive=true;break;
		case 'C':FCaseSensitive=false;break;
		case 'p':case 'P':FSearchAs=SA_PLAINTEXT;break;
		case 'r':case 'R':FSearchAs=SA_REGEXP;break;
		case 's':case 'S':FSearchAs=SA_SEVERALLINE;break;
		case 'v':FROpenModified=true;break;
		case 'V':FROpenModified=false;break;
		case 'f':FRConfirmFile=true;break;
		case 'F':FRConfirmFile=false;break;
		case 'l':FRConfirmLine=true;break;
		case 'L':FRConfirmLine=false;break;
		case 'o':FRSaveOriginal=true;break;
		case 'O':FRSaveOriginal=false;break;
		case 'b':FROverwriteBackup=true;break;
		case 'B':FROverwriteBackup=false;break;
		case 'w':FRReplaceToNew=true;break;
		case 'W':FRReplaceToNew=false;break;
		case 'd':*ShowDialog=true;break;
		case 'D':*ShowDialog=false;break;
		case '0':FSearchIn=SI_ALLDRIVES;break;
		case '1':FSearchIn=SI_ALLLOCAL;break;
		case '2':FSearchIn=SI_FROMROOT;break;
		case '3':FSearchIn=SI_FROMCURRENT;break;
		case '4':FSearchIn=SI_CURRENTONLY;break;
		case '5':FSearchIn=SI_SELECTED;break;
		default:BadCmdLine();return false;
		}
		Line++;
	}
	return true;
}

bool ProcessRNLine(const TCHAR *Line,bool *ShowDialog,INT_PTR *Item)
{
	*Item = 7;
	TCHAR Switch = Line[0];
	if (!Switch) { *ShowDialog = true; return true; }

	Line++;
	*ShowDialog = false;

	if ((Switch==' ')||(Switch=='\t')) {
		FText=Line;
		*Item=0;
		return true;
	}

	const TCHAR *NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return false; }
	FMask = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return false; }
	FText = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch = _tcsrchr(Line,Switch);
	if (NextSwitch) {
		FRReplace = tstring(Line, NextSwitch-Line);
		Line = NextSwitch+1;
	} else {
		FRReplace = Line;
	}

	while (NextSwitch&&*Line) {
		switch (*Line) {
		case 'c':FCaseSensitive=true;break;
		case 'C':FCaseSensitive=false;break;
		case 'r':FSearchAs=SA_REGEXP;break;
		case 'R':FSearchAs=SA_PLAINTEXT;break;
		case 'p':FRepeating=true;break;
		case 'P':FRepeating=false;break;
		case 'f':FRConfirmFile=true;break;
		case 'F':FRConfirmFile=false;break;
		case 'l':FRConfirmLine=true;break;
		case 'L':FRConfirmLine=false;break;
		case 'd':*ShowDialog=true;break;
		case 'D':*ShowDialog=false;break;
		case '0':FSearchIn=SI_ALLDRIVES;break;
		case '1':FSearchIn=SI_ALLLOCAL;break;
		case '2':FSearchIn=SI_FROMROOT;break;
		case '3':FSearchIn=SI_FROMCURRENT;break;
		case '4':FSearchIn=SI_CURRENTONLY;break;
		case '5':FSearchIn=SI_SELECTED;break;
		default:BadCmdLine();return false;
		}
		Line++;
	}
	return true;
}

bool ProcessQRLine(const TCHAR *Line,bool *ShowDialog,INT_PTR *Item)
{
	*Item = 8;
	TCHAR Switch = Line[0];
	if (!Switch) { *ShowDialog = true; return true; }

	Line++;
	*ShowDialog = false;

	const TCHAR *NextSwitch = _tcschr(Line,Switch);
	if (!NextSwitch) { BadCmdLine(); return false; }
	FText = tstring(Line, NextSwitch-Line);
	Line  = NextSwitch+1;

	NextSwitch = _tcsrchr(Line,Switch);
	if (NextSwitch) {
		FRReplace = tstring(Line, NextSwitch-Line);
		Line = NextSwitch+1;
	} else {
		FRReplace=Line;
	}

	while (NextSwitch&&*Line) {
		switch (*Line) {
		case 'c':FCaseSensitive=true;break;
		case 'C':FCaseSensitive=false;break;
		case 'r':FSearchAs=SA_REGEXP;break;
		case 'R':FSearchAs=SA_PLAINTEXT;break;
		case 'p':FRepeating=true;break;
		case 'P':FRepeating=false;break;
		case 'f':FRConfirmFile=true;break;
		case 'F':FRConfirmFile=false;break;
		case 'l':FRConfirmLine=true;break;
		case 'L':FRConfirmLine=false;break;
		case 'd':*ShowDialog=true;break;
		case 'D':*ShowDialog=false;break;
		default:BadCmdLine();return false;
		}
		Line++;
	}
	return true;
}

bool ProcessCommandLine(const TCHAR *Line, bool *ShowDialog, INT_PTR *Item)
{
//	f?:/mask/findtext/options
//	f?:/mask/findtext/replacetext/options
//	f?: FindText
//	ff:/mask/findtext/options
//	fr:/mask/findtext/replacetext/options
//	rn:/mask/findtext/replacetext/options
//	qr:/findtext/replacetext/options
	if (_tcsnicmp(Line,_T("ff:"),3)==0) return ProcessFFLine(Line+3,ShowDialog,Item);
	if (_tcsnicmp(Line,_T("fr:"),3)==0) return ProcessFRLine(Line+3,ShowDialog,Item);
	if (_tcsnicmp(Line,_T("rn:"),3)==0) return ProcessRNLine(Line+3,ShowDialog,Item);
	if (_tcsnicmp(Line,_T("qr:"),3)==0) return ProcessQRLine(Line+3,ShowDialog,Item);

	TCHAR Switch=Line[0];
	if (!Switch) { BadCmdLine(); return false; }

	const TCHAR *NextSwitch;
	if ((Switch!=' ')&&(Switch!='\t')) {
		if (NextSwitch=_tcschr(Line+1,Switch))
			if (NextSwitch=_tcschr(NextSwitch+1,Switch))
				if (NextSwitch=_tcschr(NextSwitch+1,Switch)) 
					return ProcessFRLine(Line+1,ShowDialog,Item);
	}
	return ProcessFFLine(Line+1,ShowDialog,Item);
}
