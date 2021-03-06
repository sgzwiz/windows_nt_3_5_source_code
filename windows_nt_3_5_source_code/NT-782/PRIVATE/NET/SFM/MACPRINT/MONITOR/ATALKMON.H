/*****
	AppleTalk Print Monitor

	(c) Microsoft 1992, all rights reserved

	FILE NAME:	  atalkmon.h

	DESCRIPTION:	This is the interface to the main module for
		the AppleTalk Print Monitor.

	AUTHOR:		 Frank D. Byrum

	MODIFICATION HISTORY:

		date	who	 description

	26-Aug-92	frankb  Initial version
*****/

#define APPLETALK_SERVICE_NAME	  TEXT("AppleTalk")


/*****  REGISTRY USAGE

	AppleTalk port information is kept in the registry using the
	registry API of Win32.  The monitor is installed by creating
	a registry key, "AppleTalk Printers" at

	HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Print\Monitors

	This key is refered to as the AppleTalk Monitor root key.  Two
	subkey of this root key are created.  "Options" contains registry
	values for configuration options for the monitor on a global
	scale, and "Ports" contains subkeys for each AppleTalk printer
	defined.

	The name of the port subkey is the port name for the printer
	as viewed in the NT Print Manager port list.  This key contains
	a number of values describing the port, including:

	REG_DWORD: TimeOut	  Number of miliseconds to wait for writes to
							the printer to complete

	REG_DWORD: ConfigFlags  Set of flags describing configuration of the
							port.  Currently only includes the flag
							indicating the printer is captured.

	REG_BINARY: NBP Name	NBP name of the printer as an NBP_NAME structure

	The "Options" subkey contains a number of registry values, including:

	REG_DWORD: DebugLevel
	REG_DWORD: DebugSystems
	REG_SZ: LogFile

*****/

#define PRINTER_ENUM_BUFFER_SIZE	1024
#define GENERIC_BUFFER_SIZE			512
#define STATUS_BUFFER_SIZE			100

//
//  registry keys
//

#define ATALKMON_PORTS_SUBKEY		TEXT("\\Ports")
#define ATALKMON_OPTIONS_SUBKEY		TEXT("Options")

//
// registry value names
//

#define ATALKMON_CONFIGFLAGS_VALUE	"Configuration Flags"
#define ATALKMON_ZONENAME_VALUE		"Zone Name"
#define ATALKMON_PORTNAME_VALUE		"Port Name"
#define	ATALKMON_PORT_CAPTURED		"Port Captured"

#define ATALKMON_LOGFILE_VALUE		TEXT("LogFile")

//
// config flags
//

#define SFM_PORT_CAPTURED 			0x00000001
#define SFM_PORT_IN_USE				0x00000002
#define SFM_PORT_POST_READ 			0x00000004
#define SFM_PORT_OPEN 				0x00000008
#define SFM_PORT_CLOSE_PENDING 		0x00000010
#define SFM_PORT_IS_SPOOLER 		0x00000020


//
// job flags
//

#define SFM_JOB_FIRST_WRITE			0x00000001
#define SFM_JOB_FILTER				0x00000002
#define SFM_JOB_DISCONNECTED		0x00000004
#define SFM_JOB_OPEN_PENDING		0x00000008
#define SFM_JOB_ERROR				0x00000010

//
// Various timeout values.
//
#define ATALKMON_DEFAULT_TIMEOUT	 5000
#define ATALKMON_DEFAULT_TIMEOUT_SEC 5
#define CONFIG_TIMEOUT				 (5*60*1000)

//
// filter characters
//

#define CTRL_C						0x03
#define CTRL_D						0x04
#define CTRL_S						0x13
#define CTRL_Q						0x11
#define CTRL_T						0x14
#define CR							0x0d

//
// NBP types
//

#define ATALKMON_RELEASED_TYPE		"LaserWriter"

#define ATALKMON_CAPTURED_TYPE		" LaserWriter"

#define PAP_QUANTUM_SIZE			512
#define PAP_DEFAULT_QUANTUM			8
#define PAP_DEFAULT_BUFFER			(PAP_DEFAULT_QUANTUM*PAP_QUANTUM_SIZE)


// 
// Data structures used.
//

typedef struct _ATALKPORT
{
	struct _ATALKPORT *	pNext;
	
	// Get/Set is protected by hmutexPortList. 
	DWORD				fPortFlags;		

	// These flags do not need mutual exclusion since only the current
	// job will look at them. There will be no contention.
	DWORD				fJobFlags;	

	HANDLE				hmutexPort;
	HANDLE				hPrinter;
	DWORD				dwJobId;
	SOCKET				sockQuery;
	SOCKET				sockIo;
	SOCKET				sockStatus;
	WSH_NBP_NAME		nbpPortName;
	WSH_ATALK_ADDRESS	wshatPrinterAddress;
	WCHAR				pPortName[(MAX_ENTITY+1)*2];
	UCHAR				pReadBuffer[PAP_DEFAULT_BUFFER];
} ATALKPORT;

typedef ATALKPORT * PATALKPORT;
typedef SOCKET * PSOCKET;

typedef struct _TOKENLIST
{
	LPSTR				pszToken;
	DWORD				dwError;
	DWORD				dwStatus;
} TOKENLIST, *PTOKENLIST;


//
//*****  GLOBAL VARIABLES
//

#ifdef ALLOCATE

HANDLE	  		hInst;
HKEY			hkeyPorts 		= NULL;
HANDLE			hmutexPortList 	= NULL;
HANDLE			hmutexDeleteList= NULL;
HANDLE			hmutexJob 		= NULL;
PATALKPORT		pPortList		= NULL;		 
PATALKPORT		pDeleteList		= NULL;		 
HANDLE			hEventLog 		= NULL;
HANDLE			hevConfigChange = NULL;
HANDLE			hevPrimeRead 	= NULL;
HANDLE			hCapturePrinterThread 	= INVALID_HANDLE_VALUE;
HANDLE			hReadThread 	= INVALID_HANDLE_VALUE;
BOOL			boolExitThread 	= FALSE;
CHAR		 	chComputerName[MAX_ENTITY+1];
WCHAR			wchBusy[STATUS_BUFFER_SIZE];
WCHAR			wchPrinting[STATUS_BUFFER_SIZE];
WCHAR			wchPrinterError[STATUS_BUFFER_SIZE];
WCHAR			wchPrinterOffline[STATUS_BUFFER_SIZE];

#ifdef DEBUG_MONITOR

HANDLE			hLogFile = INVALID_HANDLE_VALUE	;

#endif

#else

extern HANDLE	hInst;
extern HKEY		hkeyPorts;
extern HANDLE	hmutexPortList;
extern HANDLE	hmutexDeleteList;
extern HANDLE	hmutexJob; 		
extern PATALKPORT pPortList;  
extern PATALKPORT pDeleteList;  
extern HANDLE	hEventLog;
extern HANDLE	hevConfigChange;
extern HANDLE	hevPrimeRead;
extern HANDLE	hCapturePrinterThread;
extern HANDLE	hReadThread;
extern BOOL		boolExitThread;
extern CHAR		chComputerName[];
extern WCHAR	wchBusy[];
extern WCHAR	wchPrinting[];
extern WCHAR	wchPrinterError[];
extern WCHAR	wchPrinterOffline[];

#ifdef DEBUG_MONITOR 

extern  HANDLE  hLogFile;

#endif

#endif

#ifdef DEBUG_MONITOR

VOID
DbgPrintf(
	char *Format,
	...
	);

#define DBGPRINT(args) DbgPrintf args

#else

#define DBGPRINT(args)

#endif


//***** FUNCTION PROTOTYPES

DWORD 
CapturePrinterThread( 
	IN LPVOID pParameterBlock
);

DWORD 
ReadThread(
	IN LPVOID pParameterBlock 
);

DWORD 
CaptureAtalkPrinter(
	IN SOCKET sock, 
	IN PWSH_ATALK_ADDRESS pAddress,
	IN BOOL fCapture
);

DWORD 
TransactPrinter(
	IN SOCKET sock, 
	IN PWSH_ATALK_ADDRESS pAddress, 
	IN LPBYTE pRequest, 
	IN DWORD cbRequest, 
	IN LPBYTE pResponse,
	IN DWORD  cbResponse
);

VOID 
ParseAndSetPrinterStatus(
	IN PATALKPORT pPort
);

DWORD 
ConnectToPrinter(
	IN PATALKPORT pPort, 
	IN DWORD dwTimeout
);

DWORD 
SetPrinterStatus(
	IN PATALKPORT pPort, 
	IN LPWSTR	 lpwsStatus
);

PATALKPORT 
AllocAndInitializePort(
	VOID
);

VOID 
FreeAppleTalkPort(
	IN PATALKPORT pNewPort
);

DWORD	
LoadAtalkmonRegistry(
	IN HKEY hkeyPorts
);

DWORD	
CreateRegistryPort(
	IN PATALKPORT pNewPort
);

DWORD
SetRegistryInfo( 
	IN PATALKPORT pWalker 
);

DWORD 
WinSockNbpLookup(
	SOCKET sQuerySocket,
	PCHAR pchZone,
	PCHAR pchType,
	PCHAR pchObject,
	PWSH_NBP_TUPLE pTuples,
	DWORD cbTuples,
	PDWORD pcTuplesFound);

DWORD 
OpenAndBindAppleTalkSocket(
	IN PSOCKET pSocket
);

DWORD 
CapturePrinter(
	IN PATALKPORT pPort,
	IN BOOL 	  fCapture
);

DWORD 
IsSpooler(
	IN	 PWSH_ATALK_ADDRESS pAddress,
	IN OUT BOOL * pfSpooler
);

VOID
GetAndSetPrinterStatus( 
	IN PATALKPORT pPort 
);
