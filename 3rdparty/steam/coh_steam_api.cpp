/****************************************************************************
*
*    steam_api.cpp
*	 Expose function definitions that match those declared in coh_steam_api.h
*    to C.
*
*    Owned by Robert Anderberg
*
***/
#include "..\3rdparty\steam\steam_api.h"
#include "float.h"

class SteamCallbackHandlerClass;

static int  s_debugLevel = 0;
static bool s_haveStats = false;		// Waiting for user stats/achievements
static bool s_storingStats = false;		// Waiting for a store operation to finish
static bool s_needStoreStats = false;	// Need to store stats (local stats/achievements have changed)
static SteamCallbackHandlerClass *s_callbackHandler = NULL;	// Class to handle callbacks

static int s_steamAppID = 205210;	// The City of Heroes Steam App ID

class SteamCallbackHandlerClass
{
public:
	// Constructor
	SteamCallbackHandlerClass();

	// Callbacks
	STEAM_CALLBACK( SteamCallbackHandlerClass, OnUserStatsReceived, UserStatsReceived_t, m_CallbackUserStatsReceived );
	STEAM_CALLBACK( SteamCallbackHandlerClass, OnUserStatsStored, UserStatsStored_t, m_CallbackUserStatsStored );
	STEAM_CALLBACK( SteamCallbackHandlerClass, OnGetAuthSessionTicketResponse, GetAuthSessionTicketResponse_t, m_CallbackGetAuthSessionTicketResponse );
};

SteamCallbackHandlerClass::SteamCallbackHandlerClass() :
	m_CallbackUserStatsReceived( this, &SteamCallbackHandlerClass::OnUserStatsReceived ),
	m_CallbackUserStatsStored( this, &SteamCallbackHandlerClass::OnUserStatsStored ),
	m_CallbackGetAuthSessionTicketResponse( this, &SteamCallbackHandlerClass::OnGetAuthSessionTicketResponse )
{
}

void SteamCallbackHandlerClass::OnUserStatsReceived(UserStatsReceived_t *pResponse)
{
	// TODO: re-retrieve stats if the request failed?
	if (pResponse->m_nGameID == SteamUtils()->GetAppID() &&
		pResponse->m_eResult == k_EResultOK)
	{
		s_haveStats = true;
	}
}

void SteamCallbackHandlerClass::OnUserStatsStored(UserStatsStored_t *pResponse)
{
	if (pResponse->m_nGameID == SteamUtils()->GetAppID())
	{
		if ( pResponse->m_eResult != k_EResultOK)
		{
			// Try storing the stats again
			s_needStoreStats = false;
		}

		s_storingStats = false;
	}
}

void SteamCallbackHandlerClass::OnGetAuthSessionTicketResponse(GetAuthSessionTicketResponse_t *pResponse)
{
	if (s_debugLevel)
	{
		printf("SteamCallbackHandlerClass::OnGetAuthSessionTicketResponse() for AuthTicket %d result %d\n", pResponse->m_hAuthTicket, pResponse->m_eResult);
	}
}

class SetSteamFPUPrecision
{
public:
	SetSteamFPUPrecision()
	{
		_controlfp_s(&m_saved_fpu_control, 0, 0);	// save current fpu precision
		_controlfp_s(NULL, _PC_64, _MCW_PC);	// set the fpu to full precision for steam
	}

	~SetSteamFPUPrecision()
	{
		// Restore FPU Precision
		_controlfp_s(NULL, m_saved_fpu_control, _MCW_PC);
	}
private:
	unsigned int m_saved_fpu_control;
};

extern "C"
{

int COHSteam_SetDebugLevel(int newLevel)
{
	int oldLevel = s_debugLevel;
	s_debugLevel = newLevel;
	return oldLevel;
}

//************************************
// Method:    COHSteam_Init
// Returns:   void
//************************************
bool COHSteam_Init()
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call
	bool rc = SteamAPI_Init();

	if (rc)
	{
		s_callbackHandler = new SteamCallbackHandlerClass();
	}

	return rc;
}
	
//************************************
// Method:    COHSteam_Tick
// Returns:   void
//************************************
void COHSteam_Tick()
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call

	// Push out any steam achievement changes to Steam servers
	if (s_needStoreStats && !s_storingStats)
	{
		s_storingStats = true;
		s_needStoreStats = false;

		if (s_debugLevel)
		{
			printf("STEAM DEBUG: storing stats\n");
		}

		SteamUserStats()->StoreStats();
	}

	SteamAPI_RunCallbacks();
}

//************************************
// Method:    COHSteam_Shutdown
// Returns:   void
//************************************
void COHSteam_Shutdown()
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call

	if (s_callbackHandler)
	{
		delete s_callbackHandler;
		s_callbackHandler = NULL;
	}

	SteamAPI_Shutdown();
}

//************************************
// Method:    COHSteam_GetUserName
// Returns:   const char * : Steam User's name
//************************************
const char *COHSteam_GetUserName()
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call

	return SteamFriends()->GetPersonaName();
}

//************************************
// Method:    COHSteam_GetAuthSessionTicket
// Returns:   unsigned int : a handle to the created ticket. return value is zero if the call fails
// Parameter: void * ticketString	: a pointer to a array of bytes, the ticket string will be written here
// Parameter: int maxTicketLength	: the length of the array ticketString points to
// Parameter: uint32 * ticketLength : the length of the returned ticket
//************************************
unsigned int COHSteam_GetAuthSessionTicket( void *ticketString, int maxTicketLength, uint32 *ticketLength )
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call

	int ticketID = SteamUser()->GetAuthSessionTicket(ticketString, maxTicketLength, ticketLength);
	if (s_debugLevel)
	{
		uint32 i;

		printf("STEAM DEBUG: COHSteam_GetAuthSessionTicket() returned ID %d\n", ticketID);
		printf("  steam_auth_ticket = 0x");
		for (i = 0; i < *ticketLength; i++)
		{
			int val = ((U8*) ticketString)[i];
			printf("%02x", val);
		}
		printf("\n");

		CSteamID steamID = SteamUser()->GetSteamID();
		printf("SteamID is 0x%016I64x\n", steamID.ConvertToUint64());
	}

	return ticketID;
}

//************************************
// Method:    COHSteam_CancelAuthSessionTicket
// Returns:   none
// Parameter: unsigned int ticketID	: value returned from COHSteam_GetAuthSessionTicket()
//************************************
void COHSteam_CancelAuthSessionTicket( unsigned int ticketID )
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call

	if (s_debugLevel)
	{
		printf("STEAM DEBUG: COHSteam_CancelAuthSessionTicket %d\n", ticketID);
	}

	SteamUser()->CancelAuthTicket(ticketID);
}

//************************************
// Method:    COHSteam_RequestCurrentStats
// Returns:   bool
// Notes:     You will receive a UserStatsReceived_t callback when the data is ready.
//************************************
bool COHSteam_RequestCurrentStats()
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call

	s_haveStats = false;
	if (s_debugLevel)
	{
		printf("STEAM DEBUG: COHSteam_RequestCurrentStats\n");
	}

	return SteamUserStats()->RequestCurrentStats();
}

//************************************
// Method:    COHSteam_SetAchievement
// Returns:   bool
// Parameter: const char *pchName	: string name of the achievement to unlock/grant
//************************************
bool COHSteam_SetAchievement(const char *pchName)
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call

	// TODO: cache new achievements while waiting on stats?
	if (!s_haveStats)
		return false;

	bool rc = SteamUserStats()->SetAchievement(pchName);

	if (s_debugLevel)
	{
		printf("STEAM DEBUG: COHSteam_SetAchievement '%s': return %d\n", pchName, rc);
	}

	if (rc)
		s_needStoreStats = true;

	return rc;
}

//************************************
// Method:    COHSteam_GetAchievement
// Returns:   bool
// Parameter: const char *pchName	: string name of the achievement to query
// Parameter: bool *pbAchieved	    : pointer to bool to update with achievement state
//************************************
bool COHSteam_GetAchievement(const char *pchName, bool *pbAchieved)
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call

	bool rc;

	if (!s_haveStats)
		return false;

	rc = SteamUserStats()->GetAchievement(pchName, pbAchieved);

	if (s_debugLevel)
	{
		printf("STEAM DEBUG: COHSteam_GetAchievement '%s': result %d return %d\n", pchName, *pbAchieved, rc);
	}

	return rc;
}

//************************************
// Method:    COHSteam_ClearAchievement
// Returns:   bool
// Parameter: const char *pchName	: string name of the achievement to clear
//************************************
bool COHSteam_ClearAchievement(const char *pchName)
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call

	if (!s_haveStats)
		return false;

	bool rc = SteamUserStats()->ClearAchievement(pchName);

	if (s_debugLevel)
	{
		printf("STEAM DEBUG: COHSteam_ClearAchievement '%s': return %d\n", pchName, rc);
	}

	if (rc)
		s_needStoreStats = true;

	return rc;
}

//************************************
// Method:    COHSteam_ClearAllAchievements
// Returns:   bool
//************************************
bool COHSteam_ClearAllAchievements()
{
	SetSteamFPUPrecision fpu; //set correct precision for steam call

	if (!s_haveStats)
		return false;

	// Resets stats and achievements
	bool rc = SteamUserStats()->ResetAllStats(true);

	if (s_debugLevel)
	{
		printf("STEAM DEBUG: COHSteam_ClearAllAchievements: return %d\n", rc);
	}

	if (rc)
		s_needStoreStats = true;

	return rc;
}


bool COHSteamAPI_RestartAppIfNecessary()
{
	return SteamAPI_RestartAppIfNecessary(s_steamAppID);
}

} // end extern "C"
