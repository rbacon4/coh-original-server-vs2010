#include "statgroupstruct.h"
#include "container/dbcontainerpack.h"
#include "teamCommon.h"

LineDesc levelingpact_line_desc[] =
{
	{{ PACKTYPE_INT,	SIZE_INT32,	"Count",				OFFSET(LevelingPact, count) },			"Number of members in the pact" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"Experience",			OFFSET(LevelingPact, experience) },		"Total experience gained by the pact" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"Influence1",			OFFSET(LevelingPact, influence[0]) },	"Influence waiting for member 1" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"Influence2",			OFFSET(LevelingPact, influence[1]) },	"Influence waiting for member 2" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLogged",			OFFSET(LevelingPact, timeLogged) },		"Total time put into the pact by both members." },
	{{ PACKTYPE_INT,	SIZE_INT32,	"version",				OFFSET(LevelingPact, version) },		"The leveling pact version.  As the structure changes and needs updates, this number will increase." },
	{{ PACKTYPE_INT,	SIZE_INT32,	"Infamy1",				OFFSET(LevelingPact, deprecatedInfamy[0]) },	"Infamy waiting for member 1" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"Infamy2",				OFFSET(LevelingPact, deprecatedInfamy[1]) },	"Infamy waiting for member 2" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel0",		OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel1",		OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel2",		OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel3",		OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel4",		OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel5",		OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel6",		OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel7",		OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel8",		OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel9",		OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel10",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel11",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel12",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel13",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel14",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel15",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel16",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel17",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel18",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel19",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel20",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel21",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel22",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel23",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel24",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel25",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel26",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel27",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel28",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel29",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel30",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel31",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel32",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel33",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel34",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel35",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel36",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel37",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel38",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel39",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel40",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel41",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel42",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel43",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel44",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel45",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel46",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel47",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel48",	OFFSET_IGNORE() },	"deprecated" },
	{{ PACKTYPE_INT,	SIZE_INT32,	"timeLoggedLevel49",	OFFSET_IGNORE() },	"deprecated" },	{ 0 },
};
STATIC_ASSERT(ARRAY_SIZE(((LevelingPact*)NULL)->influence) == 2); // update fields above

StructDesc levelingpact_desc[] =
{
	sizeof(LevelingPact),
	{AT_NOT_ARRAY,{0}},
	levelingpact_line_desc,
	"The list of leveling pacts."
};

LineDesc league_teamleaders_line_desc[] =
{
	// Note there's no offset, because we find the base address of the
	// array elements and they're just numbers so there's no offsetting
	// within a larger structure.
	{{PACKTYPE_INT, SIZE_INT32, "TeamLeadId", 0},
	"This members teamLeader Id"},
	{0}
};

StructDesc league_teamleaders_desc[] =
{
	//	array of team leaders
	//	this is meant to store the team leaders while there is a member update occuring
	sizeof(U32),
	{AT_STRUCT_ARRAY,{INDIRECTION(League, teamLeaderIDs, 0)}},
	league_teamleaders_line_desc,

	"League team leader ids\n"
};

LineDesc league_teamlock_line_desc[] =
{
	// Note there's no offset, because we find the base address of the
	// array elements and they're just numbers so there's no offsetting
	// within a larger structure.
	{{PACKTYPE_INT, SIZE_INT32, "TeamLockStatus", 0},
	"This members team lock status"},
	{0}
};
StructDesc league_teamlock_desc[] =
{
	//	array of team locks
	//	this is meant to store the team locks while there is a member update occuring
	sizeof(U32),
	{AT_STRUCT_ARRAY,{INDIRECTION(League, lockStatus, 0)}},
	league_teamlock_line_desc,

	"League team lock ids\n"
};

LineDesc league_line_desc[] =
{
	{{ PACKTYPE_INT,			SIZE_INT32,						"LeaderId",		OFFSET2(League, members, TeamMembers, leader  ) },
				"DB ID - ContainerID of the leader of the team"},

	{{ PACKTYPE_INT,	SIZE_INT32,	"version",				OFFSET(League, revision) },		
				"The league revision. When members get added/quit too fast, the db can lag behind the statserver" },
	{ PACKTYPE_SUB, MAX_LEAGUE_MEMBERS,	"TeamLeaderIds",		(int)league_teamleaders_desc		},

	{ PACKTYPE_SUB, MAX_LEAGUE_MEMBERS,	"TeamLockStatus",		(int)league_teamlock_desc		},

	{ 0 },
};

StructDesc league_desc[] =
{
	sizeof(League),
	{AT_NOT_ARRAY,{0}},
	league_line_desc,

	"Describe a league that consists of teams."	
};