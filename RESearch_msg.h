enum eStringTable {

	MZero,
	MOk,
	MCancel,
	MBtnPresets,
	MBtnAdvanced,
	MBtnBatch,
	MBtnRetry,
	MBtnSettings,
	MBtnClose,
	MError,
	MWarning,
	MQuoteSearch,
	MQuoteReplace,
	MEllipsis,
	MKB,
	MMB,

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
	MMenuShowLastResults,
	MMenuBatches,
	MMenuPreset,
	MMenuBatch,
	MUsedPreset,
	MMenuClearVariables,

	MRESearch,
	MREReplace,
	MMask,
	MAsRegExp,
	MText,
	MCaseSensitive,
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
	MFileConsoleTitle,
	MEditConsoleTitle,

	MREGrep,
	MGrepOutputNames,
	MGrepOutputLCount,
	MGrepOutputMCount,
	MGrepOutputLines,
	MGrepOutputLNumbers,
	MGrepAndContext,
	MGrepContextLines,
	MGrepMatchedLinePart,
	MGrepOutputTo,
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
	MReplaceToNew,
	MConfirmRequest,
	MInFile,
	MTheFile,
	MModifyReadonlyRequest,

	MFilterLines,
	MLeaveMatching,
	MRemoveMatching,

	MListAllLines,
	MTotalLines,

	MRenumber,

	MFileOpenError,
	MFileCreateError,
	MPathCreateError,
	MFileBackupError,
	MFileReadError,
	MFileWriteError,
	MWarnMacrosInPlainText,
	MWarnRNInSeveralLine,
	MWarnContinue,

	MCommonSettings,
	MSeveralLineSettings,
	MSeveralLinesIs,
	MLinesOr,
	MDotMatchesNewline,
	MUseSeparateThread,
	MMaxInThreadLength,
	MThreadStackMB,
	MShowUsageWarnings,
	MUseEscapesInPlainText,
	MIgnoreIdentReplace,
	MUpdateScriptEngines,

	MDefaultCP,
	MDefaultOEM,
	MDefaultANSI,
	MAllCPInclude,
	MAllCPSelect,
	MAllCPMenu,

	MFileSearchSettings,
	MBufferSize,
	MDefaultMaskCase,
	MMaskSensitive,
	MMaskInsensitive,
	MMaskVolumeDependent,
	MReplaceReadonly,
	MNever,
	MAsk,
	MAlways,
	MSkipSystemFolders,
	MUseSingleCR,
	MEditSrchAfterFile,

	MGrepSettings,
	MGrepFileNamePrepend,
	MGrepFileNameAppend,

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
	MLRSideOffset,
	MLROffsetChars,
	MTDSideOffset,
	MTDOffsetLines,
	MFindTextAtCursor,
	MNone,
	MWordOnly,
	MAnyText,
	MFindSelection,
	MAutoFindInSelection,
	MUseRealEOL,
	MPositionCursor,
	MCurPosStart,
	MCurPosMove,
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
	MInSelection,
	MRemoveEmpty,
	MRemoveNoMatch,
	MEvaluateAsScript,
	MIncrementalSearch,
	MRunEditor,
	MCannotFind,
	MErrorCreatingEngine,
	MErrorParsingText,
	MErrorLoadingTypeLib,
	MErrorExecutingScript,
	MListAllFromPreset,
	MFromCurrentPosition,

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
	MSkipFile,
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
	MCopyOf,

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
