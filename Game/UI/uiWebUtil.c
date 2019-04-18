/***************************************************************************
 *     Copyright (c) 2008-2008, NCSoft
 *     All Rights Reserved
 *     Confidential Property of NCSoft
 *
 * Module Description:
 *
 *
 ***************************************************************************/
#include "uiWebUtil.h"
#include "rsa.h"
#include "AppLocale.h"
#include "cmdgame.h"
#include "file.h"
#include "uiLogin.h"
#include "BigNum.h"
#include "utils.h"
#include "assert.h"
#include "error.h"
#include "mathutil.h"
#include "earray.h"
#include "MemoryPool.h"
#include "StashTable.h"
#include "structNet.h"
#include "netio_core.h"
#include "net_linklist.h"
#include "net_link.h"
#include "crypt.h"
#include "../../libs/HeroBrowser/HeroBrowser.h"
#include "../../3rdparty/steam/coh_steam_api.h"

// modulus for live 
static char g_ncnc_1024_dev_pubmod[] = {
    0x00,0xbf,0xdd,0x1d,0xd4,0x97,0x0e,0xcf,0x38,0x50,0xe6,0x72,0xe8,0x7b,0x22,
    0x22,0xaf,0x07,0x25,0xf2,0x77,0xba,0x1c,0x74,0x46,0x66,0x96,0x21,0xa2,0x78,
    0x7b,0xeb,0x11,0x92,0x4f,0xb6,0xc3,0xc0,0xc6,0xb5,0x6e,0x3d,0x70,0x25,0xc4,
    0xd5,0x9a,0xb4,0x8d,0x07,0xc7,0x91,0xed,0x60,0x03,0x3d,0x77,0x3e,0x36,0x8b,
    0x1a,0xb2,0x8f,0x55,0xfb,0x15,0x50,0x26,0xf7,0x4f,0x51,0x60,0xa1,0x68,0xde,
    0xb1,0x9c,0xa1,0x87,0x03,0xea,0xb5,0x49,0x4b,0x12,0xf0,0x48,0x9a,0xc4,0x5e,
    0x62,0x8e,0x23,0x05,0xa8,0x32,0x5f,0x6f,0x2d,0xdf,0x9d,0xa7,0x3c,0xb3,0xb6,
    0x00,0xf8,0x3b,0x08,0x9f,0x3b,0xe3,0x51,0x00,0xdf,0x69,0x85,0x78,0x19,0xb2,
    0x5f,0x12,0x8a,0xf3,0x2a,0x47,0xc3,0xd5,0x45 
};

// mod for testing
static char g_ncnc_1024_pub_pubmod[] = {
    0x00,0xa8,0xaa,0xf5,0xe3,0xe0,0x6a,0x1e,0x3c,0x0d,0xf9,0xc4,0x0f,0xb6,0xc7,
    0xb1,0x62,0x2d,0x0f,0xce,0x78,0xaa,0x3c,0x19,0x0d,0xa2,0xcb,0x47,0x04,0x98,
    0x3a,0xb8,0x30,0xca,0xc3,0x7b,0xa9,0x0e,0xaa,0x95,0x00,0x65,0x0e,0x7d,0x52,
    0x2b,0x0f,0xce,0x7c,0xea,0xdd,0x8d,0x02,0xcd,0x73,0x94,0x1c,0x1f,0x13,0xcd,
    0x0c,0x4e,0x11,0x26,0xf5,0xf8,0x13,0x5e,0x77,0x35,0xa2,0x8a,0xeb,0xeb,0x8c,
    0x79,0x26,0xf2,0xdf,0x62,0x85,0xff,0xe1,0x1a,0xc3,0x28,0x08,0xe7,0xfc,0x77,
    0x50,0xe7,0xe3,0xab,0xe1,0x4f,0x22,0x27,0x98,0x0a,0x38,0xd6,0x69,0x3f,0x6a,
    0x32,0xd7,0x82,0x1c,0x18,0x81,0x8c,0xaa,0x22,0x73,0xca,0xc6,0xb6,0xc4,0x6e,
    0x63,0x93,0xf2,0xb9,0x38,0x43,0xbc,0x92,0x3b,
};

static char *escapeCharToHex(char *dest, int len, char c)
{
    static const char hexlookup[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    int i = 0;

    if(len < 3)
        return NULL;
    *dest++ = '%';
    *dest++ = hexlookup[(c>>4)&0xf];
    *dest++ = hexlookup[c&0xf];
    return dest;
}

// from rfc2396
static BOOL isUnreservedUrlChar(char c)
{
    static int mark_chars[256/32] = {0}; 
    if(!TSTB(mark_chars,'-'))
    {
        char marks[] = { 
            '-', '_', '.', '!', '~', '*', '\'', '(', ')',
        };
        int i;
        for( i = 0; i < ARRAY_SIZE( marks ); ++i )
            SETB(mark_chars,marks[i]);
    }
    return isalnum(c) || TSTB(mark_chars,c);
}


// for whatever stupid reason base64 
// cannot be used in urls, who thought that up?
// from rfc2396
char *urlEscape(const char *url)
{
    static char urlesc[1024];
    int esclen = ARRAY_SIZE(urlesc);
	const char *u;
	char	*ue;
    int i;
    
	for(u=url,ue=urlesc,i=0;*u && i < esclen;u++,i++)
	{
        if(isUnreservedUrlChar(*u))
            *ue++ = *u;
        else
            ue = escapeCharToHex(ue,esclen-i,*u);
	}
	*ue = 0;
	return urlesc;
}


void BrowserOpenLoginFailure(void * foo)
{
    char tmp[1152];
    static U8 *encrypted = NULL;
    char *url;
    char *game;
    char *data;
    char *mod;
    int mod_len;

    if((isDevelopmentMode() && game_state.url_env != URLENV_LIVE) 
       || game_state.url_env == URLENV_DEV)
    {
        mod = g_ncnc_1024_dev_pubmod;
        mod_len = ARRAY_SIZE(g_ncnc_1024_dev_pubmod);
        url = "http://plaync-preplay.ncaustin.com:8001";
    }
    else
    {
        mod = g_ncnc_1024_pub_pubmod;
        mod_len = ARRAY_SIZE( g_ncnc_1024_pub_pubmod );
        url = "https://secure.ncsoft.com";
    }

    if(locIsEuropean(getCurrentLocale())) 
        game = "ecoh";
    else
        game = "coh";
    
    
	{
		char password[32];
		assert(sizeof(password) == sizeof(g_achPassword));
		cryptRetrieve(password, g_achPassword, sizeof(password));
		sprintf(tmp,"a=%s&p=%s", g_achAccountName, password);
		memset(password, 0, sizeof(password));
	}
    data = rsaEncryptToBase64(tmp,mod,mod_len); 
    if(!data)
    {
        printf("%s: failed to encrypt data ",__FUNCTION__);
        return;
    }

    sprintf(tmp,"%s/cgi-bin/accountStatus.pl?game=%s&data=%s",url,game,urlEscape(data));
    ShellExecute(NULL,NULL,tmp,NULL,NULL,0);
}

void BrowserSendSteamAuthSessionTicket(void)
{
	if (game_state.steamIsInitialized)
	{
		U8 steam_auth_ticket[STEAM_AUTH_SESSION_TICKET_MAX_LEN];
		U32 steam_auth_ticket_len = 0;

		// Cancel the old ticket first
		if (game_state.steamAuthSessionTicketID)
		{
			COHSteam_CancelAuthSessionTicket(game_state.steamAuthSessionTicketID);
			game_state.steamAuthSessionTicketID = 0;
		}

		game_state.steamAuthSessionTicketID = COHSteam_GetAuthSessionTicket(steam_auth_ticket, sizeof(steam_auth_ticket), &steam_auth_ticket_len);

		if (!game_state.steamAuthSessionTicketID)
			steam_auth_ticket_len = 0;

		if (steam_auth_ticket_len)
			webBrowser_setSteamAuthSessionTicket(steam_auth_ticket, steam_auth_ticket_len);
	}
}

