//This file contains data and prototypes for autoruns. It is autogenerated by StructParser


// autogeneratednocheckin

extern "C"
{
extern void Add_Auto_Cmds_dbserver(void);
extern void _dbserver_AutoRun_SPECIALINTERNAL(void);
extern void autoStruct_fixup_MissionHeaderDescription(void);
extern void autoStruct_fixup_MissionSearchGuestBio(void);
extern void autoStruct_fixup_MissionSearchHeader(void);
extern void autoStruct_fixup_MissionSearchParams(void);
void doAutoRuns_UtilitiesLib_0(void);
void doAutoRuns_UtilitiesLib_1(void);
void doAutoRuns_UtilitiesLib_2(void);
void doAutoRuns_UtilitiesLib_3(void);
void doAutoRuns_UtilitiesLib_4(void);


void doAutoRuns_dbserver_0(void)
{
	static int once = 0;
	if (once) return;
	once = 1;
	doAutoRuns_UtilitiesLib_0();
	_dbserver_AutoRun_SPECIALINTERNAL();
}



void doAutoRuns_dbserver_1(void)
{
	static int once = 0;
	if (once) return;
	once = 1;
	doAutoRuns_UtilitiesLib_1();
	Add_Auto_Cmds_dbserver();
	autoStruct_fixup_MissionHeaderDescription();
	autoStruct_fixup_MissionSearchGuestBio();
	autoStruct_fixup_MissionSearchHeader();
	autoStruct_fixup_MissionSearchParams();
}



void doAutoRuns_dbserver_2(void)
{
	static int once = 0;
	if (once) return;
	once = 1;
	doAutoRuns_UtilitiesLib_2();
}



void doAutoRuns_dbserver_3(void)
{
	static int once = 0;
	if (once) return;
	once = 1;
	doAutoRuns_UtilitiesLib_3();
}



void doAutoRuns_dbserver_4(void)
{
	static int once = 0;
	if (once) return;
	once = 1;
	doAutoRuns_UtilitiesLib_4();
}

extern void utilitiesLibPreAutoRunStuff(void);
int MagicAutoRunFunc_dbserver(void)
{
	utilitiesLibPreAutoRunStuff();
	doAutoRuns_dbserver_0();
	doAutoRuns_dbserver_1();
	doAutoRuns_dbserver_2();
	doAutoRuns_dbserver_3();
	doAutoRuns_dbserver_4();
	return 0;
}

};


#if 0
PARSABLE
6
"Add_Auto_Cmds_dbserver" "autogen_magiccommands" 1 "" "" 
"_dbserver_AutoRun_SPECIALINTERNAL" "_SPECIAL_INTERNAL" 0 "" "" 
"autoStruct_fixup_MissionHeaderDescription" "c:\game\code\CoH\dbserver\..\Common\MissionSearch.h" 1 "" "" 
"autoStruct_fixup_MissionSearchGuestBio" "c:\game\code\CoH\dbserver\..\Common\MissionSearch.h" 1 "" "" 
"autoStruct_fixup_MissionSearchHeader" "c:\game\code\CoH\dbserver\..\Common\MissionSearch.h" 1 "" "" 
"autoStruct_fixup_MissionSearchParams" "c:\game\code\CoH\dbserver\..\Common\MissionSearch.h" 1 "" "" 
#endif