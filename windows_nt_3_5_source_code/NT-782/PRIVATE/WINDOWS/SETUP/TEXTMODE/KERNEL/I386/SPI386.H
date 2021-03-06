/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spi386.h

Abstract:

    x86-specific header file for text setup.

Author:

    Ted Miller (tedm) 29-October-1993

Revision History:

--*/



#ifndef _SPi386_DEFN_
#define _SPi386_DEFN_


BOOLEAN
SpLocateWin31(
    IN  PVOID         SifHandle,
    OUT PDISK_REGION *InstallRegion,
    OUT PWSTR        *InstallPath,
    OUT PDISK_REGION *SystemPartitionRegion
    );

BOOLEAN
SpConfirmWin31Path(
    VOID
    );

BOOLEAN
SpIsWin31Dir(
    IN PDISK_REGION Region,
    IN PWSTR        PathComponent,
    IN ULONG        MinKB
    );

VOID
SpCheckHpfsCColon(
    VOID
    );

VOID
SpMashemSmashem(
    IN HANDLE FileHandle, OPTIONAL
    IN PWSTR  Name1,      OPTIONAL
    IN PWSTR  Name2,      OPTIONAL
    IN PWSTR  Name3       OPTIONAL
    );

#endif // ndef _SPi386_DEFN_
