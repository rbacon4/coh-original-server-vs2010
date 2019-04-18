#include "messagefile.h"

#include "utils.h"
#include "earray.h"
#include "timing.h"
#include "fileutil.h"
#include "HashFunctions.h"
#include "EString.h"
#include "log.h"
#include "chatdb.h"
#include "msgsend.h"
#include "chatsqldb.h"

void messageFileTick(void)
{
	static int   timer = 0;
	static char  s_message_path[MAX_PATH] = {0};
	static int   s_message_age = 0;
	static U32   s_message_hash = 0;
	static char *s_message_text = NULL;
	static int   s_message_user = 0;

	int i = 0;
	User** users = chatUserGetAll();
	int count = eaSize(&users);

	if(!count) return;

	if(!timer)
		timer = timerAlloc();
	timerStart(timer);

	if(!s_message_path[0])
	{
		char *s;
		strcpy(s_message_path,fileDataDir());
		s = strrchr(s_message_path,'/');
		if(s)
			*s = '\0';
		strcat(s_message_path,"/chatdb/message.txt");
	}

	if(!fileExists(s_message_path))
	{
		s_message_age = 0;
		return;
	}
	else if(fileHasBeenUpdated(s_message_path,&s_message_age))
	{
        int len;
		char *buf = fileAlloc(s_message_path,&len);
        if(len > 2000)
            LOG(LOG_TEXT_JOURNAL, LOG_LEVEL_VERBOSE, LOG_CONSOLE_ALWAYS, "MOTD file is too long. max number of characters is 2000, %s is %i characters",s_message_path,len);
		
		if(buf)
		{
			s_message_hash = hashString(buf,true);
			estrPrintf(&s_message_text,"SMFMsg \"%s\"",escapeString(buf));
			free(buf);
		}
		else
		{
			s_message_age = 0;
			return;
		}
	}

	while(timerElapsed(timer) < MESSAGEFILE_TICK_DURATION && i < count)
	{
		User *user;

		if(s_message_user >= count)
			s_message_user = 0;

		user = users[s_message_user];

		if(user->link && user->message_hash != s_message_hash)
		{
			if (user->publink_id)
				sendMsg(user->link,user->publink_id,s_message_text);
			else
				sendMsg(user->link,user->auth_id,s_message_text);
			user->message_hash = s_message_hash;
		}

		s_message_user++;
		i++;
	}
}


