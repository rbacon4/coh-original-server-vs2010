// Code defining communication between ServerMonitor and ShardMonitor
#include "net_structdefs.h"

enum {
	SVRMONSHARDMON_STATS = COMM_MAX_CMD,
	SVRMONSHARDMON_CONNECT,
	SVRMONSHARDMON_RELAYMESSAGE, // Nothing to do with CmdRelay, just relaying a mapserver message
	SVRMONSHARDMON_RELAY_KILL_ALL_MAPSERVER,
	SVRMONSHARDMON_RELAY_KILL_ALL_LAUNCHER,
	SVRMONSHARDMON_RELAY_START_LAUNCHER,
	SVRMONSHARDMON_RELAY_START_DBSERVER,
	SVRMONSHARDMON_RELAY_CANCEL_ALL,
	SVRMONSHARDMON_RELAY_APPLY_PATCH,
	SVRMONSHARDMON_RELAY_RUN_BATCH_FILE,
	SVRMONSHARDMON_RELAY_START_ALL,
	SVRMONSHARDMON_RELAY_STOP_ALL,
	SVRMONSHARDMON_RECONCILE_LAUNCHER,
};


void svrMonShardMonCommInit(void);
void svrMonShardMonCommSendUpdates(void);
