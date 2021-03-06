/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    ctrlist.c

Abstract:

    Program to read the current perfmon counters and dump a list of
        objects and counters returned by the registry

Author:

    Bob Watson (a-robw) 4 Dec 92

Revision History:
    HonWah Chan May 22, 93 - added more features
    HonWah Chan Oct 18, 93 - added check for perflib version.
            Old version --> get names from registry
            New version --> get names from PerfLib thru HKEY_PERFORMANCE_DATA
--*/
#define UNICODE 1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winperf.h>

#define MAX_LEVEL 400
LPSTR DetailLevelStr[] = { "Novice", "Advanced", "Expert", "Wizard"};
// const LPWSTR lpwszDiskPerfKey = L"SYSTEM\\CurrentControlSet\\Services\\Diskperf";        
const LPWSTR NamesKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib";
const LPWSTR DefaultLangId = L"009";
const LPWSTR Counters = L"Counters";
const LPWSTR Help = L"Help";
const LPWSTR LastHelp = L"Last Help";
const LPWSTR LastCounter = L"Last Counter";
const LPWSTR Slash = L"\\";

// the following strings are for getting texts from perflib
#define  OLD_VERSION 0x010000
const LPWSTR VersionName = L"Version";
const LPWSTR CounterName = L"Counter ";
const LPWSTR HelpName = L"Explain ";


#define RESERVED    0L
#define INITIAL_SIZE     1024L
#define EXTEND_SIZE      4096L
#define LINE_LENGTH     80
#define WRAP_POINT      LINE_LENGTH-12

typedef LPVOID  LPMEMORY;
typedef HGLOBAL HMEMORY;

CHAR  PerfCounterCounter[]       = "PERF_COUNTER_COUNTER";
CHAR  PerfCounterTimer[]         = "PERF_COUNTER_TIMER";
CHAR  PerfCounterQueueLen[]      = "PERF_COUNTER_QUEUELEN_TYPE";
CHAR  PerfCounterBulkCount[]     = "PERF_COUNTER_BULK_COUNT";
CHAR  PerfCounterText[]          = "PERF_COUNTER_TEXT";
CHAR  PerfCounterRawcount[]      = "PERF_COUNTER_RAWCOUNT";
CHAR  PerfCounterLargeRawcount[] = "PERF_COUNTER_LARGE_RAWCOUNT";
CHAR  PerfSampleFraction[]       = "PERF_SAMPLE_FRACTION";
CHAR  PerfSampleCounter[]        = "PERF_SAMPLE_COUNTER";
CHAR  PerfCounterNodata[]        = "PERF_COUNTER_NODATA";
CHAR  PerfCounterTimerInv[]      = "PERF_COUNTER_TIMER_INV";
CHAR  PerfSampleBase[]           = "PERF_SAMPLE_BASE";
CHAR  PerfAverageTimer[]         = "PERF_AVERAGE_TIMER";
CHAR  PerfAverageBase[]          = "PERF_AVERAGE_BASE";
CHAR  PerfAverageBulk[]          = "PERF_AVERAGE_BULK";
CHAR  Perf100nsecTimer[]         = "PERF_100NSEC_TIMER";
CHAR  Perf100nsecTimerInv[]      = "PERF_100NSEC_TIMER_INV";
CHAR  PerfCounterMultiTimer[]    = "PERF_COUNTER_MULTI_TIMER";
CHAR  PerfCounterMultiTimerInv[] = "PERF_COUNTER_MULTI_TIMER_INV";
CHAR  PerfCounterMultiBase[]     = "PERF_COUNTER_MULTI_BASE";
CHAR  Perf100nsecMultiTimer[]    = "PERF_100NSEC_MULTI_TIMER";
CHAR  Perf100nsecMultiTimerInv[] = "PERF_100NSEC_MULTI_TIMER_INV";
CHAR  PerfRawFraction[]          = "PERF_RAW_FRACTION";
CHAR  PerfRawBase[]              = "PERF_RAW_BASE";
CHAR  PerfElapsedTime[]          = "PERF_ELAPSED_TIME";
CHAR  PerfCounterHistogramType[] = "PERF_COUNTER_HISTOGRAM_TYPE";
CHAR  NotDefineCounterType[]     = " ";

LPSTR GetCounterType(DWORD CounterType)
{
    switch (CounterType) {
         case PERF_COUNTER_COUNTER:
               return (PerfCounterCounter);
               break;

         case PERF_COUNTER_TIMER:
               return (PerfCounterTimer);
               break;

         case PERF_COUNTER_QUEUELEN_TYPE:
               return (PerfCounterQueueLen);
               break;

         case PERF_COUNTER_BULK_COUNT:
               return (PerfCounterBulkCount);
               break;

         case PERF_COUNTER_TEXT:
               return (PerfCounterText);
               break;

         case PERF_COUNTER_RAWCOUNT:
               return (PerfCounterRawcount);
               break;

         case PERF_COUNTER_LARGE_RAWCOUNT:
               return (PerfCounterLargeRawcount);
               break;

         case PERF_SAMPLE_FRACTION:
               return (PerfSampleFraction);
               break;

         case PERF_SAMPLE_COUNTER:
               return (PerfSampleCounter);
               break;

         case PERF_COUNTER_NODATA:
               return (PerfCounterNodata);
               break;

         case PERF_COUNTER_TIMER_INV:
               return (PerfCounterTimerInv);
               break;

         case PERF_SAMPLE_BASE:
               return (PerfSampleBase);
               break;

         case PERF_AVERAGE_TIMER:
               return (PerfAverageTimer);
               break;

         case PERF_AVERAGE_BASE:
               return (PerfAverageBase);
               break;

         case PERF_AVERAGE_BULK:
               return (PerfAverageBulk);
               break;

         case PERF_100NSEC_TIMER:
               return (Perf100nsecTimer);
               break;

         case PERF_100NSEC_TIMER_INV:
               return (Perf100nsecTimerInv);
               break;

         case PERF_COUNTER_MULTI_TIMER:
               return (PerfCounterMultiTimer);
               break;

         case PERF_COUNTER_MULTI_TIMER_INV:
               return (PerfCounterMultiTimerInv);
               break;

         case PERF_COUNTER_MULTI_BASE:
               return (PerfCounterMultiBase);
               break;

         case PERF_100NSEC_MULTI_TIMER:
               return (Perf100nsecMultiTimer);
               break;

         case PERF_100NSEC_MULTI_TIMER_INV:
               return (Perf100nsecMultiTimerInv);
               break;

         case PERF_RAW_FRACTION:
               return (PerfRawFraction);
               break;

         case PERF_RAW_BASE:
               return (PerfRawBase);
               break;

         case PERF_ELAPSED_TIME:
               return (PerfElapsedTime);
               break;

         case PERF_COUNTER_HISTOGRAM_TYPE:
               return (PerfCounterHistogramType);
               break;

         default:
               return (NotDefineCounterType);
               break;

    }
}


void DisplayUsage (void)
{

    printf("\nCtrList - Lists all the objects and counters installed in\n");
    printf("          the system for the given language ID\n");
    printf("\nUsage:  ctrlist [LangID] > <filename>\n");
    printf("   LangID  - 009 for English (default)\n");
    printf("           - 007 for German\n");
    printf("           - 00A for Spanish\n");
    printf("           - 00C for French\n\n");
    printf("   Example - \"ctrlist 00C > french.lst\" will get all the\n");
    printf("   objects and counters for the French system and put\n");
    printf("   them in the file french.lst\n");


    return;

} /* DisplayUsage() */


LPMEMORY
MemoryAllocate (
    DWORD dwSize
)
{  // MemoryAllocate
    HMEMORY        hMemory ;
    LPMEMORY       lpMemory ;

    hMemory = GlobalAlloc (GHND, dwSize);
    if (!hMemory)
        return (NULL);
    lpMemory = GlobalLock (hMemory);
    if (!lpMemory)
        GlobalFree (hMemory);
    return (lpMemory);
}  // MemoryAllocate


VOID
MemoryFree (
    LPMEMORY lpMemory
)
{  // MemoryFree
    HMEMORY        hMemory ;

    if (!lpMemory)
        return ;

    hMemory = GlobalHandle (lpMemory);

    if (hMemory)
        {  // if
        GlobalUnlock (hMemory);
        GlobalFree (hMemory);
        }  // if
}  // MemoryFree
   
DWORD
MemorySize (
    LPMEMORY lpMemory
)
{
    HMEMORY        hMemory ;

    hMemory = GlobalHandle (lpMemory);
    if (!hMemory)
        return (0L);

    return (GlobalSize (hMemory));
}

LPMEMORY
MemoryResize (
    LPMEMORY lpMemory,
    DWORD dwNewSize
)
{
    HMEMORY        hMemory ;
    LPMEMORY       lpNewMemory ;

    hMemory = GlobalHandle (lpMemory);
    if (!hMemory)
        return (NULL);

    GlobalUnlock (hMemory); 

    hMemory = GlobalReAlloc (hMemory, dwNewSize, GHND);

    if (!hMemory)
        return (NULL);


    lpNewMemory = GlobalLock (hMemory);

    return (lpNewMemory);
}  // MemoryResize

LPWSTR
*BuildNameTable(
    HKEY    hKeyRegistry,   // handle to registry db with counter names
    LPWSTR  lpszLangId,     // unicode value of Language subkey
    PDWORD  pdwLastItem     // size of array in elements
)
/*++
   
BuildNameTable

Arguments:

    hKeyRegistry
            Handle to an open registry (this can be local or remote.) and
            is the value returned by RegConnectRegistry or a default key.

    lpszLangId
            The unicode id of the language to look up. (default is 409)

Return Value:
     
    pointer to an allocated table. (the caller must free it when finished!)
    the table is an array of pointers to zero terminated strings. NULL is
    returned if an error occured.

--*/
{

    LPWSTR  *lpReturnValue;

    LPWSTR  *lpCounterId;
    LPWSTR  lpCounterNames;
    LPWSTR  lpHelpText;

    LPWSTR  lpThisName;

    LONG    lWin32Status;
    DWORD   dwLastError;
    DWORD   dwValueType;
    DWORD   dwArraySize;
    DWORD   dwBufferSize;
    DWORD   dwCounterSize;
    DWORD   dwHelpSize;
    DWORD   dwThisCounter;

    UNICODE_STRING  usTemp;
    
    NTSTATUS    Status;

    DWORD   dwSystemVersion;
    DWORD   dwLastId;
    DWORD   dwLastHelpId;
    
    HKEY    hKeyValue;
    HKEY    hKeyNames;

    LPWSTR  lpValueNameString;
    WCHAR   CounterNameBuffer [50];
    WCHAR   HelpNameBuffer [50];



    lpValueNameString = NULL;   //initialize to NULL
    lpReturnValue = NULL;
    hKeyValue = NULL;
    hKeyNames = NULL;
   
    // check for null arguments and insert defaults if necessary

    if (!lpszLangId) {
        lpszLangId = DefaultLangId;
    }

    // open registry to get number of items for computing array size

    lWin32Status = RegOpenKeyEx (
        hKeyRegistry,
        NamesKey,
        RESERVED,
        KEY_READ,
        &hKeyValue);
    
    if (lWin32Status != ERROR_SUCCESS) {
        goto BNT_BAILOUT;
    }

    // get number of items
    
    dwBufferSize = sizeof (dwLastHelpId);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        LastHelp,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastHelpId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }

    // get number of items
    
    dwBufferSize = sizeof (dwLastId);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        LastCounter,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwLastId,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        goto BNT_BAILOUT;
    }


    if (dwLastId < dwLastHelpId)
        dwLastId = dwLastHelpId;

    dwArraySize = dwLastId * sizeof(LPWSTR);

    // get Perflib system version
    dwBufferSize = sizeof (dwSystemVersion);
    lWin32Status = RegQueryValueEx (
        hKeyValue,
        VersionName,
        RESERVED,
        &dwValueType,
        (LPBYTE)&dwSystemVersion,
        &dwBufferSize);

    if ((lWin32Status != ERROR_SUCCESS) || (dwValueType != REG_DWORD)) {
        dwSystemVersion = OLD_VERSION;
    }

    if (dwSystemVersion == OLD_VERSION) {
        // get names from registry
        lpValueNameString = MemoryAllocate (
            lstrlen(NamesKey) * sizeof (WCHAR) +
            lstrlen(Slash) * sizeof (WCHAR) +
            lstrlen(lpszLangId) * sizeof (WCHAR) +
            sizeof (UNICODE_NULL));
        
        if (!lpValueNameString) goto BNT_BAILOUT;

        lstrcpy (lpValueNameString, NamesKey);
        lstrcat (lpValueNameString, Slash);
        lstrcat (lpValueNameString, lpszLangId);

        lWin32Status = RegOpenKeyEx (
            hKeyRegistry,
            lpValueNameString,
            RESERVED,
            KEY_READ,
            &hKeyNames);
    } else {
        hKeyNames = HKEY_PERFORMANCE_DATA;
        lstrcpy (CounterNameBuffer, CounterName);
        lstrcat (CounterNameBuffer, lpszLangId);

        lstrcpy (HelpNameBuffer, HelpName);
        lstrcat (HelpNameBuffer, lpszLangId);
    }

    // get size of counter names and add that to the arrays
    
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwCounterSize = dwBufferSize;

    // get size of counter names and add that to the arrays
    
    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;

    dwBufferSize = 0;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
        RESERVED,
        &dwValueType,
        NULL,
        &dwBufferSize);

    if (lWin32Status != ERROR_SUCCESS) goto BNT_BAILOUT;
   
    dwHelpSize = dwBufferSize;

    lpReturnValue = MemoryAllocate (dwArraySize + dwCounterSize + dwHelpSize);

    if (!lpReturnValue) goto BNT_BAILOUT;

    // initialize pointers into buffer

    lpCounterId = lpReturnValue;
    lpCounterNames = (LPWSTR)((LPBYTE)lpCounterId + dwArraySize);
    lpHelpText = (LPWSTR)((LPBYTE)lpCounterNames + dwCounterSize);

    // read counters into memory

    dwBufferSize = dwCounterSize;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Counters : CounterNameBuffer,
        RESERVED,
        &dwValueType,
        (LPVOID)lpCounterNames,
        &dwBufferSize);

    if (!lpReturnValue) goto BNT_BAILOUT;
 
    dwBufferSize = dwHelpSize;
    lWin32Status = RegQueryValueEx (
        hKeyNames,
        dwSystemVersion == OLD_VERSION ? Help : HelpNameBuffer,
        RESERVED,
        &dwValueType,
        (LPVOID)lpHelpText,
        &dwBufferSize);
                            
    if (!lpReturnValue) goto BNT_BAILOUT;
 
    // load counter array items

    for (lpThisName = lpCounterNames;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        usTemp.Length = lstrlen(lpThisName) * sizeof (WCHAR);
        usTemp.MaximumLength = usTemp.Length + sizeof (UNICODE_NULL);
        usTemp.Buffer = lpThisName;

        Status = RtlUnicodeStringToInteger (
            &usTemp,
            10L,
            &dwThisCounter);

        if (Status != ERROR_SUCCESS) goto BNT_BAILOUT;  // bad entry

        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);  

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

    }

    for (lpThisName = lpHelpText;
         *lpThisName;
         lpThisName += (lstrlen(lpThisName)+1) ) {

        // first string should be an integer (in decimal unicode digits)

        usTemp.Length = lstrlen(lpThisName) * sizeof (WCHAR);
        usTemp.MaximumLength = usTemp.Length + sizeof (UNICODE_NULL);
        usTemp.Buffer = lpThisName;

        Status = RtlUnicodeStringToInteger (
            &usTemp,
            10L,
            &dwThisCounter);

        if (Status != ERROR_SUCCESS) goto BNT_BAILOUT;  // bad entry

        // point to corresponding counter name

        lpThisName += (lstrlen(lpThisName)+1);

        // and load array element;

        lpCounterId[dwThisCounter] = lpThisName;

    }
    
    if (pdwLastItem) *pdwLastItem = dwLastId;

    MemoryFree ((LPVOID)lpValueNameString);
    RegCloseKey (hKeyValue);
    if (dwSystemVersion == OLD_VERSION)
        RegCloseKey (hKeyNames);

    return lpReturnValue;

BNT_BAILOUT:
    if (lWin32Status != ERROR_SUCCESS) {
        dwLastError = GetLastError();
    }

    if (lpValueNameString) {
        MemoryFree ((LPVOID)lpValueNameString);
    }
    
    if (lpReturnValue) {
        MemoryFree ((LPVOID)lpReturnValue);
    }
    
    if (hKeyValue) RegCloseKey (hKeyValue);

    if (dwSystemVersion == OLD_VERSION &&
        hKeyNames) RegCloseKey (hKeyNames);


    return NULL;
}

NTSTATUS
GetEnumPerfData (
    IN HKEY hKeySystem,
    IN DWORD dwIndex,
    IN PPERF_DATA_BLOCK *pPerfData
)
{  // GetSystemPerfData
    LONG     lError ;
    DWORD    Size;
    DWORD    Type;
    DWORD    ValueBufferLen;

    WCHAR    wcValueBuffer[64];

    if (dwIndex >= 2)
        return !ERROR_SUCCESS;

    if (*pPerfData == NULL) {
        *pPerfData = MemoryAllocate (INITIAL_SIZE);
        if (*pPerfData == NULL) return ERROR_OUTOFMEMORY;
    }

    while (TRUE) {
        Size = MemorySize (*pPerfData);
        ValueBufferLen = sizeof (wcValueBuffer);
   
        lError = RegQueryValueEx (
            hKeySystem,
            dwIndex == 0 ?
               L"Global" :
               L"Costly",
            RESERVED,
            &Type,
            (LPSTR)*pPerfData,
            &Size);

        if ((!lError) &&
            (Size > 0) &&
            (*pPerfData)->Signature[0] == (WCHAR)'P' &&
            (*pPerfData)->Signature[1] == (WCHAR)'E' &&
            (*pPerfData)->Signature[2] == (WCHAR)'R' &&
            (*pPerfData)->Signature[3] == (WCHAR)'F' ) {

            return (ERROR_SUCCESS);
        }

        if (lError == ERROR_MORE_DATA) {
            *pPerfData = MemoryResize (
                *pPerfData, 
                MemorySize (*pPerfData) +
                EXTEND_SIZE);

            if (!*pPerfData) {
                return (lError);
            }
        } else {
            return (lError);  
        }  // else
    }
}  // GetSystemPerfData

PPERF_OBJECT_TYPE
FirstObject (
    PPERF_DATA_BLOCK pPerfData
)
{
    return ((PPERF_OBJECT_TYPE) ((PBYTE) pPerfData + pPerfData->HeaderLength));
}


PPERF_OBJECT_TYPE
NextObject (
    PPERF_OBJECT_TYPE pObject
)
{  // NextObject
    return ((PPERF_OBJECT_TYPE) ((PBYTE) pObject + pObject->TotalByteLength));
}  // NextObject

PERF_COUNTER_DEFINITION *
FirstCounter(
    PERF_OBJECT_TYPE *pObjectDef
)
{
    return (PERF_COUNTER_DEFINITION *)
               ((PCHAR) pObjectDef + pObjectDef->HeaderLength);
}


PERF_COUNTER_DEFINITION *
NextCounter(
    PERF_COUNTER_DEFINITION *pCounterDef
)
{
    return (PERF_COUNTER_DEFINITION *)
               ((PCHAR) pCounterDef + pCounterDef->ByteLength);
}


LONG
PrintHelpText(
    DWORD   Indent,
    DWORD   dwID,
    LPWSTR  szTextString
)
{
    LPWSTR  szThisChar;

    BOOL    bNewLine;

    DWORD   dwThisPos;
    DWORD   dwLinesUsed;

    szThisChar = szTextString;
    dwLinesUsed = 0;

    // check arguments

    if (!szTextString) {
        return dwLinesUsed;
    }
    
    if (Indent > WRAP_POINT) {
        return dwLinesUsed; // can't do this
    }

    // display id number 

    for (dwThisPos = 0; dwThisPos < Indent - 6; dwThisPos++) {
        putchar (' ');
    }

    dwThisPos += printf ("[%3.3d] ", dwID);
    bNewLine = FALSE;

    // print text

    while (*szThisChar) {

        if (bNewLine){
            for (dwThisPos = 0; dwThisPos < Indent; dwThisPos++) {
                putchar (' ');
            }

            bNewLine = FALSE;
        }
        if ((*szThisChar == L' ') && (dwThisPos >= WRAP_POINT)) {
            putchar ('\n');
            bNewLine = TRUE;
            // go to next printable character
            while (*szThisChar <= L' ') {
                szThisChar++;
            }
            dwLinesUsed++;
        } else {
            putchar (*szThisChar);
            szThisChar++;
        }

        dwThisPos++;
    }

    putchar ('\n');
    bNewLine = TRUE;
    dwLinesUsed++;

    return dwLinesUsed;
}



int
_CRTAPI1 main(
    int argc,
    char *argv[]
    )
{
   
    LPWSTR  *lpCounterText;

    DWORD   dwLastElement;

    DWORD   dwIndex;
    
    DWORD   dwDisplayLevel;

    PPERF_DATA_BLOCK   pDataBlock; // pointer to perfdata block
    BOOL   bError;

    DWORD   dwThisObject;
    DWORD   dwThisCounter;
    CHAR    LangID[10];
    WCHAR   wLangID[10];
    BOOL    UseDefaultID = FALSE;

    PPERF_OBJECT_TYPE   pThisObject;
    PPERF_COUNTER_DEFINITION pThisCounter;


    dwDisplayLevel = PERF_DETAIL_WIZARD;
    
    // open key to registry or use default

    if (argc >= 2) {
        if ((argv[1][0] == '-' || argv[1][0] == '\\') &&
             argv[1][1] == '?') {
            DisplayUsage();
            return 0;
        }

        // get the lang ID
        LangID[0] = argv[1][0];        
        LangID[1] = argv[1][1];        
        LangID[2] = argv[1][2];        
        LangID[3] = '\0';
        mbstowcs(wLangID, LangID, 4);

#if 0
        // get user level from command line
        if (argc > 2 && sscanf(argv[2], " %d", &dwDisplayLevel) == 1) {
            if (dwDisplayLevel <= PERF_DETAIL_NOVICE) {
                dwDisplayLevel = PERF_DETAIL_NOVICE;
            } else if (dwDisplayLevel <= PERF_DETAIL_ADVANCED) {
                dwDisplayLevel = PERF_DETAIL_ADVANCED;
            } else if (dwDisplayLevel <= PERF_DETAIL_EXPERT) {
                dwDisplayLevel = PERF_DETAIL_EXPERT;
            } else {
                dwDisplayLevel = PERF_DETAIL_WIZARD;
            }
        } else {
            dwDisplayLevel = PERF_DETAIL_WIZARD;
        }
#endif

    } else {
        UseDefaultID = TRUE;
    }

    lpCounterText = BuildNameTable (
        HKEY_LOCAL_MACHINE,
        UseDefaultID ? DefaultLangId : wLangID,
        &dwLastElement);

    if (!lpCounterText) {
        printf("***FAILure*** Cannot open the registry\n");
        return 0;
    }

    // get a performance data buffer with counters

    pDataBlock = 0;

    for (dwIndex = 0; (bError = GetEnumPerfData (  
        HKEY_PERFORMANCE_DATA,
        dwIndex,
        &pDataBlock) == ERROR_SUCCESS); dwIndex++) {

        for (dwThisObject = 0, pThisObject = FirstObject (pDataBlock);
            dwThisObject < pDataBlock->NumObjectTypes;
            dwThisObject++, pThisObject = NextObject(pThisObject)) {

            if (pThisObject->DetailLevel <= dwDisplayLevel) {

                printf ("\nObject: \"%ws\"  [%3.3d] \n",
                    lpCounterText[pThisObject->ObjectNameTitleIndex],
                    pThisObject->ObjectNameTitleIndex);
                printf ("   Detail Level: %s\n",
                    pThisObject->DetailLevel <= MAX_LEVEL ?
                    DetailLevelStr[pThisObject->DetailLevel/100-1] :
                    "<N\\A>");

                PrintHelpText (9,
                    pThisObject->ObjectHelpTitleIndex,
                    lpCounterText[pThisObject->ObjectHelpTitleIndex]);

                for (dwThisCounter = 0, pThisCounter = FirstCounter(pThisObject);
                    dwThisCounter < pThisObject->NumCounters;
                    dwThisCounter++, pThisCounter = NextCounter(pThisCounter)) {

                    if (pThisCounter->DetailLevel <= dwDisplayLevel) {

                        printf ("\n    <%ws> [%3.3d]",
                            lpCounterText[pThisCounter->CounterNameTitleIndex],
                            pThisCounter->CounterNameTitleIndex);
                        printf ("\n          Default Scale: %d",
                            pThisCounter->DefaultScale);
                        printf ("\n          Detail Level: %s",
                            pThisCounter->DetailLevel <= MAX_LEVEL ?
                            DetailLevelStr[pThisCounter->DetailLevel/100-1] :
                            "<N\\A>");
                        printf ("\n          Counter Type: 0x%x, %s",
                            pThisCounter->CounterType,
                            GetCounterType(pThisCounter->CounterType));
                        printf ("\n          Counter Size: %d bytes",
                            pThisCounter->CounterSize);

                        printf ("\n");
                        PrintHelpText (16,
                            pThisCounter->CounterHelpTitleIndex,
                            lpCounterText[pThisCounter->CounterHelpTitleIndex]);

                    }
                }
                printf ("\n");
            }
        }
    RegCloseKey (HKEY_PERFORMANCE_DATA);
    }

    return 0;
}

