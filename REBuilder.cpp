#include "StdAfx.h"
#include "RESearch.h"

//	Start, length
typedef map<int, int> part_map;

tstring MatchingRE(const tstring &strSource)
{
	if (CRegExp::Match(strSource, _T("^\\d+$"), 0, NULL)) return  _T("\\d+");
	if (CRegExp::Match(strSource, _T("^\\s+$"), 0, NULL)) return  _T("\\s+");
	if (CRegExp::Match(strSource, _T("^\\w+$"), 0, NULL)) return  _T("\\w+");
	if (CRegExp::Match(strSource, _T("^\\S+$"), 0, NULL)) return  _T("\\S+");
	return _T(".+");
}

tstring BuildRE(const tstring &strSource, const part_map &mapParts)
{
	tstring strResult;

	int nLeftOff = 0;
	for (part_map::const_iterator it = mapParts.begin(); it != mapParts.end(); it++) {
		tstring strStart = strSource.substr(nLeftOff, it->first-nLeftOff);
		CSO::QuoteRegExpString(strStart);
		strResult += strStart;

		tstring strPart = strSource.substr(it->first, it->second);
		tstring strRE = _T("(") + MatchingRE(strPart) + _T(")");
		strResult += strRE;

		nLeftOff = it->first + it->second;
	}

	tstring strEnd = strSource.substr(nLeftOff);
	CSO::QuoteRegExpString(strEnd);
	strResult += strEnd;

	return strResult;
}

struct sREData {
	part_map mapParts;
	int      nCurrentPart;
};

void UpdateStrings(CFarDialog *pDlg, sREData *pData)
{
	tstring strSource = pDlg->GetDlgItemText(2);
	tstring strRE = BuildRE(strSource, pData->mapParts);
	pDlg->SendDlgMessage(DM_SETTEXTPTR, 4, (LONG_PTR)strRE.data());

	pcre *re;
	if (!PreparePattern(&re, NULL, strRE, ECaseSensitive)) return;

	TREParameters ThisREParam;
	ThisREParam.AddSource(strSource.c_str(), strSource.length());
	ThisREParam.AddRE(re);

	if (pcre_exec(re, NULL, strSource.c_str(), strSource.length(), 0, 0, ThisREParam.Match(), ThisREParam.Count()) < 0) return;

	tstring strReplace = pDlg->GetDlgItemText(6);
	tstring strResult = CSO::CreateReplaceString(strReplace.c_str(), _T("\n"), -1, ThisREParam);
	pDlg->SendDlgMessage(DM_SETTEXTPTR, 8, (LONG_PTR)strResult.data());
}

LONG_PTR WINAPI REBuilderDialogProc(CFarDialog *pDlg, int nMsg, int nParam1, LONG_PTR lParam2)
{
	sREData *pData = (sREData *)pDlg->SendDlgMessage(DM_GETDLGDATA, 0, 0);

	switch (nMsg) {
	case DN_INITDIALOG:
		pDlg->SendDlgMessage(DM_SETDLGDATA, 0, lParam2);
		break;
	case DN_DRAWDLGITEM:
		if (nParam1 == 2) {
			EditorSelect Select;
#ifdef FAR3
			Select.StructSize = sizeof(EditorSelect);
#endif
			pDlg->SendDlgMessage(DM_GETSELECTION, 2, (LONG_PTR)&Select);
			pData->mapParts.erase(pData->nCurrentPart);

			if ((Select.BlockType == BTYPE_STREAM) && (Select.BlockWidth > 0)) {
				int BlockEndPos = Select.BlockStartPos + Select.BlockWidth;
				bool bNewBlock = true;
				for (part_map::const_iterator it = pData->mapParts.begin(); bNewBlock && (it != pData->mapParts.end()); it++) {
					if (Select.BlockStartPos < it->first) {
						bNewBlock = BlockEndPos <= it->first;
					} else if (Select.BlockStartPos < it->first+it->second) {
						bNewBlock = false;
					}
				}
				if (bNewBlock) {
					pData->nCurrentPart = Select.BlockStartPos;
					pData->mapParts[pData->nCurrentPart] = Select.BlockWidth;
				}
			} else {
				pData->nCurrentPart = -1;
			}

			UpdateStrings(pDlg, pData);
		}
		break;
#ifdef FAR3
	case DN_CONTROLINPUT:
		if (pDlg->GetFocus() != 2) break;
		INPUT_RECORD *record = (INPUT_RECORD *)lParam2;
		if ((record->EventType == KEY_EVENT) && (record->Event.KeyEvent.bKeyDown)) {
			if (record->Event.KeyEvent.wVirtualKeyCode == VK_RETURN) {
				pData->nCurrentPart = -1;
				UpdateStrings(pDlg, pData);
				return TRUE;
			}
			if (record->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) {
				pData->nCurrentPart = -1;
				pData->mapParts.clear();
				UpdateStrings(pDlg, pData);
				return TRUE;
			}
		}
		break;
#else
	case DN_KEY:
		if ((nParam1 == 2) && (lParam2 == KEY_ENTER)) {
			pData->nCurrentPart = -1;
			UpdateStrings(pDlg, pData);
			return TRUE;
		}
		if ((nParam1 == 2) && (lParam2 == KEY_ESC)) {
			pData->nCurrentPart = -1;
			pData->mapParts.clear();
			UpdateStrings(pDlg, pData);
			return TRUE;
		}
		break;
#endif
	}
	return pDlg->DefDlgProc(nMsg, nParam1, lParam2);
}

bool RunREBuilder(tstring &strSearch, tstring &strReplace)
{
	tstring strSource = strSearch, strRE, strResult;
	
	sREData Data;
	Data.nCurrentPart = -1;

	CFarDialog Dialog(76, 15, _T("REBuilder"));
	Dialog.SetWindowProc(REBuilderDialogProc, (LONG_PTR)&Data);
	Dialog.AddFrame(MREBuilder);
	Dialog.Add(new CFarTextItem(5, 2, 0, MRBSourceText));
	Dialog.Add(new CFarEditItem(5, 3, 70, DIF_HISTORY,_T("SearchText"), strSource));
	Dialog.Add(new CFarTextItem(5, 4, 0, MRBResultRE));
	Dialog.Add(new CFarEditItem(5, 5, 70, DIF_HISTORY|DIF_READONLY,_T("RESearch.RBResultRE"), strRE));
	Dialog.Add(new CFarTextItem(5, 6, 0, MRBReplaceText));
	Dialog.Add(new CFarEditItem(5, 7, 70, DIF_HISTORY,_T("ReplaceText"), strReplace));
	Dialog.Add(new CFarTextItem(5, 8, 0, MRBResultText));
	Dialog.Add(new CFarEditItem(5, 9, 70, DIF_HISTORY|DIF_READONLY,_T("RESearch.RBResultText"), strResult));
	Dialog.AddButtons(MOk, MCancel);

	do {
		switch (Dialog.Display(1, -2)) {
		case 0:
			strSearch = strRE;
			return true;
		case -1:
			return false;
		}
	} while (true);
}
