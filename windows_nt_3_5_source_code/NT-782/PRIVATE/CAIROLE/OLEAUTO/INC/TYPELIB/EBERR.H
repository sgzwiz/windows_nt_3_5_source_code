/***
*C:\SILVER\D2W32ND\eberr.h - Defines error code constants
*
* THIS FILE IS AUTOMATICALLY GENERATED FROM EB.ERR
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  Defines the EBERR_XXX error codes.
*
*****************************************************************************/

#ifndef EBERR_H_INCLUDED
#define EBERR_H_INCLUDED

#define EBERR_None (USHORT)0
#define EBERR_ReturnWOGoSub (USHORT)3
#define EBERR_IllegalFuncCall (USHORT)5
#define EBERR_Overflow (USHORT)6
#define EBERR_OutOfMemory (USHORT)7
#define EBERR_OutOfBounds (USHORT)9
#define EBERR_DuplicateDefn (USHORT)10
#define EBERR_DivByZero (USHORT)11
#define EBERR_PrecisionLost (USHORT)12
#define EBERR_TypeMismatch (USHORT)13
#define EBERR_OutOfStrSpace (USHORT)14
#define EBERR_StrExprTooComplex (USHORT)16
#define EBERR_CantContinue (USHORT)17
#define EBERR_UserInterrupt (USHORT)18
#define EBERR_ResumeWOErr (USHORT)20
#define EBERR_OutOfStack (USHORT)28
#define EBERR_UNDONE (USHORT)29
#define EBERR_UndefinedProc (USHORT)35
#define EBERR_TooManyClients (USHORT)47
#define EBERR_DLLLoadErr (USHORT)48
#define EBERR_DLLBadCallingConv (USHORT)49
#define EBERR_InternalError (USHORT)51
#define EBERR_BadFileNameOrNumber (USHORT)52
#define EBERR_FileNotFound (USHORT)53
#define EBERR_BadFileMode (USHORT)54
#define EBERR_FileAlreadyOpen (USHORT)55
#define EBERR_IOError (USHORT)57
#define EBERR_FileAlreadyExists (USHORT)58
#define EBERR_BadRecordLen (USHORT)59
#define EBERR_DiskFull (USHORT)61
#define EBERR_EndOfFile (USHORT)62
#define EBERR_BadRecordNum (USHORT)63
#define EBERR_TooManyFiles (USHORT)67
#define EBERR_DevUnavailable (USHORT)68
#define EBERR_PermissionDenied (USHORT)70
#define EBERR_DiskNotReady (USHORT)71
#define EBERR_DifferentDrive (USHORT)74
#define EBERR_PathFileAccess (USHORT)75
#define EBERR_PathNotFound (USHORT)76
#define EBERR_ObjNotSet (USHORT)91
#define EBERR_IllegalFor (USHORT)92
#define EBERR_BadPatStr (USHORT)93
#define EBERR_CantUseNull (USHORT)94
#define EBERR_UserDefined (USHORT)95
#define EBERR_Abort (USHORT)287
#define EBERR_NoSuchControlOrProperty (USHORT)423
#define EBERR_NotObject (USHORT)424
#define EBERR_OLENotSupported (USHORT)430
#define EBERR_OLENoPropOrMethod (USHORT)438
#define EBERR_OLEAutomationError (USHORT)440
#define EBERR_ActionNotSupported (USHORT)445
#define EBERR_NamedArgsNotSupported (USHORT)446
#define EBERR_LocaleSettingNotSupported (USHORT)447
#define EBERR_NamedParamNotFound (USHORT)448
#define EBERR_ParameterNotOptional (USHORT)449
#define EBERR_FuncArityMismatch (USHORT)450
#define EBERR_NotEnum (USHORT)451
#define EBERR_InvalidOrdinal (USHORT)452
#define EBERR_InvalidDllFunctionName (USHORT)453
#define EBERR_CodeResourceNotFound (USHORT)454
#define EBERR_CodeResourceLockError (USHORT)455
#define EBERR_LastTrappable 455 


#define EBERR_NotYetImplemented (USHORT)32768
#define EBERR_SeekErr (USHORT)32771
#define EBERR_ReadFault (USHORT)32772
#define EBERR_WriteFault (USHORT)32773
#define EBERR_BadFunctionId (USHORT)32774
#define EBERR_FileLockViolation (USHORT)32775
#define EBERR_BufferTooSmall (USHORT)32790
#define EBERR_InvDataRead (USHORT)32792
#define EBERR_UnsupFormat (USHORT)32793
#define EBERR_RegistryAccess (USHORT)32796
#define EBERR_LibNotRegistered (USHORT)32797
#define EBERR_Usage (USHORT)32799
#define EBERR_UndefinedType (USHORT)32807
#define EBERR_QualifiedNameDisallowed (USHORT)32808
#define EBERR_ElementNotFound (USHORT)32811
#define EBERR_AmbiguousName (USHORT)32812
#define EBERR_ModNameConflict (USHORT)32813
#define EBERR_UnknownLcid (USHORT)32814
#define EBERR_BadModuleKind (USHORT)35005
#define EBERR_NoContainingLib (USHORT)35009
#define EBERR_BadTypeId (USHORT)35010
#define EBERR_BadLibId (USHORT)35011
#define EBERR_Eof (USHORT)35012
#define EBERR_SizeTooBig (USHORT)35013
#define EBERR_ExpectedFuncNotModule (USHORT)35015
#define EBERR_ExpectedFuncNotRecord (USHORT)35016
#define EBERR_ExpectedFuncNotProject (USHORT)35017
#define EBERR_ExpectedFuncNotVar (USHORT)35018
#define EBERR_ExpectedTypeNotProj (USHORT)35019
#define EBERR_UnsuitableFuncPropMatch (USHORT)35020
#define EBERR_BrokenLibRef (USHORT)35021
#define EBERR_UnsupportedTypeLibFeature (USHORT)35022
#define EBERR_ModuleAsType (USHORT)35024
#define EBERR_InvalidTypeInfoKind (USHORT)35025
#define EBERR_InvalidTypeLibFunction (USHORT)35026
#define EBERR_InvalidTypeLibVariable (USHORT)35027
#define EBERR_CompileError (USHORT)40036
#define EBERR_CantEvalWatch (USHORT)40037
#define EBERR_MissingOLB (USHORT)40038
#define EBERR_LibAlreadyAvail (USHORT)40039


#define EBERR_UserReset (USHORT)40040


#define EBERR_Syntax (USHORT)40048
#define EBERR_InvalidProcName (USHORT)40066
#define EBERR_ModuleNotFound (USHORT)40070
#define EBERR_CycleDetected (USHORT)40175
#define EBERR_OutOfHandles (USHORT)40176
#define EBERR_Eol (USHORT)40177
#define EBERR_InvalidHgnam (USHORT)40178
#define EBERR_NotAProject (USHORT)40179
#define EBERR_NotAModule (USHORT)40180
#define EBERR_NoOpenProject (USHORT)40181
#define EBERR_NoOpenModule (USHORT)40182
#define EBERR_NoBasicTypeSrc (USHORT)40183
#define EBERR_NoDynTypeMembers (USHORT)40184
#define EBERR_NonZeroArity (USHORT)40185
#define EBERR_NoPcodeTypeInfo (USHORT)40186
#define EBERR_NoExMgr (USHORT)40187
#define EBERR_ZeroLenRecordsDisallowed (USHORT)40188
#define EBERR_IndexOutOfBounds (USHORT)40189


#define EBERR_IncorrectVersion (USHORT)40190
#define EBERR_IncorrectOleVersion (USHORT)40191


#define EBERR_TooManyPanes (USHORT)40192
#define EBERR_TooManyLineConts (USHORT)40193
#define EBERR_CantSaveUndoInfo (USHORT)40196
#define EBERR_Disabled (USHORT)40197
#define EBERR_CannotEdit (USHORT)40198
#define EBERR_CompErrInHiddenModule (USHORT)40201
#define EBERR_BadSearchString (USHORT)40202
#define EBERR_EmptySearchString (USHORT)40203
#define EBERR_SrchNotFound (USHORT)40204
#define EBERR_LineTooLong (USHORT)40205
#define EBERR_NoSelection (USHORT)40206
#define EBERR_SearchTooLong (USHORT)40207
#define EBERR_SearchQuitByUser (USHORT)40208
#define EBERR_ViewProcNotProcName (USHORT)40211
#define EBERR_HelpQuitByUser (USHORT)40212
#define EBERR_NoUserAction (USHORT)40213
#define EBERR_BadWatchExpr (USHORT)40215
#define EBERR_BadInstWatchExpr (USHORT)40216
#define EBERR_EmptyWatchExpr (USHORT)40217
#define EBERR_EmptyInstWatchExpr (USHORT)40218
#define EBERR_NoBpCase (USHORT)40219
#define EBERR_WatchOutOfContext (USHORT)40220
#define EBERR_CantCompileWatchConText (USHORT)40221
#define EBERR_OOMemWatchesDeleted (USHORT)40229
#define EBERR_HiddenModule (USHORT)40230
#define EBERR_ProcInHiddenModule (USHORT)40231
#define EBERR_HostPrintingError (USHORT)40241
#define EBERR_RecordModCantChange (USHORT)40242
#define EBERR_FileNotFoundWithName (USHORT)40243
#define EBERR_CannotEditInRunMode (USHORT)40244
#define EBERR_LangMismatch (USHORT)40338
#define EBERR_CantLoadModule (USHORT)40340
#define EBERR_InvalidMe (USHORT)40347
#define EBERR_InvalidNew (USHORT)40348
#define EBERR_NoExecStmt (USHORT)40349
#define EBERR_InvalLineForNextStmt (USHORT)40350
#define EBERR_CantExecImmedInWaitMode (USHORT)40351
#define EBERR_InvalidConstValSyntax (USHORT)40354
#define EBERR_NoTimer (USHORT)57005
#define EBERR_NoMoreSrchTextFound (USHORT)57007
#define EBERR_EditCanceledByUser (USHORT)57017


#endif 
