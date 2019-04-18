/****************************************************************************
*
*    coh_steam_api.h
*    C function declarations for the wrappers defined in steam_api.cpp.
*	 Steam_api.h cannot be included in C files as it contains C++ keywords
*    This file can be included in standard C files.
*
*    Owned by Robert Anderberg
*
***/

// This is the size used in the SteamWorks example 
#define STEAM_AUTH_SESSION_TICKET_MAX_LEN 1024

#ifdef TEST_CLIENT
int COHSteam_SetDebugLevel(int newLevel) {return 0;}

bool COHSteam_Init() { return false; }

void COHSteam_Tick() {}

void COHSteam_Shutdown() {}

const char *COHSteam_GetUserName() { return NULL; }

unsigned int COHSteam_GetAuthSessionTicket(void *ticketString, int maxTicketLength, unsigned int* ticketLength) { return 0; }

void COHSteam_CancelAuthSessionTicket(unsigned int ticketID) {}

// Update user stats
bool COHSteam_RequestCurrentStats() { return false; }

// Mark an achievement as granted
bool COHSteam_SetAchievement(const char *pchName) { return false; }

// Mark an achievement as not granted
bool COHSteam_ClearAchievement(const char *pchName) { return false; }

// Mark all achievements as not granted
bool COHSteam_ClearAllAchievements() { return false; }

// See if an achievement has been granted
bool COHSteam_GetAchievement(const char *pchName, bool *pbAchieved) { return false; }

bool COHSteamAPI_RestartAppIfNecessary() {return false;}

#else
int COHSteam_SetDebugLevel(int newLevel);

bool COHSteam_Init();

void COHSteam_Tick();

void COHSteam_Shutdown();

const char *COHSteam_GetUserName();

unsigned int COHSteam_GetAuthSessionTicket(void *ticketString, int maxTicketLength, unsigned int* ticketLength);

void COHSteam_CancelAuthSessionTicket(unsigned int ticketID);

// Update user stats
bool COHSteam_RequestCurrentStats();

// Mark an achievement as granted
bool COHSteam_SetAchievement(const char *pchName);

// Mark an achievement as not granted
bool COHSteam_ClearAchievement(const char *pchName);

// Mark all achievements as not granted
bool COHSteam_ClearAllAchievements();

// See if an achievement has been granted
bool COHSteam_GetAchievement(const char *pchName, bool *pbAchieved);

bool COHSteamAPI_RestartAppIfNecessary();
#endif


