/*
 *Connor Cormier
 *5/31/14
 *Database client interface, used for interacting with SQLite database
 */

#ifndef DATABASECLIENT_H
#define DATABASECLIENT_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include "evernote_api/src/NoteStore.h"
#include <stdexcept>

#define DB_FLAGS SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
//#define WAIT_TIME 1000

class DatabaseClient
{
public:
	DatabaseClient(const std::string& dbFile);
	~DatabaseClient();

	//Methods to get update count and sync time from db
	evernote::edam::Timestamp getSyncTime();
	int getLastUpdateCount();

	//Setters for aforementioned values
	void setSyncTime(const evernote::edam::Timestamp& time);
	void setLastUpdateCount(int updateCount);

	//Notebook retrieval methods. Return value indicates success.
	bool getNotebooks(std::vector<evernote::edam::Notebook>& ret);
	bool getNotebookByGuid(evernote::edam::Notebook& ret, const evernote::edam::Guid& guid);
	bool getNotebookByName(evernote::edam::Notebook& ret, const std::string& name);

	//Notebook alteration methods. Return value indicates success.
	bool addNotebook(const evernote::edam::Notebook& notebook);
	bool updateNotebook(const evernote::edam::Notebook& notebook);
	bool removeNotebook(const evernote::edam::Notebook& notebook);
	bool renameNotebook(evernote::edam::Notebook& notebook);

	//Special notebook tasks. Always returns true (no fail case, returns bool for consistency)
	bool getNotesInNotebook(std::vector<evernote::edam::Note>& ret, const evernote::edam::Notebook& notebook);
	bool getNotesMetadataInNotebook(evernote::edam::NotesMetadataList& ans, const evernote::edam::Notebook& notebook);
	bool deleteNotesInNotebook(const evernote::edam::Notebook& notebook);

	//Note retrieval methods. Return value indicates success.
	bool getNotes(std::vector<evernote::edam::Note>& ret);
	bool getNoteByGuid(evernote::edam::Note& ret, const evernote::edam::Guid& guid);

	//Note alteration methods. Return value indicates success.
	bool addNote(const evernote::edam::Note& note);
	bool updateNote(const evernote::edam::Note& note);
	bool removeNote(const evernote::edam::Note& note);
	bool deleteNote(const evernote::edam::Note& note);
	bool purgeDeletedNotes();

	//Special note tasks
	bool getResourcesInNote(std::vector<evernote::edam::Resource>& ret, const evernote::edam::Note& note, bool withData);
	bool removeResourcesInNote(const evernote::edam::Note& note);
	
	//Resource retrieval methods
	bool getResourceByGuid(evernote::edam::Resource& ret, const evernote::edam::Guid& guid, bool withData);

	//Resource alteration methods
	bool addResource(const evernote::edam::Resource& res);
	bool updateResource(const evernote::edam::Resource& res);
	bool removeResource(const evernote::edam::Resource& res);

	//Special dirty methods for synchronization. Note: if no dirty resources in note, leave note resource field unset for the sync. Otherwise, merge the vectors filled by these two methods
	bool getDirtyResourcesInNote(std::vector<evernote::edam::Resource>& ret, const evernote::edam::Note& note);
	bool getCleanResourcesInNoteNoData(std::vector<evernote::edam::Resource>& ret, const evernote::edam::Note& note);

	//Dirty query methods. Return value, as always, indicates success.
	bool getDirtyNotebooks(std::vector<evernote::edam::Notebook>& ret);
	bool getDirtyNotes(std::vector<evernote::edam::Note>& ret);
	
	//Methods to check if dirty
	bool isNotebookDirty(const evernote::edam::Notebook& notebook);
	bool isNoteDirty(const evernote::edam::Note& note);
	bool isResourceDirty(const evernote::edam::Resource& res);

	//Setters for dirty flag in database. The "flag" and "unflag" methods are just more explicit ways of invoking these
	bool setDirty(const evernote::edam::Notebook& notebook, bool dirty);
	bool setDirty(const evernote::edam::Note& note, bool dirty);
	bool setDirty(const evernote::edam::Resource& res, bool dirty);

	//Methods to flag dirty. Return value indicates operation success.
	bool flagDirty(const evernote::edam::Notebook& notebook);
	bool flagDirty(const evernote::edam::Note& note);
	bool flagDirty(const evernote::edam::Resource& res);
	bool flagNotesInNotebookDirty(const evernote::edam::Notebook& notebook);

	//Methods to unflag dirty. Return values indicates operation success.
	bool unflagDirty(const evernote::edam::Notebook& notebook);
	bool unflagDirty(const evernote::edam::Note& note);
	bool unflagDirty(const evernote::edam::Resource& res);

private:
	//The database connection "object"
	sqlite3 *db;

	//Sqlite prepared statements. Created with database and continuously reused. Destroyed with object
	sqlite3_stmt *sGetSyncTime,
			*sGetLastUpdateCount,
			*sSetSyncTime,
			*sSetLastUpdateCount,
			*sGetNotebooks,
			*sGetNotebookByGuid,
			*sGetNotebookByName,
			*sAddNotebook,
			*sUpdateNotebook,
			*sRemoveNotebook,
			*sGetNotesInNotebook,
			*sGetNotesMetadataInNotebook,
			*sDeleteNotesInNotebook,
			*sGetNotes,
			*sGetNoteByGuid,
			*sAddNote,
			*sUpdateNote,
			*sRemoveNote,
			*sDeleteNote,
			*sPurgeDeletedNotes,
			*sGetResourcesInNote,
			*sGetResourcesInNoteNoData,
			*sRemoveResourcesInNote,
			*sGetResourceByGuid,
			*sGetResourceByGuidNoData,
			*sAddResource,
			*sUpdateResource,
			*sRemoveResource,
			*sGetDirtyResourcesInNote,
			*sGetCleanResourcesInNoteNoData,
			*sGetDirtyNotebooks,
			*sGetDirtyNotes,
			*sIsNotebookDirty,
			*sIsNoteDirty,
			*sIsResourceDirty,
			*sSetNotebookDirty,
			*sSetNoteDirty,
			*sSetResourceDirty,
			*sFlagNotesInNotebookDirty;


	//Method to execute statement synchronously with no result returned
	bool execSyncNoRes(const std::string& stat);
	bool execSyncNoRes(sqlite3_stmt *stat);

	//Database management methods. Validate that tables are as expected and recreate if not
	bool validateTables();

	//Prepare all prepared statements at start (after validation) and finalize at end
	void prepareStatements();
	void finalizeStatements();

	//Reset and unbind statment. Just a convenience wrapper meant to cut down code length
	void sReset(sqlite3_stmt *stat);

	//Helper methods to set appropriate fields in notes or notebooks
	void prepareNotebook(evernote::edam::Notebook& notebook);
	void prepareNote(evernote::edam::Note& note);
	void prepareResource(evernote::edam::Resource& resource, bool withData);

	//Methods to fetch notes and notebooks from database once statement has been prepared
	//These DO NOT reset or unbind statements
	void fetchNotebooks(std::vector<evernote::edam::Notebook>& ret, sqlite3_stmt *readyStat);
	void fetchNotes(std::vector<evernote::edam::Note>& ret, sqlite3_stmt *readyStat);
	void fetchResources(std::vector<evernote::edam::Resource>& ret, sqlite3_stmt *readyStat, bool withData);

	void bindNotebook(sqlite3_stmt *stat, const evernote::edam::Notebook& notebook, bool dirty = false);
	void bindNote(sqlite3_stmt *stat, const evernote::edam::Note& note, bool dirty = false);
	void bindResource(sqlite3_stmt *stat, const evernote::edam::Resource& resource, bool dirty = false);
};

//For use with database
class DatabaseException : public std::runtime_error
{
public:
	DatabaseException(const std::string& str="Generic Database Exception") : std::runtime_error(str) {}
};

#endif
