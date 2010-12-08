enum eStringTable {

	MOk,
	MCancel,
	MBtnPresets,
	MBtnAdvanced,
	MBtnBatch,
	MError,
	MWarning,

	MSearchResults,

	MF7,

	MTempUpdate,
	MTempSendFiles,

	MMenuHeader,
	MMenuSearch,
	MMenuReplace,
	MMenuGrep,
	MMenuRename,
	MMenuRenameSelected,
	MMenuRenumber,
	MMenuUndoRename,
	MMenuSelect,
	MMenuUnselect,
	MMenuFlipSelection,
	MMenuSearchAgain,
	MMenuSearchAgainRev,
	MMenuSearchReplaceAgain,
	MMenuSearchReplaceAgainRev,
	MMenuFilterText,
	MMenuTransliterate,
	MMenuUTF8Converter,
	MMenuShowLastResults,
	MMenuBatches,
	MMenuPreset,
	MMenuBatch,

	MRESearch,
	MREReplace,
	MMask,
	MAsRegExp,
	MText,
	MCaseSensitive,
	MCaseSensitiveS,
	MPlainText,
	MRegExp,
	MSeveralLineRegExp,
	MMultiLineRegExp,
	MMultiPlainText,
	MMultiRegExp,
	MSearchIn,
	MAllDrives,
	MAllLocalDrives,
	MFromRoot,
	MFromCurrent,
	MCurrentOnly,
	MSelected,
	MInverseSearch,
	MAllCharTables,
	MFilesScanned,
	MFilesFound,
	MNoFilesFound,
	MConsoleTitle,

	MREGrep,
	MGrepNames,
	MGrepNamesCount,
	MGrepLines,
	MGrepNamesLines,
	MGrepAddLineNumbers,
	MGrepAdd,
	MGrepContext,
	MGrepOutput,
	MGrepEditor,

	MAdvancedOptions,
	MFullFileNameMatch,
	MDirectoryMatch,
	MInverse,
	MRecursionLevel,
	MDateBefore,
	MDateAfter,
	MCurrent,
	MCreationDate,
	MModificationDate,
	MSizeLess,
	MSizeGreater,
	MSearchHead,
	MAttributes,
	MDirectory,
	MReadOnly,
	MArchive,
	MHidden,
	MSystem,
	MCompressed,
	MEncrypted,

	MViewModified,
	MConfirmFile,
	MConfirmLine,
	MSaveOriginal,
	MOverwriteBackup,
	MConfirmRequest,
	MInFile,
	MTheFile,
	MModifyReadonlyRequest,

	MFilterLines,
	MLeaveMatching,
	MRemoveMatching,

	MListAllLines,

	MRenumber,

	MUTF8Converter,
	MConverterANSI,
	MConverterUTF8,
	MConverterHex,
	MConvert,
	MUp,
	MDown,

	MFileOpenError,
	MFileCreateError,
	MFileBackupError,
	MFileReadError,
	MFileWriteError,
	MWarnMacrosInPlainText,
	MWarnRNInSeveralLine,
	MWarnContinue,

	MCommonSettings,
	MSeveralLinesIs,
	MLinesOr,
	MDotMatchesNewline,
	MUseSeparateThread,
	MMaxInThreadLength,
	MThreadStackMB,
	MShowUsageWarnings,
	MUseEscapesInPlainText,
	MIgnoreIdentReplace,

	MDefaultCP,
	MDefaultOEM,
	MDefaultANSI,
	MAllCPInclude,
	MAllCPSelect,
	MAllCPMenu,

	MFileSearchSettings,
	MReadAtOnceLimit,
	MKB,
	MMaskDelimiter,
	MMaskNegation,
	MAddAsterisk,
	MDefaultMaskCase,
	MMaskSensitive,
	MMaskInsensitive,
	MMaskVolumeDependent,
	MReplaceReadonly,
	MNever,
	MAsk,
	MAlways,
	MSkipSystemFolders,
	MEditSrchAfterFile,

	MRenumberSettings,
	MStripFromBeginning,
	MStripCommonPart,
	MPrefix,
	MPostfix,
	MStartFrom,
	MWidth,

	MEditorSearchSettings,
	MShowPositionOffset,
	Mfrom,
	MTop,
	MCenter,
	MBottom,
	MKeepLineIfVisible,
	MRightSideOffset,
	MFindTextAtCursor,
	MNone,
	MWordOnly,
	MAnyText,
	MFindSelection,
	MAutoFindInSelection,
	MUseRealEOL,
	MPositionCursor,
	MCurPosStart,
	MCurPosEnd,
	MCurAtNamedRef,

	MInvalidNumber,

	MRename,
	MRenameSelected,
	MSelect,
	MUnselect,
	MFlipSelection,
	MRepeating,
	MLeaveSelection,
	MPreviewRename,
	MRenamePreview,

	MSearchFor,
	MReplaceWith,
	MReverseSearch,
	MSeveralLine,
	MUTF8,
	MInSelection,
	MRemoveEmpty,
	MRemoveNoMatch,
	MEvaluateAsScript,
	MRunEditor,
	MCannotFind,
	MErrorCreatingEngine,
	MErrorParsingText,
	MErrorLoadingTypeLib,
	MErrorExecutingScript,
	MListAllFromPreset,

	MTransliterate,
	MTransSource,
	MTransTarget,

	MBtnREBuilder,
	MREBuilder,
	MRBSourceText,
	MRBResultRE,
	MRBReplaceText,
	MRBResultText,

	MAskReplace,
	MAskWith,
	MSearch,
	MReplace,
	MAll,
	MAllFiles,
	MSkip,
	MShowAll,

	MAskRename,
	MAskTo,
	MRenameError,
	MFile,
	MAskOverwrite,
	MAskCreatePath,

	MInvalidCmdLine,

	MESPreset,
	MERPreset,
	MEFPreset,
	METPreset,
	MFSPreset,
	MFRPreset,
	MFGPreset,
	MFAPreset,
	MRnPreset,
	MQRPreset,
	MVSPreset,
	MPresetName,
	MAddToMenu,
	MDeletePresetQuery,

	MPanelBatches,
	MEditorBatches,
	MBatch,
	MBatchName,
	MBtnCommands,
	MBatchCommands,

	MDeleteBatchQuery,
	MExecuteBatchQuery,

	MRegExpError,
};
