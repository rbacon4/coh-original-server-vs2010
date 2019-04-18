#ifndef RELAY_ACTIONS_H
#define RELAY_ACTIONS_H

#include "netio.h"
#include "relay_types.h"
#include "kill_process.h"

// copies to temp folder & restarts 
BOOL copyAndRestartInTempDir();

// report status back to server - current CohSvr version, idle/executing cmd, etc
void onSendStatus(NetLink * link);

// send the CmdRelay protocol version used by this application
void onSendProtocol(NetLink * link);

// close down launcher/mapserver, apply patch, & restart launcher
BOOL onApplyPatch(char * updateSvr);

BOOL onStartDbServer();

BOOL onStartLauncher();

BOOL onKillAllLauncher();

BOOL onKillAllMapsever();

BOOL onStopAll();

BOOL onStartAll();

BOOL onStartArenaServer();

BOOL onKillAllArenaServer();

BOOL onStartStatServer();

BOOL onKillAllStatServer();


// execute any custom command line action
BOOL onExecuteCustomCommand(char * commandStr);

BOOL onExecuteBatchFile(char * commandStr);

// set global flag to alert worker threads that they need to stop
void onCancelAll();

BOOL onUpdateSelf(Packet * pak);

void SetError(const char * msg);
void SetStatus(CmdRelayCmdStatus s, const char * msg);

#endif RELAY_ACTIONS_H
