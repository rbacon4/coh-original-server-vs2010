extern "C"
{
#include "perforce.h"
#include "wininclude.h"
#include "error.h"
#include "file.h"
#include "winutil.h"
#include "utils.h"
#include "SuperAssert.h"
#include "sysutil.h"
#include "RegistryReader.h"
#include "earray.h"
#include "genericDialog.h"
#include "UtilsNew/lock.h"
}

static const char *blockFileRegPath = "HKEY_CURRENT_USER\\Software\\Cryptic\\Perforce\\BlockedFiles";

static char *perforceErrorString[] = {
	NULL,
	"Perforce command failed.",
	"PES02",
	"PES03",
	"PES04",
	"PES05",
	"PES06",
	"File is locked by someone else.",
	"PES08",
	"File is already checked out.",
	"File does not exist in source control.",
	"Local file does not exist.",
	"Perforce.cpp failed to connect to Perforce.",
	"File already deleted.",
	"PES14",
	"PES15",
	"PES16",
	"PES17",
	"PES18",
	"Failed to merge local changes with new revision of file.",
	"P4V installation was not found.",
};

#define PES(_x) (perforceErrorString[_x])

static const char *getBlockString(const char *fullpath)
{
	char fullpath_local[MAX_PATH];
	RegReader reader;
	static char out[1024];
	strcpy(fullpath_local, fullpath);

	reader = createRegReader();
	initRegReader(reader, blockFileRegPath);
	const char *ret = rrReadString(reader, forwardSlashes(fullpath_local), SAFESTR(out)) ? out : NULL;
	destroyRegReader(reader);
	return ret;
}

const char *perforceOfflineGetErrorString(PerforceErrorValue error)
{
	if (error>=ARRAY_SIZE(perforceErrorString)) {
		return perforceErrorString[0];
	}
	return perforceErrorString[error];
}

PerforceErrorValue perforceOfflineBlockFile(const char *relpath, const char *block_string)
{
	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	char fullpath_local[MAX_PATH];
	RegReader reader;
	strcpy(fullpath_local, fullpath);

	reader = createRegReader();
	initRegReader(reader, blockFileRegPath);
	rrWriteString(reader, forwardSlashes(fullpath_local), block_string);
	destroyRegReader(reader);
	return PERFORCE_NO_ERROR;
}

PerforceErrorValue perforceOfflineUnblockFile(const char *relpath)
{
	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	char fullpath_local[MAX_PATH];
	RegReader reader;
	strcpy(fullpath_local, fullpath);

	reader = createRegReader();
	initRegReader(reader, blockFileRegPath);
	rrDelete(reader, forwardSlashes(fullpath_local));
	destroyRegReader(reader);
	return PERFORCE_NO_ERROR;
}

int perforceOfflineQueryIsFileBlocked(const char *relpath)
{
	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	return getBlockString(fullpath) ? 1 : 0;
}

#ifdef FINAL

static void perforceFinish() { }
void perforceDisable(int disable) { }
PerforceErrorValue perforceAdd(const char *relpath, PerforcePathType pathType) { return PERFORCE_ERROR_NO_SC; }
PerforceErrorValue perforceEdit(const char *relpath, PerforcePathType pathType) { return PERFORCE_ERROR_NO_SC; }
PerforceErrorValue perforceSubmit(const char *relpath, PerforcePathType pathType, const char *description) { return PERFORCE_ERROR_NO_SC; }
PerforceErrorValue perforceDelete(const char *relpath, PerforcePathType pathType) { return PERFORCE_ERROR_NO_SC; }
PerforceErrorValue perforceSync(const char *relpath, PerforcePathType pathType) { return PERFORCE_ERROR_NO_SC; }
PerforceErrorValue perforceSyncForce(const char *relpath, PerforcePathType pathType) { return PERFORCE_ERROR_NO_SC; }
PerforceErrorValue perforceRevert(const char *relpath, PerforcePathType pathType) { return PERFORCE_ERROR_NO_SC; }
PerforceErrorValue perforceGuiSubmitFile(const char *path) { return PERFORCE_ERROR_NO_SC; }
int perforceQueryIsFileNew(const char *relpath) {return 0;}
PerforceErrorValue perforceGuiHistory(const char *path) { return PERFORCE_ERROR_NO_SC; }
const char *perforceQueryIsFileLocked(const char *relpath) { return PES(PERFORCE_ERROR_NO_SC); }
int perforceQueryIsFileLockedByMeOrNew(const char *relpath) { return 1; }
int perforceQueryIsFileMine(const char *relpath) { return 1; }
const char *perforceQueryLastAuthor(const char *relpath) { return PES(PERFORCE_ERROR_NO_SC); }
int perforceQueryIsFileLatest(const char *relpath) { return 1; }
const char *perforceQueryUserName(void) { return PES(PERFORCE_ERROR_NO_SC); }
int perforceQueryAvailable(void) { return 1; }
const char *perforceQueryBranchName(const char *localpath) { return PES(PERFORCE_ERROR_NO_SC); }
int attemptToCheckOut(char *filename, int force) { return 0; }

#else

#include <string>
#include <sstream>
#include <vector>

static bool bPerforceEnabled=true;
static bool bVerbose=false;
static char sPerforcePort[256];
static char sPerforceClientName[128];
static char sPerforceUser[128];
static char sPerforceClientRoot[MAX_PATH];

static const char *perforce_gla_error = "Not in database";
static const char *perforce_gla_not_latest = "You do not have the latest version";
static const char *p4v_exe_path = "C:\\Program Files\\Perforce\\p4v.exe";

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include "../../../3rdparty/p4api/include/p4/clientapi.h"
#pragma comment(lib, "libclient.lib")
#pragma comment(lib, "librpc.lib")
#pragma comment(lib, "libsupp.lib")

static ClientApi * client = NULL;
static Error s_e;
static bool bClientInitialized = false;
static void perforceFinish();

class InitializeClientUser : public ClientUser
{
public:
	bool bInfoPopulated;

	InitializeClientUser() : bInfoPopulated(false)
	{
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *port = varList->GetVar("serverAddress");
		StrPtr *client = varList->GetVar("clientName");
		StrPtr *user = varList->GetVar("userName");
		StrPtr *clientRoot = varList->GetVar("clientRoot");

		if (port && client && user && clientRoot)
		{
			strcpy(sPerforcePort, port->Text());
			strcpy(sPerforceClientName, client->Text());
			strcpy(sPerforceUser, user->Text());
			strcpy(sPerforceClientRoot, clientRoot->Text());
			bool settingsMismatch = 
				stricmp(sPerforceClientName, getComputerName()) != 0 ||
				stricmp(sPerforceUser, getUserName()) != 0;

			char buffer[2];
			bool skipDialog = GetEnvironmentVariable("NOWARNPERFORCE", buffer, sizeof(buffer)) == 1 && strcmp(buffer, "1") == 0;

			if (settingsMismatch && !skipDialog)
			{
				std::stringstream dialogTextStream;
				dialogTextStream << "The Perforce environment [ ";
				dialogTextStream << sPerforceClientName;
				dialogTextStream << ", ";
				dialogTextStream << sPerforceUser;
				dialogTextStream << " ] does not match the system [ ";
				dialogTextStream << getComputerName();
				dialogTextStream << ", ";
				dialogTextStream << getUserName();
				dialogTextStream << " ]. It is dangerous to use Perforce in this state unless you know what you are doing. Are you sure you're fine with this?";
				if (okCancelDialog(dialogTextStream.str().c_str(), "Potential Perforce Problem") == IDOK)
				{
					this->bInfoPopulated = true;
				}
				else
				{
					bPerforceEnabled = false;
				}
			}
			else
			{
				this->bInfoPopulated = true;
			}
		}

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}
};

class FileLockClientUser : public ClientUser
{
public:
	bool locked;

	FileLockClientUser() : locked(false)
	{
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *action = varList->GetVar("action");

		if (action)
		{
			this->locked = true;
		}

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}
};

class LastAuthorClientUser : public ClientUser
{
public:
	char author[128];

	LastAuthorClientUser()
	{
		this->author[0] = '\0';
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *user = varList->GetVar("user");

		if (user)
		{
			strcpy(this->author, user->Text());
		}
		else
		{
			this->author[0] = '\0';
		}

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}
};

class FileLatestClientUser : public ClientUser
{
public:
	bool haveEqualsHead;
	int modTime;

	FileLatestClientUser() : haveEqualsHead(false), modTime(-1)
	{
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *headRev = varList->GetVar("headRev");
		StrPtr *haveRev = varList->GetVar("haveRev");
		StrPtr *headModTime = varList->GetVar("headModTime");

		devassert(headRev);
		// dont assert, client may not have file locally -- devassert(haveRev);
		devassert(headModTime);

		this->haveEqualsHead = headRev && haveRev && atoi(headRev->Text()) == atoi(haveRev->Text());
		if (headModTime)
		{
			modTime = atoi(headModTime->Text());
		}

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}
};

class NewClientUser : public ClientUser
{
public:
	bool isNew;
	bool headDeleted;

	// isNew defaults to true because OutputStat is not called if file isn't known to perforce 
	NewClientUser() : isNew(true), headDeleted(false)
	{
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *headRev = varList->GetVar("headRev");
		StrPtr *headAction = varList->GetVar("headAction");

		isNew = (headRev == NULL);
		headDeleted = (!isNew && headAction && strcmp(headAction->Text(), "delete") == 0);

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}
};

class AddClientUser : public ClientUser
{
public:
	bool success;
	bool alreadyExists;
	bool alreadyDeleted;

	AddClientUser() : success(false), alreadyExists(false), alreadyDeleted(false)
	{
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *action = varList->GetVar("action");

		devassert(!action || strcmp(action->Text(), "add") == 0);
		this->success = this->success || (action ? true : false);

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}

	void Message(Error *err)
	{
		StrBuf buf;
		err->Fmt(&buf);
		this->alreadyExists = this->alreadyExists || strstr(buf.Text(), "existing file") ? true : false;
		this->alreadyDeleted = this->alreadyDeleted || strstr(buf.Text(), "already opened for delete") ? true : false;

		if (bVerbose)
		{
			ClientUser::Message(err);
		}
	}
};

class EditClientUser : public ClientUser
{
public:
	bool success;
	bool missing;
	bool alreadyDeleted;
	bool alreadyLocked;

	EditClientUser() : success(false), missing(false), alreadyDeleted(false), alreadyLocked(false)
	{
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *action = varList->GetVar("action");

		devassert(!action || strcmp(action->Text(), "edit") == 0);
		this->success = this->success || (action ? true : false);

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}

	void Message(Error *err)
	{
		StrBuf buf;
		err->Fmt(&buf);
		this->missing = this->missing || strstr(buf.Text(), "not on client") ? true : false;
		this->alreadyDeleted = this->alreadyDeleted || strstr(buf.Text(), "already opened for delete") ? true : false;
		this->alreadyLocked = this->alreadyLocked || strstr(buf.Text(), "can't edit exclusive") ? true : false;

		if (bVerbose)
		{
			ClientUser::Message(err);
		}
	}
};

class TestResolveClientUser : public ClientUser
{
public:
	bool success;

	TestResolveClientUser() : success(false)
	{
	}

	void Message(Error *err)
	{
		StrBuf buf;
		err->Fmt(&buf);
		this->success = this->success || strstr(buf.Text(), "no file(s) to resolve") ? true : false;

		if (bVerbose)
		{
			ClientUser::Message(err);
		}
	}
};

class SubmitClientUser : public ClientUser
{
public:
	bool success;

	SubmitClientUser() : success(false)
	{
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *submittedChange = varList->GetVar("submittedChange");
		this->success = this->success || (submittedChange ? true : false);

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}

	void Message(Error *err)
	{
		StrBuf buf;
		err->Fmt(&buf);
		this->success = this->success || strstr(buf.Text(), "No files to submit") ? true : false;

		if (bVerbose)
		{
			ClientUser::Message(err);
		}
	}
};

class DeleteClientUser : public ClientUser
{
public:
	bool success;
	bool missing;

	DeleteClientUser() : success(false), missing(false)
	{
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *action = varList->GetVar("action");

		devassert(!action || strcmp(action->Text(), "delete") == 0);
		this->success = this->success || (action ? true : false);

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}

	void Message(Error *err)
	{
		StrBuf buf;
		err->Fmt(&buf);
		this->missing = this->missing || strstr(buf.Text(), "not on client") ? true : false;

		if (bVerbose)
		{
			ClientUser::Message(err);
		}
	}
};

class SyncClientUser : public ClientUser
{
public:
	bool success;
	bool missing;

	SyncClientUser() : success(false), missing(false)
	{
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *action = varList->GetVar("action");

		this->success = this->success || (action ? true : false);

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}

	void Message(Error *err)
	{
		StrBuf buf;
		err->Fmt(&buf);
		this->success = this->success || strstr(buf.Text(), "up-to-date") ? true : false;
		this->missing = this->missing || strstr(buf.Text(), "no such file") ? true : false;
		this->missing = this->missing || strstr(buf.Text(), "not in client view") ? true : false;

		if (bVerbose)
		{
			ClientUser::Message(err);
		}
	}
};

class RevertClientUser : public ClientUser
{
public:
	bool success;

	RevertClientUser() : success(false)
	{
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *action = varList->GetVar("action");

		devassert(!action || strcmp(action->Text(), "reverted") == 0);
		this->success = this->success || (action ? true : false);

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}

	void Message(Error *err)
	{
		StrBuf buf;
		err->Fmt(&buf);
		this->success = this->success || strstr(buf.Text(), "not opened on this client") ? true : false;

		if (bVerbose)
		{
			ClientUser::Message(err);
		}
	}
};

class DirPathClientUser : public ClientUser
{
public:
	char path[2048];

	DirPathClientUser()
	{
		path[0]=0;
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *dir = varList->GetVar("dir");
		strcpy(path, dir ? dir->Text() : "");

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}
};

class GroupListClientUser : public ClientUser
{
public:
	bool success;
	char **groups;

	GroupListClientUser() : success(false)
	{
		eaCreate(&this->groups);
	}

	~GroupListClientUser()
	{
		eaDestroyEx(&this->groups, 0);
	}

	void OutputStat(StrDict *varList)
	{
		StrPtr *group = varList->GetVar("group");

		eaPush(&this->groups, strdup(group->Text()));
		this->success = true;

		if (bVerbose)
		{
			ClientUser::OutputStat(varList);
		}
	}
};


static const int TIMEOUT_MSECS				= 600000; // our timeout

// subclass KeepAlive to implement a customized IsAlive function.
// code from http://www.perforce.com/perforce/doc.current/manuals/p4api/03_methods.html#1113644
class MyKeepAlive : public KeepAlive
{
	DWORD m_startTime;

public:
	virtual int		IsAlive();
	void			Reset() { m_startTime = timeGetTime(); }
};
static MyKeepAlive sKeepAlive;

int   MyKeepAlive::IsAlive()
{
	if( (timeGetTime() - m_startTime) >  TIMEOUT_MSECS) {
		printf("Perforce command took too long to respond, aborting perforce command\n");
	   return 0;
	}

   return 1;
}

static bool runsWithTimeout()
{
	char buffer[2];
	return
		GetEnvironmentVariable("NOTIMEOUTPERFORCE", buffer, ARRAY_SIZE_CHECKED(buffer)) != 1 ||
		strcmp(buffer, "1") != 0;
}

static bool tryRunCommand(const char *func, int argc, char *const *argv, ClientUser &ui)
{
	static LazyLock mpCritSect = {0};

	lazyLock(&mpCritSect);
	client->SetProtocol("tag", "");

	if(!bClientInitialized)
	{
		// Connect to server
		client->Init(&s_e);
		sKeepAlive.Reset();
		if (runsWithTimeout())
		{
			client->SetBreak( &sKeepAlive ); 
		}

		if (s_e.Test())
		{
			StrBuf msg;
			s_e.Fmt(&msg);
			printf("%s\n", msg.Text());
			lazyUnlock(&mpCritSect);
			return false;
		}

		bClientInitialized = true;
		atexit(perforceFinish);
	}

	// Populate info
	if (argc && argv)
	{
		client->SetArgv(argc, argv);
	}
	sKeepAlive.Reset();
	client->Run(func, &ui);

	lazyUnlock(&mpCritSect);
	return true;
}

static bool initializeClient()
{
	InitializeClientUser ui;

	printf("Initializing perforce client...");
	if (!tryRunCommand("info", 0, 0, ui))
	{
		return 0;
	}

	bool bSuccess = ui.bInfoPopulated;
	printf("%s\n", bSuccess ? "success":"FAILED");
	return bSuccess;
}

static void perforceInit(void)
{
	if (client)
		return;

	client = new ClientApi;

	if (isProductionMode() || !initializeClient())
	{
		bPerforceEnabled = false;
	}
}

static void perforceFinish()
{
	// Close connection
	bPerforceEnabled = false;
	client->Final(&s_e);

	delete client;
	client = NULL;
}

void perforceDisable(int disable)
{
	bPerforceEnabled = !disable;
}

#define PERFORCE_LAZY_INIT(...)					\
	if (!client) perforceInit();		\
	if (!bPerforceEnabled) return __VA_ARGS__;	//

static int attemptFakeCheckOut(char *fileName)
{
	DWORD dwAttrs = GetFileAttributes(fileName);
	if (dwAttrs == INVALID_FILE_ATTRIBUTES)
	{
		return 0;
	}

	if (dwAttrs & FILE_ATTRIBUTE_READONLY)
	{
		SetFileAttributes(fileName, dwAttrs & ~FILE_ATTRIBUTE_READONLY); 
		dwAttrs = GetFileAttributes(fileName);
		if (dwAttrs == INVALID_FILE_ATTRIBUTES || dwAttrs & FILE_ATTRIBUTE_READONLY)
		{
			return 0;
		}
	}

	return 1;
}

static void str_replace(std::string &subject, const std::string &search, const std::string &replace)
{
	for (std::string::size_type matchIndex = subject.find(search);
		matchIndex != std::string::npos;
		matchIndex = subject.find(search, matchIndex + replace.length()))
	{
		subject.replace(matchIndex, search.length(), replace);
	}
}

static std::string formPerforceFilespec(const char *path, bool escapeSpecialChars /* false for p4 add */, PerforcePathType pathType)
{
	if (!path || path[0] == '\0')
	{
		return 0;
	}

	std::string filespec(path);

	if (escapeSpecialChars)
	{
		str_replace(filespec, "%", "%25");
		str_replace(filespec, "@", "%40");
		str_replace(filespec, "#", "%23");
		str_replace(filespec, "*", "%2A");
	}

	if (pathType == PERFORCE_PATH_FOLDER)
	{
		std::string::size_type length = filespec.length();
		if (filespec[length - 1] == '/' || filespec[length - 1] == '\\')
		{
			filespec.append("...");
		}
		else
		{
			filespec.append("/...");
		}
	}

	return filespec;
}

PerforceErrorValue perforceAdd(const char *relpath, PerforcePathType pathType)
{
	PERFORCE_LAZY_INIT(PERFORCE_ERROR_NO_SC);

	if(perforceQueryIsFileMine(relpath))
	{
		return PERFORCE_NO_ERROR;
	}

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, false, pathType).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		&buf[0],
	};
	AddClientUser ui;
	if (!tryRunCommand("add", ARRAYSIZE(argv), argv, ui))
	{
		return PERFORCE_ERROR_NO_SC;
	}

	devassertmsg(ui.success || ui.alreadyExists || ui.alreadyDeleted, 
		"perforceAdd failed for file \"%s\"", fullpath);
#if 0 // in case need to remove the assertion, enable this printf
	if( !(ui.success || ui.alreadyExists || ui.alreadyDeleted) ) {
		printf("%s -- ERROR adding file to perforce\n", fullpath);
	}
#endif

	if (ui.alreadyDeleted)
	{
		return PERFORCE_ERROR_ALREADY_DELETED;
	}

	return PERFORCE_NO_ERROR;
}

PerforceErrorValue perforceEdit(const char *relpath, PerforcePathType pathType)
{
	PERFORCE_LAZY_INIT(PERFORCE_ERROR_NO_SC);

	if(perforceQueryIsFileMine(relpath))
	{
		return PERFORCE_NO_ERROR;
	}

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, pathType).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		&buf[0],
	};
	EditClientUser ui;
	if (!tryRunCommand("edit", ARRAYSIZE(argv), argv, ui))
	{
		return PERFORCE_ERROR_NO_SC;
	}

	devassertmsg(ui.success || ui.missing || ui.alreadyDeleted || ui.alreadyLocked, 
		"perforceEdit failed for file \"%s\"", fullpath);
	if (ui.alreadyDeleted)
	{
		return PERFORCE_ERROR_ALREADY_DELETED;
	}

	if (ui.alreadyLocked)
	{
		return PERFORCE_ERROR_NOTLOCKEDBYYOU;
	}

	return ui.success ? PERFORCE_NO_ERROR : PERFORCE_ERROR_NOT_IN_DB;
}

static bool tryResolveFiles(const char *relpath, PerforcePathType pathType)
{
	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, pathType).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		"-am",
		&buf[0],
	};
	ClientUser ui;
	if (!tryRunCommand("resolve", ARRAYSIZE(argv), argv, ui))
	{
		return false;
	}

	TestResolveClientUser testUI;
	if (!tryRunCommand("resolve", ARRAYSIZE(argv), argv, testUI))
	{
		return false;
	}

	return testUI.success;
}

PerforceErrorValue perforceSubmit(const char *relpath, PerforcePathType pathType, const char *description)
{
	if (getBlockString(relpath))
	{
		return PERFORCE_ERROR_NOTLOCKEDBYYOU;
	}

	PERFORCE_LAZY_INIT(PERFORCE_ERROR_NO_SC);

	if (!tryResolveFiles(relpath, pathType))
	{
		return PERFORCE_ERROR_MERGE_CONFLICT;
	}

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, pathType).c_str();
	std::vector<char> buf(strlen(description) + 1);
	std::copy(description, description + strlen(description), buf.begin());
	std::vector<char> buf2(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf2.begin());
	char *argv[] = {
		"-d",
		&buf[0],
		&buf2[0],
	};
	SubmitClientUser ui;
	if (!tryRunCommand("submit", ARRAYSIZE(argv), argv, ui))
	{
		return PERFORCE_ERROR_NO_SC;
	}

	devassertmsg(ui.success, "perforceSubmit failed for file \"%s\"", fullpath);

	return ui.success ? PERFORCE_NO_ERROR : PERFORCE_ERROR_NOT_IN_DB;
}

PerforceErrorValue perforceDelete(const char *relpath, PerforcePathType pathType)
{
	PERFORCE_LAZY_INIT(PERFORCE_ERROR_NO_SC);

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, pathType).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		&buf[0],
	};
	DeleteClientUser ui;
	if (!tryRunCommand("delete", ARRAYSIZE(argv), argv, ui))
	{
		return PERFORCE_ERROR_NO_SC;
	}

	devassertmsg(ui.success || ui.missing, 
		"perforceDelete failed for file \"%s\"", fullpath);

	return ui.success ? PERFORCE_NO_ERROR : PERFORCE_ERROR_NOT_IN_DB;
}

PerforceErrorValue perforceSync(const char *relpath, PerforcePathType pathType)
{
	PERFORCE_LAZY_INIT(PERFORCE_ERROR_NO_SC);

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, pathType).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		&buf[0],
	};
	SyncClientUser ui;
	if (!tryRunCommand("sync", ARRAYSIZE(argv), argv, ui))
	{
		return PERFORCE_ERROR_NO_SC;
	}

	devassertmsg(ui.success || ui.missing, 
		"perforceSync failed for file \"%s\"", fullpath);

	return ui.success ? PERFORCE_NO_ERROR : PERFORCE_ERROR_NOT_IN_DB;
}

PerforceErrorValue perforceSyncForce(const char *relpath, PerforcePathType pathType)
{
	PERFORCE_LAZY_INIT(PERFORCE_ERROR_NO_SC);

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, pathType).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		"-f",
		&buf[0],
	};
	SyncClientUser ui;
	if (!tryRunCommand("sync", ARRAYSIZE(argv), argv, ui))
	{
		return PERFORCE_ERROR_NO_SC;
	}

	devassertmsg(ui.success || ui.missing, 
		"perforceSyncForce failed for file \"%s\"", fullpath);

	return ui.success ? PERFORCE_NO_ERROR : PERFORCE_ERROR_NOT_IN_DB;
}

PerforceErrorValue perforceRevert(const char *relpath, PerforcePathType pathType)
{
	PERFORCE_LAZY_INIT(PERFORCE_ERROR_NO_SC);

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, pathType).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		&buf[0],
	};
	RevertClientUser ui;
	if (!tryRunCommand("revert", ARRAYSIZE(argv), argv, ui))
	{
		return PERFORCE_ERROR_NO_SC;
	}

	devassertmsg( ui.success, "perforceRevert failed for file \"%s\"", fullpath); // adjust if unhandled case causes assert to trip

	return ui.success ? PERFORCE_NO_ERROR : PERFORCE_ERROR_NOT_IN_DB;
}

static PerforceErrorValue tryRunGuiCommand(const char *command, const char *relpath)
{
	PERFORCE_LAZY_INIT(PERFORCE_ERROR_NO_SC);

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	char buf[1024];
	PROCESS_INFORMATION res_pi;
	sprintf_s(buf, sizeof(buf), "\"%s\" -p %s -c %s -u %s -cmd \"%s %s\"", p4v_exe_path, sPerforcePort, sPerforceClientName, sPerforceUser, command, fullpath);
	if (!winCreateProcess(buf, &res_pi))
	{
		switch (GetLastError()) {
			case ERROR_FILE_NOT_FOUND:
				return PERFORCE_ERROR_NO_P4V;
		}
		return PERFORCE_ERROR_COMMAND_FAILED;
	}

	CloseHandle(res_pi.hThread);
	CloseHandle(res_pi.hProcess);

	return PERFORCE_NO_ERROR;
}

PerforceErrorValue perforceGuiSubmitFile(const char *relpath)
{
	return tryRunGuiCommand("submit", relpath);
}

PerforceErrorValue perforceGuiHistory(const char *relpath)
{
	return tryRunGuiCommand("history", relpath);
}

int perforceQueryIsFileNew(const char *relpath)
{
	PERFORCE_LAZY_INIT(PERFORCE_ERROR_NO_SC);

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, PERFORCE_PATH_FILE).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		&buf[0],
	};
	NewClientUser ui;
	if (!tryRunCommand("fstat", ARRAYSIZE(argv), argv, ui))
	{
		// true/false return code
		return 0; //PERFORCE_ERROR_NO_SC;
	}

	// treat head deleted as new 
	return ui.isNew || ui.headDeleted;
}

int perforceQueryIsFileLockedByMeOrNew(const char *relpath)
{
	return perforceQueryIsFileMine(relpath) || perforceQueryIsFileNew(relpath);
}

int perforceQueryIsFileMine(const char *relpath)
{
	PERFORCE_LAZY_INIT(PERFORCE_ERROR_NO_SC);

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, PERFORCE_PATH_FILE).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		&buf[0],
	};
	FileLockClientUser ui;
	if (!tryRunCommand("fstat", ARRAYSIZE(argv), argv, ui))
	{
		// true/false return code
		return 0; //PERFORCE_ERROR_NO_SC;
	}

	return ui.locked;
}

const char *perforceQueryLastAuthor(const char *relpath)
{
	PERFORCE_LAZY_INIT(PES(PERFORCE_ERROR_NO_SC));

	if (perforceQueryIsFileMine(relpath))
	{
		return sPerforceUser;
	}

	if (!perforceQueryIsFileLatest(relpath))
	{
		return perforce_gla_not_latest;
	}

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, PERFORCE_PATH_FILE).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		"-m",
		"1",
		&buf[0],
	};
	LastAuthorClientUser ui;
	if (!tryRunCommand("changes", ARRAYSIZE(argv), argv, ui))
	{
		return PES(PERFORCE_ERROR_COMMAND_FAILED);
	}

	static char author[128];
	strcpy(author, ui.author);
	return author[0] ? author : perforce_gla_error;
}

int perforceQueryIsFileLatest(const char *relpath)
{
	PERFORCE_LAZY_INIT(1);

	char fullpath[MAX_PATH];
	fileLocateWrite(relpath, fullpath);

	std::string filespec = formPerforceFilespec(fullpath, true, PERFORCE_PATH_FILE).c_str();
	std::vector<char> buf(filespec.size() + 1);
	std::copy(filespec.begin(), filespec.end(), buf.begin());
	char *argv[] = {
		&buf[0],
	};
	FileLatestClientUser ui;
	if (!tryRunCommand("fstat", ARRAYSIZE(argv), argv, ui))
	{
		return 1;
	}

	if(ui.modTime == -1)
	{
		// file is not in source control.  Should probably query for this case before checking to see if latest?
		//	Currently perforceQueryLastAuthor relies on this returning true so that it can return perforce_gla_error
		//	to indicate that a file is not in source control.  
		return 1;
	}

	// DST can be buggy (allow diff of 1 or 2 hrs exactly)
	int deltaTime = abs(ui.modTime - fileLastChanged(fullpath));
	bool matchesModTime = ( (deltaTime == 0) || (deltaTime == 3600) || (deltaTime == 7200) );
	return ui.haveEqualsHead && matchesModTime;
}

const char *perforceQueryUserName(void)
{
	PERFORCE_LAZY_INIT(PES(PERFORCE_ERROR_NO_SC));

	return sPerforceUser;
}

int perforceQueryAvailable(void) // Whether or not source control is available
{
	PERFORCE_LAZY_INIT(0);

	return 1;
}

const char *perforceQueryBranchName(const char *localpath)
{
	PERFORCE_LAZY_INIT(PES(PERFORCE_ERROR_NO_SC));
	static char out[1000];

	char *argv[] = {(char*)localpath};		
	DirPathClientUser ui;
	if (!tryRunCommand("dirs", ARRAYSIZE(argv), argv, ui))
	{
		return PES(PERFORCE_ERROR_COMMAND_FAILED);
	}

	strcpy(out, ui.path);
	return out;
}

const char * const * perforceQueryGroupList(const char *username)
{
	static const char * const * ret = NULL;
	if (ret)
		return ret;

	PERFORCE_LAZY_INIT(0);

	std::string copy(username);
	std::vector<char> buf(copy.size() + 1);
	std::copy(copy.begin(), copy.end(), buf.begin());
	char *argv[] = {
		&buf[0],
	};
	GroupListClientUser ui;
	if (!tryRunCommand("groups", ARRAYSIZE(argv), argv, ui))
	{
		return 0;
	}

	ret = ui.groups;
	return ret;
}

int attemptToCheckOut( char * fileName, int force_checkout )
{
	int checkedOut = 0;

	if(perforceQueryIsFileMine(fileName))
	{
		checkedOut = LAYER_IS_CHECKED_OUT_TO_ME;
	}
	else if(perforceQueryIsFileNew(fileName))
	{
		// add the file to source control
		perforceAdd(fileName, PERFORCE_PATH_FILE);
		checkedOut = LAYER_IS_CHECKED_OUT_TO_ME;
	}
	else if (!force_checkout && !perforceQueryIsFileLatest(fileName))
	{
		// The user doesn't have the latest version of the file, do not let them edit it!
		// If we were to check out the file here, the file would be changed on disk, but not reloaded,
		//   and that would be bad!  Someone else's changes would most likely be lost.
		Errorf("Error: %s can't be checked out! \nSomeone else has changed it since you last got latest.\n  Exit, get latest and reload the map.", fileName!=NULL?fileName:"NULL");
		checkedOut = 0;
		//TO DO : fire a reload the map command
	}
	else
	{
		if (force_checkout)
		{
			perforceSyncForce(fileName, PERFORCE_PATH_FILE);
		}
		else
		{
			perforceSync(fileName, PERFORCE_PATH_FILE);
		}

		PerforceErrorValue ret = perforceEdit(fileName, PERFORCE_PATH_FILE);
		if( ret==PERFORCE_ERROR_NO_SC )
		{
			//layer is local only pretend it's checked out
			//sendEditMessage(NULL, 2, "CHECK OUT OK: %s \n You are not connected source control.  You are working only locally.", fileName!=NULL?fileName:"NULL" );
			checkedOut = LAYER_IS_CHECKED_OUT_TO_ME;
		}
		else if( ret == PERFORCE_ERROR_NOT_IN_DB )
		{
			// Not in the db, it will be added when we save it
			// pretend it's checked out
			//sendEditMessage(NULL, 2, "CHECK OUT OK: %s \n This file is new and not under source control yet.", fileName!=NULL?fileName:"NULL", perforceOfflineGetErrorString(ret) );
			checkedOut = LAYER_IS_CHECKED_OUT_TO_ME;
		}
		else if ( ret ) //Any other error message
		{
			Errorf("Error: %s \n can't be checked out. \n (%s).\n Your changes to this file won't be saved!", fileName!=NULL?fileName:"NULL", perforceOfflineGetErrorString(ret) );
			checkedOut = 0;
		}
		else
		{
			//sendEditMessage(NULL, 2, "CHECK OUT OK: %s\n Checked out by %s ", fileName!=NULL?fileName:"UNNAMED", perforceQueryUserName());
			checkedOut = LAYER_IS_CHECKED_OUT_TO_ME;
		}
	}

	return checkedOut;
}

#endif
