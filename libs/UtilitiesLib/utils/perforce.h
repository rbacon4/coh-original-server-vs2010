#pragma once

typedef enum PerforceErrorValue {
	PERFORCE_NO_ERROR=0,
	PERFORCE_ERROR_COMMAND_FAILED=1,
	PERFORCE_ERROR_NOTLOCKEDBYYOU=7,
	PERFORCE_ERROR_ALREADY_CHECKEDOUT=9,
	PERFORCE_ERROR_NOT_IN_DB=10,
	PERFORCE_ERROR_FILENOTFOUND=11,
	PERFORCE_ERROR_NO_SC=12,
	PERFORCE_ERROR_ALREADY_DELETED=13,
	PERFORCE_ERROR_MERGE_CONFLICT=19,
	PERFORCE_ERROR_NO_P4V=20,
} PerforceErrorValue;

typedef enum PerforcePathType {
	PERFORCE_PATH_FILE,
	PERFORCE_PATH_FOLDER,
} PerforcePathType;

#define LAYER_IS_CHECKED_OUT_TO_ME				1
#define PERFORCE_BRANCH_UNKNOWN -2


#ifdef __cplusplus
extern "C" {
#endif

const char *perforceOfflineGetErrorString(PerforceErrorValue error);
PerforceErrorValue perforceOfflineBlockFile(const char *relpath, const char *block_string);
PerforceErrorValue perforceOfflineUnblockFile(const char *relpath);
int			perforceOfflineQueryIsFileBlocked(const char *relpath);

void perforceDisable(int disable);

PerforceErrorValue perforceAdd(const char *relpath, PerforcePathType pathType);
PerforceErrorValue perforceEdit(const char *relpath, PerforcePathType pathType);
PerforceErrorValue perforceSubmit(const char *relpath, PerforcePathType pathType, const char *description);
PerforceErrorValue perforceDelete(const char *relpath, PerforcePathType pathType);
PerforceErrorValue perforceSync(const char *relpath, PerforcePathType pathType);
PerforceErrorValue perforceSyncForce(const char *relpath, PerforcePathType pathType);
PerforceErrorValue perforceRevert(const char *relpath, PerforcePathType pathType);

PerforceErrorValue perforceGuiSubmitFile(const char *relpath);
PerforceErrorValue perforceGuiHistory(const char *relpath);

int			perforceQueryIsFileLockedByMeOrNew(const char *relpath);
int			perforceQueryIsFileMine(const char *relpath);

const char *perforceQueryLastAuthor(const char *relpath);
int			perforceQueryIsFileLatest(const char *relpath);
int			perforceQueryIsFileNew(const char *relpath);
const char *perforceQueryUserName(void);
int			perforceQueryAvailable(void); // Whether or not source control is available

const char *perforceQueryBranchName(const char *localpath);
const char *const *perforceQueryGroupList(const char *username);

int attemptToCheckOut(char *filename, int force);

#ifdef __cplusplus
}
#endif
