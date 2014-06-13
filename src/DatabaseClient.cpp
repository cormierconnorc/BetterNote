/*
 *Connor Cormier
 *5/31/14
 *Client to handle connection to SQLite database, where notes are stored
 */

#include "DatabaseClient.h"
#include <iostream>
#include <sstream>
#include <ctime>

using namespace std;
using namespace evernote::edam;

DatabaseClient::DatabaseClient(const string& dbFile)
{
	//Open the database
	int retCode = sqlite3_open_v2(dbFile.c_str(), &db, DB_FLAGS, NULL);

	if (retCode != SQLITE_OK)
	{
		string except = sqlite3_errmsg(db);
		string error = "Failed to open database: " + except;
		sqlite3_close_v2(db);
		throw DatabaseException(error);
	}

	
	//First task: validate tables
	validateTables();

	//Next, prepare statements
	prepareStatements();
}

DatabaseClient::~DatabaseClient()
{
	//Finalize prepared statements
	finalizeStatements();
	
	sqlite3_close_v2(db);
}

Timestamp DatabaseClient::getSyncTime()
{
	//Execute (single step) query
	sqlite3_step(sGetSyncTime);

	//Get value
	sqlite3_int64 stamp = sqlite3_column_int64(sGetSyncTime, 0);

	//Reset statement
	sReset(sGetSyncTime);
	
	return (Timestamp)stamp;
}

int DatabaseClient::getLastUpdateCount()
{
	sqlite3_step(sGetLastUpdateCount);
	int val = sqlite3_column_int(sGetLastUpdateCount, 0);
	sReset(sGetLastUpdateCount);
	return val;
}

void DatabaseClient::setSyncTime(const Timestamp& time)
{
	sqlite3_bind_int64(sSetSyncTime, 1, time);
	sqlite3_step(sSetSyncTime);
	sReset(sSetSyncTime);
}

void DatabaseClient::setLastUpdateCount(int updateCount)
{
	sqlite3_bind_int(sSetLastUpdateCount, 1, updateCount);
	sqlite3_step(sSetLastUpdateCount);
	sReset(sSetLastUpdateCount);
}

bool DatabaseClient::getNotebooks(vector<Notebook>& ret)
{
	fetchNotebooks(ret, sGetNotebooks);
	sReset(sGetNotebooks);

	//Can't report failure with current model
	return true;
}

bool DatabaseClient::getNotebookByGuid(Notebook& ret, const Guid& guid)
{
	//Bind guid to statement
	sqlite3_bind_text(sGetNotebookByGuid, 1, guid.c_str(), -1, SQLITE_STATIC);

	vector<Notebook> books;
	fetchNotebooks(books, sGetNotebookByGuid);

	bool thingsAreGood = books.size() > 0;

	if (thingsAreGood)
		ret = books[0];
	
	//Reset and unbind
	sReset(sGetNotebookByGuid);

	return thingsAreGood;
}

bool DatabaseClient::getNotebookByName(Notebook& ret, const string& name)
{
	//Bind guid to statement
	sqlite3_bind_text(sGetNotebookByName, 1, name.c_str(), -1, SQLITE_STATIC);

	vector<Notebook> books;
	fetchNotebooks(books, sGetNotebookByName);

	bool weGood = books.size() > 0;

	if (weGood)
		ret = books[0];

	//Reset and unbind
	sReset(sGetNotebookByName);

	return weGood;
}

bool DatabaseClient::addNotebook(const Notebook& notebook)
{
	//Bind values to statement
	bindNotebook(sAddNotebook, notebook);

	//Execute statement
	bool ret = execSyncNoRes(sAddNotebook);

	//Reset and unbind
	sReset(sAddNotebook);

	return ret;
}

bool DatabaseClient::updateNotebook(const Notebook& notebook)
{
	//Bind values to statement
	bindNotebook(sUpdateNotebook, notebook, isNotebookDirty(notebook));
	
	//Now bind on the guid to look for
	sqlite3_bind_text(sUpdateNotebook, 7, notebook.guid.c_str(), -1, SQLITE_STATIC);

	//Execute the statement
	bool ret = execSyncNoRes(sUpdateNotebook);

	sReset(sUpdateNotebook);

	return ret;	
}

bool DatabaseClient::removeNotebook(const Notebook& notebook)
{
	sqlite3_bind_text(sRemoveNotebook, 1, notebook.guid.c_str(), -1, SQLITE_STATIC);

	bool ret = execSyncNoRes(sRemoveNotebook);

	sReset(sRemoveNotebook);

	return ret;
}

bool DatabaseClient::renameNotebook(Notebook& notebook)
{
	string newName = notebook.name;
	Notebook worthless;
	

	//Keep trying to add a number onto the end until no more conflict exists (conflicts between a notebook and itself cannot exist, as notebooks cannot experience cognitive dissonance)
	for (int i = 1; getNotebookByName(worthless, newName) && worthless.guid != notebook.guid; i++)
	{
		ostringstream oss;
		oss << notebook.name << " (" << i << ")";
		newName = oss.str();
	}

	//No more conflict
	notebook.name = newName;

	//So update it
	bool success = updateNotebook(notebook);

	return success;
}

bool DatabaseClient::getNotesInNotebook(vector<Note>& ret, const Notebook& notebook)
{
	//Bind notbook guid to statement
	sqlite3_bind_text(sGetNotesInNotebook, 1, notebook.guid.c_str(), -1, SQLITE_STATIC);

	//Now execute the statement and get the result
	fetchNotes(ret, sGetNotesInNotebook);

	sReset(sGetNotesInNotebook);

	//No failure notifications possible with this model. May need to fix.
	return true;
}

bool DatabaseClient::getNotesMetadataInNotebook(NotesMetadataList& ans, const Notebook& notebook)
{
	//The name's too long, so here's a solution
	sqlite3_stmt *s = sGetNotesMetadataInNotebook;
	
	//Bind notebook guid
	sqlite3_bind_text(s, 1, notebook.guid.c_str(), -1, SQLITE_STATIC);

	//Empty notes from list
	ans.notes.clear();

	//Now run the query and build the new list
	while (sqlite3_step(s) == SQLITE_ROW)
	{
		NoteMetadata nm;

		//Set the "isset" fields
		nm.__isset.title = true;
		nm.__isset.notebookGuid = true;

		//Now set the values to the columns from the query
		nm.guid = (char *)sqlite3_column_text(s, 0);
		nm.title = (char *)sqlite3_column_text(s, 1);
		nm.deleted = sqlite3_column_int64(s, 2);

		if (nm.deleted < 0)
			nm.__isset.deleted = false;
		else
			nm.__isset.deleted = true;

		nm.updateSequenceNum = sqlite3_column_int(s, 3);

		if (nm.updateSequenceNum < 0)
			nm.__isset.updateSequenceNum = false;
		else
			nm.__isset.updateSequenceNum = true;

		nm.notebookGuid = (char *)sqlite3_column_text(s, 4);
		
		//Place it in the list
		ans.notes.push_back(nm);
	}
	
	sReset(s);

	return true;
}

bool DatabaseClient::deleteNotesInNotebook(const Notebook& notebook)
{
	//Bind time of deletion
	//Move away from ctime?
	sqlite3_bind_int64(sDeleteNotesInNotebook, 1, time(NULL) * 1000);

	//Bind notebook guid
	sqlite3_bind_text(sDeleteNotesInNotebook, 2, notebook.guid.c_str(), -1, SQLITE_STATIC);

	bool ret = execSyncNoRes(sDeleteNotesInNotebook);

	sReset(sDeleteNotesInNotebook);

	return ret;
}

bool DatabaseClient::getNotes(vector<Note>& ret)
{
	fetchNotes(ret, sGetNotes);
	sReset(sGetNotes);

	//May need to change
	return true;
}

bool DatabaseClient::getNoteByGuid(Note& ret, const Guid& guid)
{
	//Bind note guid to statement
	sqlite3_bind_text(sGetNoteByGuid, 1, guid.c_str(), -1, SQLITE_STATIC);

	//Do the fetch
	vector<Note> notes;
	fetchNotes(notes, sGetNoteByGuid);

	bool good = notes.size() > 0;

	if (good)
		ret = notes[0];

	sReset(sGetNoteByGuid);

	return good;
}

bool DatabaseClient::addNote(const Note& note)
{
	//Bind note to statement
	bindNote(sAddNote, note);

	//Execute without result, getting statement success
	bool ret = execSyncNoRes(sAddNote);

	sReset(sAddNote);

	return ret;	
}

bool DatabaseClient::updateNote(const Note& note)
{
	//Bind note to statement
	bindNote(sUpdateNote, note, isNoteDirty(note));

	//Bind guid of note
	sqlite3_bind_text(sUpdateNote, 8, note.guid.c_str(), -1, SQLITE_STATIC);

	bool ret = execSyncNoRes(sUpdateNote);
	
	sReset(sUpdateNote);

	return ret;
}

bool DatabaseClient::removeNote(const Note& note)
{
	//Bind note guid to remove
	sqlite3_bind_text(sRemoveNote, 1, note.guid.c_str(), -1, SQLITE_STATIC);

	bool ret = execSyncNoRes(sRemoveNote);

	sReset(sRemoveNote);

	return ret;
}

bool DatabaseClient::deleteNote(const Note& note)
{
	//Bind time of deletion
	//Move away from ctime?
	sqlite3_bind_int64(sDeleteNote, 1, time(NULL) * 1000);

	//Bind notebook guid
	sqlite3_bind_text(sDeleteNote, 2, note.guid.c_str(), -1, SQLITE_STATIC);

	bool ret = execSyncNoRes(sDeleteNote);

	sReset(sDeleteNote);

	return ret;
}

bool DatabaseClient::purgeDeletedNotes()
{
	bool ret = execSyncNoRes(sPurgeDeletedNotes);
	sReset(sPurgeDeletedNotes);
	return ret;
}

bool DatabaseClient::getResourcesInNote(vector<Resource>& ret, const Note& note, bool withData)
{
	//Select appropriate prepared statement
	sqlite3_stmt *s = (withData ? sGetResourcesInNote : sGetResourcesInNoteNoData);

	//Bind note guid
	sqlite3_bind_text(s, 1, note.guid.c_str(), -1, SQLITE_STATIC);

	fetchResources(ret, s, true);

	sReset(s);

	return true;
}

bool DatabaseClient::removeResourcesInNote(const Note& note)
{
	sqlite3_stmt *s = sRemoveResourcesInNote;

	sqlite3_bind_text(s, 1, note.guid.c_str(), -1, SQLITE_STATIC);

	bool ret = execSyncNoRes(s);

	sReset(s);

	return ret;
}

bool DatabaseClient::getResourceByGuid(Resource& ret, const Guid& guid, bool withData)
{
	//Select appropriate prepared statement
	sqlite3_stmt *s = (withData ? sGetResourceByGuid : sGetResourceByGuidNoData);

	//Bind guid
	sqlite3_bind_text(s, 1, guid.c_str(), -1, SQLITE_STATIC);

	vector<Resource> retVec;
	
	fetchResources(retVec, s, true);

	sReset(s);

	if (retVec.size() > 0)
	{
		ret = retVec[0];
		return true;
	}

	return false;
}

bool DatabaseClient::addResource(const Resource& res)
{
	bindResource(sAddResource, res);

	bool ret = execSyncNoRes(sAddResource);

	sReset(sAddResource);

	return ret;
}

bool DatabaseClient::updateResource(const evernote::edam::Resource& res)
{
	bindResource(sUpdateResource, res, isResourceDirty(res));

	//Bind guid
	sqlite3_bind_text(sUpdateResource, 9, res.guid.c_str(), -1, SQLITE_STATIC);

	bool ret = execSyncNoRes(sUpdateResource);

	sReset(sUpdateResource);

	return ret;
}

bool DatabaseClient::removeResource(const Resource& res)
{
	sqlite3_stmt *s = sRemoveResource;

	sqlite3_bind_text(s, 1, res.guid.c_str(), -1, SQLITE_STATIC);

	bool ret = execSyncNoRes(s);

	sReset(s);

	return ret;
}

bool DatabaseClient::getDirtyResourcesInNote(vector<Resource>& ret, const Note& note)
{
	sqlite3_stmt *s = sGetDirtyResourcesInNote;

	//Bind note guid
	sqlite3_bind_text(s, 1, note.guid.c_str(), -1, SQLITE_STATIC);

	fetchResources(ret, s, true);

	sReset(s);

	return true;
}

bool DatabaseClient::getCleanResourcesInNoteNoData(vector<Resource>& ret, const Note& note)
{
	sqlite3_stmt *s = sGetCleanResourcesInNoteNoData;

	//Bind note guid
	sqlite3_bind_text(s, 1, note.guid.c_str(), -1, SQLITE_STATIC);

	fetchResources(ret, s, false);

	sReset(s);

	return true;
}

bool DatabaseClient::getDirtyNotebooks(vector<Notebook>& ret)
{
	fetchNotebooks(ret, sGetDirtyNotebooks);

	sReset(sGetDirtyNotebooks);

	return true;
}

bool DatabaseClient::getDirtyNotes(vector<Note>& ret)
{
	fetchNotes(ret, sGetDirtyNotes);

	sReset(sGetDirtyNotes);

	return true;
}

bool DatabaseClient::isNotebookDirty(const Notebook& notebook)
{
	//Bind the notebook's guid to the appropriate statement
	sqlite3_bind_text(sIsNotebookDirty, 1, notebook.guid.c_str(), -1, SQLITE_STATIC);

	int rc;
	
	if ((rc = sqlite3_step(sIsNotebookDirty)) == SQLITE_ROW)
	{
		bool dirty = (bool)sqlite3_column_int(sIsNotebookDirty, 0);

		sReset(sIsNotebookDirty);

		return dirty;
	}
	else
	{
		sReset(sIsNotebookDirty);
		
		ostringstream oss;
		oss << "Could not get notebook dirty flag. SQLite returned " << rc;		
		throw DatabaseException(oss.str());
	}
}

bool DatabaseClient::isNoteDirty(const Note& note)
{
	//Bind the notebook's guid to the appropriate statement
	sqlite3_bind_text(sIsNoteDirty, 1, note.guid.c_str(), -1, SQLITE_STATIC);

	int rc;
	
	if ((rc = sqlite3_step(sIsNoteDirty)) == SQLITE_ROW)
	{
		bool dirty = (bool)sqlite3_column_int(sIsNoteDirty, 0);

		sReset(sIsNoteDirty);

		return dirty;
	}
	else
	{
		sReset(sIsNoteDirty);
		
		ostringstream oss;
		oss << "Could not get note dirty flag. SQLite returned " << rc;		
		throw DatabaseException(oss.str());
	}
}

bool DatabaseClient::isResourceDirty(const Resource& res)
{
	//Bind the notebook's guid to the appropriate statement
	sqlite3_bind_text(sIsResourceDirty, 1, res.guid.c_str(), -1, SQLITE_STATIC);

	int rc;
	
	if ((rc = sqlite3_step(sIsResourceDirty)) == SQLITE_ROW)
	{
		bool dirty = (bool)sqlite3_column_int(sIsResourceDirty, 0);

		sReset(sIsResourceDirty);

		return dirty;
	}
	else
	{
		sReset(sIsResourceDirty);
		
		ostringstream oss;
		oss << "Could not get note dirty flag. SQLite returned " << rc;		
		throw DatabaseException(oss.str());
	}
}

bool DatabaseClient::setDirty(const Notebook& notebook, bool dirty)
{
	//Bind dirty
	sqlite3_bind_int(sSetNotebookDirty, 1, dirty);
	
	//Bind guid
	sqlite3_bind_text(sSetNotebookDirty, 2, notebook.guid.c_str(), -1, SQLITE_STATIC);

	//Execute without result
	bool ret = execSyncNoRes(sSetNotebookDirty);

	//Reset statement
	sReset(sSetNotebookDirty);
	
	return ret;
}

bool DatabaseClient::setDirty(const Note& note, bool dirty)
{
	//Bind dirty
	sqlite3_bind_int(sSetNoteDirty, 1, dirty);
	
	//Bind guid
	sqlite3_bind_text(sSetNoteDirty, 2, note.guid.c_str(), -1, SQLITE_STATIC);

	//Execute without result
	bool ret = execSyncNoRes(sSetNoteDirty);

	//Reset statement
	sReset(sSetNoteDirty);
	
	return ret;
}

bool DatabaseClient::setDirty(const Resource& res, bool dirty)
{
	//Bind dirty
	sqlite3_bind_int(sSetResourceDirty, 1, dirty);
	
	//Bind guid
	sqlite3_bind_text(sSetResourceDirty, 2, res.guid.c_str(), -1, SQLITE_STATIC);

	//Execute without result
	bool ret = execSyncNoRes(sSetResourceDirty);

	//Reset statement
	sReset(sSetResourceDirty);
	
	return ret;
}

bool DatabaseClient::flagDirty(const Notebook& notebook)
{
	return setDirty(notebook, true);
}

bool DatabaseClient::flagDirty(const Note& note)
{
	return setDirty(note, true);
}

bool DatabaseClient::flagDirty(const Resource& res)
{
	return setDirty(res, true);
}

bool DatabaseClient::flagNotesInNotebookDirty(const Notebook& notebook)
{
	//Convenience alias (I'm feeling a bit lazy)
	sqlite3_stmt *s = sFlagNotesInNotebookDirty;

	sqlite3_bind_text(s, 1, notebook.guid.c_str(), -1, SQLITE_STATIC);

	bool ret = execSyncNoRes(s);

	sReset(s);

	return ret;
}

bool DatabaseClient::unflagDirty(const Notebook& notebook)
{
	return setDirty(notebook, false);
}

bool DatabaseClient::unflagDirty(const Note& note)
{
	return setDirty(note, false);
}

bool DatabaseClient::unflagDirty(const Resource& res)
{
	return setDirty(res, false);
}

bool DatabaseClient::execSyncNoRes(const string& stat)
{
	sqlite3_stmt *nStat;

	if (sqlite3_prepare_v2(db, stat.c_str(), -1, &nStat, 0) != SQLITE_OK)
		return false;

	bool ret = execSyncNoRes(nStat);
	
	sqlite3_finalize(nStat);

	return ret;	
}

bool DatabaseClient::execSyncNoRes(sqlite3_stmt *stat)
{
	int rc;
	
	while ((rc = sqlite3_step(stat)) != SQLITE_DONE)
	{
		//Something broke. Give up.
		if (rc == SQLITE_ERROR || rc == SQLITE_CONSTRAINT || rc == SQLITE_LOCKED)
		{
			cerr << "SQLITE has encountered a problem: " << rc << endl;
			return false;
		}
		//Wait before continuing
		//else if (rc == SQLITE_BUSY)
		//	Glib::usleep(WAIT_TIME);

		cout << rc << endl;
	}

	return true;
}

bool DatabaseClient::validateTables()
{
	sqlite3_stmt *schemaStatement;

	string syncInfoSchema = "CREATE TABLE syncInfo(syncTime integer, lastUpdateCount integer)";
	string noteSchema = "CREATE TABLE notes(guid text primary key, title text, content text, deleted integer, usn integer, notebookGuid text, dirty integer)";
	string notebookSchema = "CREATE TABLE notebooks(guid text primary key, name text, usn integer, isDefault integer, stack text, dirty integer)";
	string resourceSchema = "CREATE TABLE resources(guid text primary key, noteGuid text, bodyHash blob, body blob, mime text, fileName text, usn integer, dirty integer)";
	
	if (sqlite3_prepare_v2(db, "select sql from sqlite_master where type='table' order by name;", -1, &schemaStatement, 0) != SQLITE_OK)
		return false;

	//If statement prepared successfully, start stepping
	
	bool hasSyncInfoTable = false,
			hasNoteTable = false,
			hasNotebookTable = false,
			hasResourceTable = false;
			
	int result;
	int columns = sqlite3_column_count(schemaStatement);

	while ((result = sqlite3_step(schemaStatement)) == SQLITE_ROW)
	{
		for (int i = 0; i < columns; i++)
		{
			string s = (char *)sqlite3_column_text(schemaStatement, i);

			if (s == syncInfoSchema)
				hasSyncInfoTable = true;
			
			if (s == noteSchema)
				hasNoteTable = true;

			if (s == notebookSchema)
				hasNotebookTable = true;

			if (s == resourceSchema)
				hasResourceTable = true;
		}
	}

	//Finalize the prepared statment
	sqlite3_finalize(schemaStatement);

	if (!hasSyncInfoTable)
	{
		cout << "No sync info table found! Creating one now (and inserting start vals)" << endl;
		execSyncNoRes(syncInfoSchema);

		//Insert starting values
		execSyncNoRes("insert into syncInfo values (0, 0)");
	}
	else
		cout << "Found sync info table." << endl;
	
	if (!hasNoteTable)
	{
		cout << "No note table found! Creating one now" << endl;
		execSyncNoRes(noteSchema);
	}
	else
		cout << "Found note table." << endl;

	if (!hasNotebookTable)
	{
		cout << "No notebook table found! Creating one now" << endl;
		execSyncNoRes(notebookSchema);
	}
	else
		cout << "Found notebook table." << endl;

	if (!hasResourceTable)
	{
		cout << "No resource table found! Creating one now" << endl;
		execSyncNoRes(resourceSchema);
	}
	else
		cout << "Found resource table." << endl;

	return true;
}

void DatabaseClient::prepareStatements()
{
	sqlite3_prepare_v2(db, "select syncTime from syncInfo", -1, &sGetSyncTime, 0);
	sqlite3_prepare_v2(db, "select lastUpdateCount from syncInfo", -1, &sGetLastUpdateCount, 0);
	sqlite3_prepare_v2(db, "update syncInfo set syncTime=?", -1, &sSetSyncTime, 0);
	sqlite3_prepare_v2(db, "update syncInfo set lastUpdateCount=?", -1, &sSetLastUpdateCount, 0);
	sqlite3_prepare_v2(db, "select * from notebooks", -1, &sGetNotebooks, 0);
	sqlite3_prepare_v2(db, "select * from notebooks where guid=?", -1, &sGetNotebookByGuid, 0);
	sqlite3_prepare_v2(db, "select * from notebooks where name=?", -1, &sGetNotebookByName, 0);
	sqlite3_prepare_v2(db, "insert into notebooks values (?, ?, ?, ?, ?, ?)", -1, &sAddNotebook, 0);
	sqlite3_prepare_v2(db, "update notebooks set guid=?,name=?,usn=?,isDefault=?,stack=?,dirty=? where guid=?", -1, &sUpdateNotebook, 0);
	sqlite3_prepare_v2(db, "delete from notebooks where guid=?", -1, &sRemoveNotebook, 0);
	sqlite3_prepare_v2(db, "select * from notes where notebookGuid=? and deleted=-1", -1, &sGetNotesInNotebook, 0);
	sqlite3_prepare_v2(db, "select guid,title,deleted,usn,notebookGuid from notes where notebookGuid=? and deleted=-1", -1, &sGetNotesMetadataInNotebook, 0);
	sqlite3_prepare_v2(db, "update notes set deleted=? where notebookGuid=? and deleted=-1", -1, &sDeleteNotesInNotebook, 0);
	sqlite3_prepare_v2(db, "select * from notes where deleted=-1", -1, &sGetNotes, 0);
	sqlite3_prepare_v2(db, "select * from notes where guid=?", -1, &sGetNoteByGuid, 0);
	sqlite3_prepare_v2(db, "insert into notes values (?, ?, ?, ?, ?, ?, ?)", -1, &sAddNote, 0);
	sqlite3_prepare_v2(db, "update notes set guid=?,title=?,content=?,deleted=?,usn=?,notebookGuid=?,dirty=? where guid=?", -1, &sUpdateNote, 0);
	sqlite3_prepare_v2(db, "delete from notes where guid=?", -1, &sRemoveNote, 0);
	sqlite3_prepare_v2(db, "update notes set deleted=? where guid=?", -1, &sDeleteNote, 0);
	sqlite3_prepare_v2(db, "delete from notes where deleted!=-1", -1, &sPurgeDeletedNotes, 0);
	//TODO write and test
	sqlite3_prepare_v2(db, "select * from resources where noteGuid=?", -1, &sGetResourcesInNote, 0);
	sqlite3_prepare_v2(db, "select guid,noteGuid,bodyHash,mime,fileName,usn from resources where noteGuid=?", -1, &sGetResourcesInNoteNoData, 0);
	sqlite3_prepare_v2(db, "delete from resources where noteGuid=?", -1, &sRemoveResourcesInNote, 0);
	sqlite3_prepare_v2(db, "select * from resources where guid=?", -1, &sGetResourceByGuid, 0);
	sqlite3_prepare_v2(db, "select guid,noteGuid,bodyHash,mime,fileName,usn from resources where guid=?", -1, &sGetResourceByGuidNoData, 0);
	sqlite3_prepare_v2(db, "insert into resources values (?, ?, ?, ?, ?, ?, ?, ?)", -1, &sAddResource, 0);
	sqlite3_prepare_v2(db, "update resources set guid=?,noteGuid=?,bodyHash=?,body=?,mime=?,fileName=?,usn=?,dirty=? where guid=?", -1, &sUpdateResource, 0);
	sqlite3_prepare_v2(db, "delete from resources where guid=?", -1, &sRemoveResource, 0);
	sqlite3_prepare_v2(db, "select * from resources where dirty!=0", -1, &sGetDirtyResourcesInNote, 0);
	sqlite3_prepare_v2(db, "select guid,noteGuid,bodyHash,mime,fileName,usn from resources where dirty=0", -1, &sGetCleanResourcesInNoteNoData, 0);
	//END TODO
	sqlite3_prepare_v2(db, "select * from notebooks where dirty!=0", -1, &sGetDirtyNotebooks, 0);
	sqlite3_prepare_v2(db, "select * from notes where dirty!=0", -1, &sGetDirtyNotes, 0);
	sqlite3_prepare_v2(db, "select dirty from notebooks where guid=?", -1, &sIsNotebookDirty, 0);
	sqlite3_prepare_v2(db, "select dirty from notes where guid=?", -1, &sIsNoteDirty, 0);
	//TODO
	sqlite3_prepare_v2(db, "select dirty from resources where guid=?", -1, &sIsResourceDirty, 0);
	//END TODO
	sqlite3_prepare_v2(db, "update notebooks set dirty=? where guid=?", -1, &sSetNotebookDirty, 0);
	sqlite3_prepare_v2(db, "update notes set dirty=? where guid=?", -1, &sSetNoteDirty, 0);
	//TODO
	sqlite3_prepare_v2(db, "update resources set dirty=? where guid=?", -1, &sSetResourceDirty, 0);
	//END TODO
	sqlite3_prepare_v2(db, "update notes set dirty=1 where notebookGuid=?", -1, &sFlagNotesInNotebookDirty, 0);
}

void DatabaseClient::finalizeStatements()
{
	sqlite3_finalize(sGetSyncTime);
	sqlite3_finalize(sGetLastUpdateCount);
	sqlite3_finalize(sSetSyncTime);
	sqlite3_finalize(sSetLastUpdateCount);
	sqlite3_finalize(sGetNotebooks);
	sqlite3_finalize(sGetNotebookByGuid);
	sqlite3_finalize(sGetNotebookByName);
	sqlite3_finalize(sAddNotebook);
	sqlite3_finalize(sUpdateNotebook);
	sqlite3_finalize(sRemoveNotebook);
	sqlite3_finalize(sGetNotesInNotebook);
	sqlite3_finalize(sGetNotesMetadataInNotebook);
	sqlite3_finalize(sDeleteNotesInNotebook);
	sqlite3_finalize(sGetNotes);
	sqlite3_finalize(sGetNoteByGuid);
	sqlite3_finalize(sAddNote);
	sqlite3_finalize(sUpdateNote);
	sqlite3_finalize(sRemoveNote);
	sqlite3_finalize(sDeleteNote);
	sqlite3_finalize(sPurgeDeletedNotes);
	sqlite3_finalize(sGetResourcesInNote);
	sqlite3_finalize(sGetResourcesInNoteNoData);
	sqlite3_finalize(sRemoveResourcesInNote);
	sqlite3_finalize(sGetResourceByGuid);
	sqlite3_finalize(sGetResourceByGuidNoData);
	sqlite3_finalize(sAddResource);
	sqlite3_finalize(sUpdateResource);
	sqlite3_finalize(sRemoveResource);
	sqlite3_finalize(sGetDirtyResourcesInNote);
	sqlite3_finalize(sGetCleanResourcesInNoteNoData);
	sqlite3_finalize(sGetDirtyNotebooks);
	sqlite3_finalize(sGetDirtyNotes);
	sqlite3_finalize(sIsNotebookDirty);
	sqlite3_finalize(sIsNoteDirty);
	sqlite3_finalize(sIsResourceDirty);
	sqlite3_finalize(sSetNotebookDirty);
	sqlite3_finalize(sSetNoteDirty);
	sqlite3_finalize(sSetResourceDirty);
	sqlite3_finalize(sFlagNotesInNotebookDirty);
}

void DatabaseClient::sReset(sqlite3_stmt *stat)
{
	sqlite3_reset(stat);
	sqlite3_clear_bindings(stat);
}

void DatabaseClient::prepareNotebook(Notebook& notebook)
{
	//Set it equal to a new isset object, making all fields false
	notebook.__isset = _Notebook__isset();

	//Set the fields in the database to true. Note: this can be changed if fields are unset in db.
	notebook.__isset.guid = true;
	notebook.__isset.name = true;
	notebook.__isset.updateSequenceNum = true;
	notebook.__isset.defaultNotebook = true;
	notebook.__isset.stack = true;
}

void DatabaseClient::prepareNote(Note& note)
{
	note.__isset = _Note__isset();

	note.__isset.guid = true;
	note.__isset.title = true;
	note.__isset.content = true;
	note.__isset.deleted = true;
	note.__isset.updateSequenceNum = true;
	note.__isset.notebookGuid = true;
}

void DatabaseClient::prepareResource(Resource& res, bool withData)
{
	res.__isset = _Resource__isset();
	res.data.__isset = _Data__isset();
	res.attributes.__isset = _ResourceAttributes__isset();

	//Set fields in Database to true
	res.__isset.guid = true;
	res.__isset.noteGuid = true;
	res.__isset.data = true;
	res.__isset.mime = true;
	res.__isset.attributes = true;
	res.__isset.updateSequenceNum = true;
	res.data.__isset.bodyHash = true;
	res.data.__isset.body = withData;
	res.data.__isset.size = true;
	res.attributes.__isset.fileName = true;
}

void DatabaseClient::fetchNotebooks(vector<Notebook>& ret, sqlite3_stmt *readyStat)
{
	//Empty out vector
	ret.clear();
	
	//Execute
	while (sqlite3_step(readyStat) == SQLITE_ROW)
	{
		Notebook book;
		prepareNotebook(book);
		
		book.guid = (char *)sqlite3_column_text(readyStat, 0);
		book.name = (char *)sqlite3_column_text(readyStat, 1);
		book.updateSequenceNum = sqlite3_column_int(readyStat, 2);

		//If value is negative, let there be no update sequence num
		if (book.updateSequenceNum < 0)
			book.__isset.updateSequenceNum = false;

		book.defaultNotebook = (bool)sqlite3_column_int(readyStat, 3);
		book.stack = (char *)sqlite3_column_text(readyStat, 4);

		//No stack name? No stack.
		if (book.stack == "")
			book.__isset.stack = false;

		ret.push_back(book);
	}
}

void DatabaseClient::fetchNotes(vector<Note>& ret, sqlite3_stmt *readyStat)
{
	//Empty the vector
	ret.clear();

	while (sqlite3_step(readyStat) == SQLITE_ROW)
	{
		Note note;
		prepareNote(note);

		note.guid = (char *)sqlite3_column_text(readyStat, 0);
		note.title = (char *)sqlite3_column_text(readyStat, 1);
		note.content = (char *)sqlite3_column_text(readyStat, 2);
		note.deleted = sqlite3_column_int64(readyStat, 3);

		if (note.deleted < 0)
			note.__isset.deleted = false;

		note.updateSequenceNum = sqlite3_column_int(readyStat, 4);

		if (note.updateSequenceNum < 0)
			note.__isset.updateSequenceNum = false;

		note.notebookGuid = (char *)sqlite3_column_text(readyStat, 5);

		ret.push_back(note);
	}
}

void DatabaseClient::fetchResources(vector<Resource>& ret, sqlite3_stmt *readyStat, bool withData)
{
	//Empty vector
	ret.clear();

	while (sqlite3_step(readyStat) == SQLITE_ROW)
	{
		Resource r;
		prepareResource(r, withData);

		int c = 0;

		r.guid = (char *)sqlite3_column_text(readyStat, c++);
		r.noteGuid = (char *)sqlite3_column_text(readyStat, c++);

		//Get body hash length
		size_t bHashLen = (size_t)sqlite3_column_bytes(readyStat, c);
		r.data.bodyHash = string((char *)sqlite3_column_blob(readyStat, c++), bHashLen);

		//Get size of body
		r.data.size = sqlite3_column_bytes(readyStat, c);

		//Get data if requested
		if (withData)
			r.data.body = string((char *)sqlite3_column_blob(readyStat, c++), r.data.size);
		else
			c++;

		r.mime = (char *)sqlite3_column_text(readyStat, c++);
		r.attributes.fileName = (char *)sqlite3_column_text(readyStat, c++);

		if (r.attributes.fileName == "")
			r.attributes.__isset.fileName = false;

		r.updateSequenceNum = sqlite3_column_int(readyStat, c++);

		if (r.updateSequenceNum < 0)
			r.__isset.updateSequenceNum = false;
		
		ret.push_back(r);
	}
}

void DatabaseClient::bindNotebook(sqlite3_stmt *stat, const Notebook& notebook, bool dirty)
{
	//Bind each field of the notebook to the statement
	sqlite3_bind_text(stat, 1, notebook.guid.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stat, 2, notebook.name.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stat, 3, (notebook.__isset.updateSequenceNum ? notebook.updateSequenceNum : -1));
	sqlite3_bind_int(stat, 4, notebook.defaultNotebook);
	sqlite3_bind_text(stat, 5, (notebook.__isset.stack ? notebook.stack.c_str() : ""), -1, SQLITE_STATIC);
	sqlite3_bind_int(stat, 6, dirty);
}

void DatabaseClient::bindNote(sqlite3_stmt *stat, const Note& note, bool dirty)
{
	//Bind each field of the note to the statement
	sqlite3_bind_text(stat, 1, note.guid.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stat, 2, note.title.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stat, 3, note.content.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int64(stat, 4, (note.__isset.deleted ? note.deleted : -1));
	sqlite3_bind_int(stat, 5, (note.__isset.updateSequenceNum ? note.updateSequenceNum : -1));
	sqlite3_bind_text(stat, 6, note.notebookGuid.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stat, 7, dirty);
}

void DatabaseClient::bindResource(sqlite3_stmt *stat, const Resource& res, bool dirty)
{
	//Bind each field of resource to statement
	sqlite3_bind_text(stat, 1, res.guid.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stat, 2, res.noteGuid.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_blob(stat, 3, res.data.bodyHash.c_str(), res.data.bodyHash.length(), SQLITE_STATIC);
	sqlite3_bind_blob(stat, 4, res.data.body.c_str(), res.data.body.length(), SQLITE_STATIC);
	sqlite3_bind_text(stat, 5, res.mime.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stat, 6, res.attributes.fileName.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_int(stat, 7, (res.__isset.updateSequenceNum ? res.updateSequenceNum : -1));
	sqlite3_bind_int(stat, 8, dirty);
}

/*
//Testing
void printNotebook(const Notebook& notebook)
{
	cout << notebook.guid << "|" << notebook.name << "|" << notebook.updateSequenceNum << "|" << notebook.defaultNotebook << "|" << notebook.stack << endl;
}

//Testing
void printNote(const Note& note)
{
	cout << note.guid << "|" << note.title << "|" << note.content << "|" << note.deleted << "|" << note.updateSequenceNum << "|" << note.notebookGuid << endl;
}

//Testing
int main()
{
	DatabaseClient *db = new DatabaseClient("../database/client.db");
	delete db;
}
*/
