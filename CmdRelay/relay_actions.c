
#include "relay_actions.h"
#include "comm_backend.h"
#include "netio_core.h"
#include <stdio.h>
#include "AppRegCache.h"
#include "AppVersion.h"
#include "relay_util.h"
#include "process_util.h"
#include "utils.h"
#include "sysutil.h"
#include "file.h"
#include "EString.h"

extern CmdRelayCmdStatus g_status;
extern int g_relayType;  // whether this is a DBSERVER, ARENASERVER, or MAPSERVER relay
char g_lastMsg[2000] = "";
extern BOOL g_cancelThread;

extern CRITICAL_SECTION g_criticalSection;
extern BOOL g_bDebugZeroProtocol;
extern char gStopAllScript[FILENAME_MAX];
extern char gStartAllScript[FILENAME_MAX];
extern char g_launcherFlags[1024];

void relayWorkerThreadBegin(int cmd, char * data);	// from relay_main.c


void SetError(const char * msg)
{
	SetStatus(CMDRELAY_CMDSTATUS_ERROR, msg);
}

void SetStatus(CmdRelayCmdStatus status, const char * msg)
{
	EnterCriticalSection(&g_criticalSection);

	g_status = status;
	if(msg)
		strcpy(g_lastMsg, msg);
	else
		strcpy(g_lastMsg, "");

	LeaveCriticalSection(&g_criticalSection);
}

void onSendProtocol(NetLink * link)
{
	Packet	*pak = pktCreate();
	int protocol;

	if(g_bDebugZeroProtocol)
		protocol = 0;	// for debugging
	else
		protocol = CMDRELAY_PROTOCOL_VERSION;


	pktSendBitsPack(pak, 1,CMDRELAY_ANSWER_PROTOCOL);
	pktSendBitsPack(pak, 1, protocol);	// send this first so that server can decide whether to continue reading from packet
	pktSend(&pak,link);
	lnkFlush(link);
}


// keeps a running count in the LPARAM parameter
#define MAX_WINDOW_TITLE_SIZE	500

// criteria for determining a crashed mapserver: exe name is "mapserver.exe" (case-insensitive) and window title is "City Of Heroes"
BOOL CALLBACK EnumProcCountCrashedMapservers(HWND hwnd, LPARAM lParam)
{
	DWORD processID;
	int * count = (int*) lParam; 

	if(GetWindowThreadProcessId(hwnd, &processID))
	{
		if(ProcessNameMatch(processID, "mapserver.exe"))
		{
			char title[MAX_WINDOW_TITLE_SIZE];
			if(GetWindowText(hwnd, title, MAX_WINDOW_TITLE_SIZE))
			{
				if(stricmp(title, "City Of Heroes")==0)
				{
					++(*count);
				}
			}
		}
	}
	return TRUE;	// signal to continue processing further windows
}


int CountCrashedMapservers()
{
	int count = 0;
	EnumWindows(EnumProcCountCrashedMapservers, (LPARAM) &count);
	return count;
}

BOOL CALLBACK EnumProcCountCrashedArenaServers(HWND hwnd, LPARAM lParam)
{
	DWORD processID;
	int * count = (int*) lParam; 

	if(GetWindowThreadProcessId(hwnd, &processID))
	{
		if(ProcessNameMatch(processID, "arenaserver.exe"))
		{
			char title[MAX_WINDOW_TITLE_SIZE];
			if(GetWindowText(hwnd, title, MAX_WINDOW_TITLE_SIZE))
			{
				if(stricmp(title, "City Of Heroes")==0)
				{
					++(*count);
				}
			}
		}
	}
	return TRUE;	// signal to continue processing further windows
}


int CountCrashedArenaServers()
{
	int count = 0;
	EnumWindows(EnumProcCountCrashedArenaServers, (LPARAM) &count);
	return count;
}


void onSendStatus(NetLink * link)
{
	const char * version = getExecutablePatchVersion("coh server");
	Packet	*pak;
	pak = pktCreate();

	pktSendBitsPack(pak, 1,CMDRELAY_ANSWER_STATUS);
	pktSendBitsPack(pak, 1, g_status);
	pktSendBitsPack(pak, 1, g_relayType);
	pktSendBitsPack(pak, 1, ProcessCount("launcher.exe"));
	pktSendBitsPack(pak, 1, ProcessCount("dbserver.exe"));
	pktSendBitsPack(pak, 1, ProcessCount("mapserver.exe"));
	pktSendBitsPack(pak, 1, CountCrashedMapservers());
	pktSendBitsPack(pak, 1, ProcessCount("arenaserver.exe"));
	pktSendBitsPack(pak, 1, CountCrashedArenaServers());
	pktSendString(pak, version ? version : "Unknown");
	pktSendString(pak, g_lastMsg);
	pktSendBitsPack(pak, 1, ProcessCount("logserver.exe"));

	pktSend(&pak,link);
	lnkFlush(link);
}




BOOL onUpdateSelf(Packet * pak)
{
	FileWrapper * file = 0;
	char * data;
	int count;

	char oldFilename[2000];
	char newFilename[2000];

	printf("Updating self...\n");

	// rename currently running executable
	strcpy(oldFilename, getExecutableName());
	sprintf(newFilename, "%s.bak", oldFilename);

	printf("Renaming '%s' to '%s'\n", oldFilename, newFilename);
	safeRenameFile(oldFilename, newFilename);

	count = pktGetBitsPack(pak, 1);

	data = malloc(sizeof(char) * count);

	pktGetBitsArray(pak, count * 8, data);

	file = fopen(oldFilename, "wb");
	if(file)
	{
		BOOL res;

		if(count != fwrite(data, sizeof(char), count, file))
		{
			// error writing file - revert file back to old name
			safeRenameFile(newFilename, oldFilename);
			printf("Renaming '%s' to '%s'\n", newFilename, oldFilename);
			free(data);
			SetError("Unable auto-update: Failed to write new file");
			return FALSE;
		}
		fclose(file);	
		free(data);

		res = execAndQuit(oldFilename, GetCommandLine());	// will terminate inside this function on success
		if(!res)
			SetError("Unable to start new version of CmdRelay?");	
		return res;
	}
	else
	{
		printf("Unknown error opening output file '%s' while attempting auto-update\n", oldFilename);
		free(data);	
		SetError("Unable auto-update: Failed to open new file");
		return FALSE;
	}
}


BOOL ShouldCancelThread()
{
	BOOL res;
	EnterCriticalSection(&g_criticalSection);
	res = g_cancelThread;
	LeaveCriticalSection(&g_criticalSection);
	return res;
}



extern char g_patch_projects[1024];

BOOL onApplyPatch(char * updateSvr)
{
	char filename[2000];
	int res;
	int i=0;
	BOOL bValid;
	char parameters[2000];


	// possible directories where CohUpdater might be located
	// TODO: some sort of directory search...
	char * paths[] = {	"C:\\CohUpdater",
						"C:\\CohUpdater\\bin",
						"D:\\CohUpdater",
						"D:\\CohUpdater\\bin",
						"C:\\game\\tools\\util",
						{0},	// this must be last entry (for while-loop)
	};



	// determine the UpdateServer to use
	if(!strcmp(updateSvr, ""))
	{
		SetError("No server specified!");
		return FALSE;
	}

	printf("Applying patch from '%s'...\n", updateSvr);

	// need to specify EXE name as first parameter to use with CreateProcess()
	sprintf(parameters, "CohUpdater.exe -us %s %s -nolaunch -nopause", updateSvr, g_patch_projects);

	SetStatus(CMDRELAY_CMDSTATUS_APPLYING_PATCH, 0);


	// find the CohUpdater.exe
	bValid = FALSE;
	while(paths[i])
	{
		sprintf(filename, "%s\\CohUpdater.exe", paths[i]);
		if(fileExists(filename))
		{
			bValid = TRUE;
			break;
		}
		++i;
	}

	if(!bValid)
	{
		SetError("Could not locate CohUpdater.exe");
		return FALSE;
	}

	// kill currently running mapservers & launcher, etc
	onStopAll();


	// -------------------------------------------------
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );

		// Start the child process. 
		if( !CreateProcess( filename, // No module name (use command line). 
							parameters, // Command line. 
							NULL,             // Process handle not inheritable. 
							NULL,             // Thread handle not inheritable. 
							FALSE,            // Set handle inheritance to FALSE. 
							CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,             
							NULL,             // Use parent's environment block. 
							NULL,             // Use parent's starting directory. 
							&si,              // Pointer to STARTUPINFO structure.
							&pi )             // Pointer to PROCESS_INFORMATION structure.
			) 
		{
			SetError( "CreateProcess failed." );
			return FALSE;
		}

		// loop until CohUpdater finishes, or the thread has been "cancelled"
		while(1)
		{
			res = WaitForSingleObject(pi.hProcess, 2000); 

			if(    res != WAIT_TIMEOUT )
			{
				Sleep(6000);

				if(!ProcessCount("cohupdater.exe"))
				{
					printf("CohUpdater finished normally\n");
					break;
				}
			}
			
			
			if(ShouldCancelThread())
			{
				killall("cohupdater.exe");
				printf("Aborting apply patch procedure...");
				break;
			}
		}

		// Close process and thread handles. 
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}

	return TRUE;

}



// cancel all worker threads by setting their "cancel" flag
void onCancelAll()
{
	printf("Cancelling all commands...\n");
	EnterCriticalSection(&g_criticalSection);
	g_cancelThread	= TRUE;
	g_status		= CMDRELAY_CMDSTATUS_READY;
	LeaveCriticalSection(&g_criticalSection);
}

extern char g_workingDir[MAX_PATH];;

BOOL StartProgram(const char * filename, const char * params)
{
	BOOL ret;
	char *actualFilename = NULL;
	extern int g_are_executables_in_bin;
	const char *format = g_are_executables_in_bin ? ".\\bin\\%s" : "%s";
	estrConcatf(&actualFilename, format, filename);
	printf("Starting %s %s\n", actualFilename, params ? params : "");
	// returns less than 32 on error
	ret = 32 < (int)ShellExecute(0, "open", actualFilename, params, g_workingDir[0] ? g_workingDir : regGetInstallationDir(), SW_SHOW);
	estrDestroy(&actualFilename);
	return ret;
}

BOOL onStartDbServer()
{
	int ret;
	killall("dbserver.exe");	// just in case...
	killall("launcher.exe");	// just in case...
	ret = StartProgram("launcher.exe", "-monitor");
	ret |= StartProgram("dbserver.exe", "-startall");
	return ret;
}

BOOL onStartLogServer()
{
	int ret;
	killall("logserver.exe");	// just in case...
	killall("launcher.exe");	// just in case...
	ret = StartProgram("launcher.exe", "-monitor");
	ret |= StartProgram("logserver.exe", 0);
	return ret;
}

BOOL onStartLauncher()
{
	killall("launcher.exe");	// just in case...
	return StartProgram("launcher.exe", g_launcherFlags);
}


BOOL onStartArenaServer()
{
	int ret;
	killall("arenaserver.exe");	// just in case...
	killall("launcher.exe");	// just in case...
	ret = StartProgram("launcher.exe", "-monitor");
	ret |= StartProgram("arenaserver.exe", 0);
	return ret;
}

BOOL onStartStatServer()
{
	int ret;
	killall("statserver.exe");	// just in case...
	killall("launcher.exe");	// just in case...
	ret = StartProgram("launcher.exe", "-monitor");
	ret |= StartProgram("statserver.exe", 0);
	return ret;
}

// terminate all running mapservers & launchers
BOOL onKillAllLauncher()
{
	killall("launcher.exe");
	return TRUE;
}

BOOL onKillAllMapsever()
{
	//	printf("onKillAllMapserver()\n");
	killall("mapserver.exe");
	killall("BeaconServer.exe");
	killall("BeaconClient.exe");
	return TRUE;
}

BOOL onKillAllArenaServer()
{
	//	printf("onKillAllArenaServer()\n");
	killall("arenaserver.exe");
	return TRUE;
}

BOOL onKillAllStatServer()
{
	//	printf("onKillAllStatServer()\n");
	killall("statserver.exe");
	return TRUE;
}

BOOL runScript(char * batchFile)
{
	char * commandStr;

	if(!fileExists(batchFile))
	{
		SetError("Unknown error - custom script file doesn't exist, but should!");
		printf("Error: Custom script file '%s' does not exist!\n", batchFile);
		return FALSE;
	}

	commandStr = strdup(batchFile);	// this will be deallocated by thread
	relayWorkerThreadBegin(CMDRELAY_REQUEST_RUN_BATCH_FILE, commandStr);

	return TRUE;
}

BOOL onStopAll()
{
	if(gStopAllScript[0])
	{
		return runScript(gStopAllScript);
	}
	else
	{
		killall("mapserver.exe");
		killall("launcher.exe");
		killall("cityofheroes.exe");
		killall("cohupdater.exe");
		killall("dbserver.exe");
		killall("logserver.exe");
		killall("chatserver.exe");
		killall("arenaserver.exe");
		killall("statserver.exe");
		killall("raidserver.exe");
		killall("BeaconClient.exe");
		killall("BeaconServer.exe");
	}

	return TRUE;
}

BOOL onStartAll()
{
	switch(g_relayType)
	{
	case RELAY_TYPE_MAPSERVER:
		onStartLauncher();
	xcase RELAY_TYPE_DBSERVER:
		onStartDbServer();
	xcase RELAY_TYPE_LOGSERVER:
		onStartLogServer();
	xcase RELAY_TYPE_ARENASERVER:
		onStartArenaServer();
	xcase RELAY_TYPE_STATSERVER:
		onStartStatServer();
	xcase RELAY_TYPE_CUSTOM:
		{
			if(gStartAllScript[0])
			{
				return runScript(gStartAllScript);
			}
		}
	}

	return TRUE;
}





BOOL startProcessAndWait(char * exeName, char * params)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	BOOL bRes = FALSE;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	// Start the child process. 
	if( !CreateProcess( exeName, // No module name (use command line). 
		params, // Command line. 
		NULL,             // Process handle not inheritable. 
		NULL,             // Thread handle not inheritable. 
		FALSE,            // Set handle inheritance to FALSE. 
		CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,             
		NULL,             // Use parent's environment block. 
		NULL,             // Use parent's starting directory. 
		&si,              // Pointer to STARTUPINFO structure.
		&pi )             // Pointer to PROCESS_INFORMATION structure.
		) 
	{
		SetError( "CreateProcess failed." );
		return FALSE;
	}

	// loop until CohUpdater finishes, or the thread has been "cancelled"
	while(1)
	{
		DWORD r = WaitForSingleObject(pi.hProcess, 2000); 

		if(    r != WAIT_TIMEOUT )
		{
			printf("Process finished normally\n");
			bRes = TRUE;
			break;
		}
		else if(ShouldCancelThread())
		{
			KillProcessEx(pi.dwProcessId, TRUE);	// kill entire process tree
			printf("Aborting process...");
			bRes = TRUE;
			break;
		}
	}

	// Close process and thread handles. 
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	return bRes;
}

BOOL onExecuteBatchFile(char * commandStr)
{
	SetStatus(CMDRELAY_CMDSTATUS_CUSTOM_CMD, 0);

	return startProcessAndWait(commandStr, 0);
}

// execute a custom shell command in the background worker thread
BOOL onExecuteCustomCommand(char * commandStr)
{
//	int res;
//	int i=0;
	char params[10000];
//	SHELLEXECUTEINFO ShellInfo;

	if(!strcmp(commandStr, ""))
	{
		SetError("Empty command string!");
		return FALSE;
	}

	sprintf(params, "/C \"%s\"", commandStr);		// must surround entire command in quotes
	printf("Executing custom command '%s'...\n", params);
	SetStatus(CMDRELAY_CMDSTATUS_CUSTOM_CMD, 0);

	return startProcessAndWait("cmd.exe", params);

	//// run the updater
	//memset(&ShellInfo, 0, sizeof(ShellInfo)); // Set up memory block
	//ShellInfo.cbSize		= sizeof(ShellInfo); // Set up structure size
	//ShellInfo.hwnd			= NULL; // Calling window handle
	//ShellInfo.lpVerb		= "open"; // Open the file with default program
	//ShellInfo.lpFile		= "cmd.exe"; // File to open
	//ShellInfo.lpDirectory	= 0;
	//ShellInfo.lpParameters	= cmd;
	//ShellInfo.nShow			= SW_NORMAL; // Open in normal window
	//ShellInfo.fMask			= SEE_MASK_NOCLOSEPROCESS; // Necessary if you want to wait for spawned process



	//res = ShellExecuteEx(&ShellInfo); // Call to function
	//if (res)
	//{
	//	// loop until custom finishes, or the thread has been "cancelled"
	//	// NOTE: This still has a bug, it won't cancel child process correctly (yet)
	//	while(1)
	//	{
	//		res = WaitForSingleObject(ShellInfo.hProcess, 2000); 

	//		if(res != WAIT_TIMEOUT)
	//		{
	//			printf("Custom command finished normally\n");
	//			break;
	//		}
	//		else if(ShouldCancelThread())
	//		{
	//			KillProcessEx(ShellInfo.hProcess, TRUE);
	//			printf("Aborting ShellExecute() thread...");
	//			break;
	//		}
	//	}
	//	return TRUE;
	//}
	//else
	//{
	//	SetError("ShellExecuteEx() returned failure");
	//	return FALSE;
	//}
}
