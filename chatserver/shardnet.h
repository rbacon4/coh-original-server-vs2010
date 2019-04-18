#ifndef _SHARDNET_H
#define _SHARDNET_H

#include "chatdb.h"
#include "netio.h"

char *getShardName(NetLink *link);

enum 
{
	kChatLink_Public = 0, 
	kChatLink_Shard, 
	kChatLink_Admin,
};

typedef struct
{
	NetLink	*link;
	char	shard_name[64];
	U8		linkType;		// shard, public, or admin (ie chatadmin tool)
	Packet	*aggregate_pak;
	char	last_msg[10000];
	StashTable	user_db_ids;
	int		online;	

	// for ChatAdmin clients only
	int		uploadStatus;
	int		uploadHint;
	int		uploadPending;
	User	*adminUser;

} ClientLink;

typedef struct {
	int	processed_count;
	int	sendmsg_count;
	int	send_rate;
	int	recv_rate;
	int	cross_shard_msgs;
	int error_msgs_sent;

	F32	crossShardRate;
	F32 invalidRate; 
	int	sendMsgRate; 
	int recvMsgRate;

} ChatStats;

typedef struct ChatServerCfg
{
	char sqlLogin[1024];
	char sqlDbName[1024];
} ChatServerCfg;

C_DECLARATIONS_BEGIN

extern ChatStats g_stats;
extern int g_chatLocale;
extern ChatServerCfg g_chatcfg;

void updateCrossShardStats(User * from, User * to);
void chatServerShutdown(User * unused, char * reason);
void adminLoginFinal(ClientLink * client);
void shardDisconnectAll();

C_DECLARATIONS_END

#endif
