#include "StdAfx.h"
#include "Frontends.h"
#include "Processors.h"
#include "..\RESearch.h"

bool CSearchMultiTextFrontend::Process(IBackend *pBackend)
{
	vector<tstring> arrMaybe;
	vector<tstring> arrPlus;
	vector<tstring> arrMinus;
	size_t nMaxSize = 0;

	tstring strWhat = FText;
	tstring strWord;
	do {
		GetStripWord(strWhat, strWord);
		if (strWord.empty()) break;

		if (strWord[0] == '+') arrPlus .push_back(strWord.substr(1)); else
		if (strWord[0] == '-') arrMinus.push_back(strWord.substr(1)); else arrMaybe.push_back(strWord);

		if (nMaxSize < strWord.size()) nMaxSize = strWord.size();
	} while (true);

	bool bAnyMaybesFound = arrMaybe.empty();

	do {
		for (size_t nMinus = 0; nMinus < arrMinus.size(); nMinus++) {
			if (PrepareAndFind(pBackend, arrMinus[nMinus])) return false;
		}

		for (size_t nPlus = 0; nPlus < arrPlus.size(); ) {
			if (PrepareAndFind(pBackend, arrPlus[nPlus])) {
				arrPlus.erase(arrPlus.begin()+nPlus);
			} else
				nPlus++;
		}

		if (!bAnyMaybesFound) {
			for (size_t nMaybe = 0; nMaybe < arrMaybe.size(); nMaybe++) {
				if (PrepareAndFind(pBackend, arrMaybe[nMaybe])) {
					bAnyMaybesFound = true;
					break;
				}
			}
		}

		if (arrMinus.empty() && arrPlus.empty() && bAnyMaybesFound) return true;

		if (pBackend->Last()) break;
		if (!pBackend->Move(pBackend->Size() - nMaxSize*sizeof(TCHAR))) break;

	} while (true);

	return arrPlus.empty() && bAnyMaybesFound;
}
