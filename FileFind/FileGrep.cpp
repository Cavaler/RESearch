#include "StdAfx.h"
#include "..\RESearch.h"

OperationResult FileGrep(BOOL ShowDialog) {
	BOOL AsRegExp = (FSearchAs == SA_REGEXP) || (FSearchAs == SA_SEVERALLINE) || (FSearchAs == SA_MULTILINE) || (FSearchAs == SA_MULTIREGEXP);

	CFarDialog Dialog(76,26,"FileGrepDlg");
	Dialog.AddFrame(MREGrep);

	Dialog.Add(new CFarCheckBoxItem(35,2,0,MAsRegExp,&FMaskAsRegExp));
	Dialog.Add(new CFarTextItem(5,2,0,MMask));
	Dialog.Add(new CFarEditItem(5,3,70,DIF_HISTORY,"Masks", MaskText));

	Dialog.Add(new CFarTextItem(5,4,0,MText));
	Dialog.Add(new CFarEditItem(5,5,65,DIF_HISTORY,"SearchText", SearchText));
	Dialog.Add(new CFarButtonItem(67,5,0,0,"&\\"));

	Dialog.Add(new CFarTextItem(5,6,DIF_BOXCOLOR|DIF_SEPARATOR,(char *)NULL));
	Dialog.Add(new CFarCheckBoxItem(5,7,0,MRegExp,&AsRegExp));
	Dialog.Add(new CFarCheckBoxItem(35,7,0,MCaseSensitive,&FCaseSensitive));
	Dialog.Add(new CFarCheckBoxItem(5,8,0,MInverseSearch,&FSInverse));
	Dialog.Add(new CFarCheckBoxItem(35,8,0,MAllCharTables,&FAllCharTables));
	Dialog.Add(new CFarTextItem(5,9,DIF_BOXCOLOR|DIF_SEPARATOR,""));

	Dialog.Add(new CFarRadioButtonItem(5,10,DIF_GROUP,MGrepNames,		(int *)&FGrepWhat,GREP_NAMES));
	Dialog.Add(new CFarRadioButtonItem(5,11,DIF_GROUP,MGrepNamesCount,	(int *)&FGrepWhat,GREP_NAMES_COUNT));
	Dialog.Add(new CFarRadioButtonItem(5,12,DIF_GROUP,MGrepLines,		(int *)&FGrepWhat,GREP_LINES));
	Dialog.Add(new CFarRadioButtonItem(5,13,DIF_GROUP,MGrepNamesLines,	(int *)&FGrepWhat,GREP_NAMES_LINES));
	Dialog.Add(new CFarCheckBoxItem(48,12,0,MGrepAddLineNumbers,&FGAddLineNumbers));

	Dialog.Add(new CFarCheckBoxItem(5,14,0,MGrepAdd,&FGAddContext));
	Dialog.Add(new CFarEditItem(15,14,20,0,NULL,(int &)FGContextLines,new CFarIntegerRangeValidator(0,1024)));
	Dialog.Add(new CFarTextItem(22,14,0,MGrepContext));

	Dialog.Add(new CFarCheckBoxItem(5,15,0,MGrepOutput,&FGOutputToFile));
	Dialog.Add(new CFarEditItem(20,15,45,DIF_HISTORY,"RESearch.GrepOutput", FGOutputFile));
	Dialog.Add(new CFarCheckBoxItem(48,15,0,MGrepEditor,&FGOpenInEditor));

	Dialog.Add(new CFarTextItem(5,16,DIF_BOXCOLOR|DIF_SEPARATOR,""));
		Dialog.Add(new CFarRadioButtonItem(5,17,DIF_GROUP,MAllDrives,(int *)&FSearchIn,SI_ALLDRIVES));
		Dialog.Add(new CFarRadioButtonItem(5,18,0,MAllLocalDrives,	(int *)&FSearchIn,SI_ALLLOCAL));
		Dialog.Add(new CFarRadioButtonItem(5,19,0,MFromRoot,		(int *)&FSearchIn,SI_FROMROOT));
		Dialog.Add(new CFarRadioButtonItem(5,20,0,MFromCurrent,		(int *)&FSearchIn,SI_FROMCURRENT));
		Dialog.Add(new CFarRadioButtonItem(5,21,0,MCurrentOnly,		(int *)&FSearchIn,SI_CURRENTONLY));
		Dialog.Add(new CFarRadioButtonItem(5,22,0,MSelected,		(int *)&FSearchIn,SI_SELECTED));

	Dialog.AddButtons(MOk,MCancel);
	Dialog.Add(new CFarButtonItem(60,17,0,0,MPresets));
	Dialog.Add(new CFarCheckBoxItem(56,19,0,"",&FAdvanced));
	Dialog.Add(new CFarButtonItem(60,19,0,0,MAdvanced));
	Dialog.Add(new CFarCheckBoxItem(56,21,0,"",&FUTF8));
	Dialog.Add(new CFarButtonItem(60,21,0,0,MUTF8));
	Dialog.SetFocus(3);
	FACaseSensitive=FADirectoryCaseSensitive=MaskCaseHere();

	MaskText=FMask;
	SearchText=FText;

	int ExitCode;
	do {
		switch (ExitCode=Dialog.Display(5,-7,6,-5,-3,-1)) {
		case 0:
			FSearchAs = AsRegExp ? SA_REGEXP : SA_PLAINTEXT;
			FMask=MaskText;
			FText=SearchText;
			break;
		case 1:
			if (AsRegExp) QuoteRegExpString(SearchText);
			break;
		case 2:
//			FSPresets->ShowMenu(g_FSBatch);
//			if (Plugin&&(FSearchIn<SI_FROMCURRENT)) FSearchIn=SI_FROMCURRENT;
			break;
		case 3:
			if (AdvancedSettings()) FAdvanced=TRUE;
			break;
		case 4:
			UTF8Converter(SearchText);
			break;
		case -1:
			return OR_CANCEL;
		}
	} while ((ExitCode>=1) || !FPreparePattern());

	return OR_OK;
}
