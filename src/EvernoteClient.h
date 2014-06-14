/*
 *Connor Cormier
 *5/26/14
 *Evernote client interface
 */

#ifndef EVERNOTECLIENT_H
#define EVERNOTECLIENT_H

#include "DatabaseClient.h"
#include "evernote_api/src/NoteStore.h"
#include "evernote_api/src/Limits_constants.h"
#include <thrift/transport/THttpClient.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <string>
#include <vector>
#include <sigc++/sigc++.h>


const std::string USER_AGENT = "BetterNote InDev";
const int MAX_ENTRIES = 250;

class EvernoteClient
{
public:
	EvernoteClient(DatabaseClient *db);
	virtual ~EvernoteClient();

	//Sync methods
	void synchronize(), synchronizeAsync();

	//Get current sync state of server, used to determine type of sync to carry out
	void getSyncState(evernote::edam::SyncState& state);
	
	//Get a sync chunk from the server
	void getSyncChunks(std::vector<evernote::edam::SyncChunk>& cBuffer, bool fullSync);
	
	//Resolve server changes with client data. Requires fetching note content and resources for changed objects.
	void resolveServerChanges(const std::vector<evernote::edam::SyncChunk>& cBuffer, bool fullSync), processChunk(const evernote::edam::SyncChunk& chunk, bool fullSync);
	void processResources(const evernote::edam::Note& sNote);
	
	//Send client changes to server and resolve server response (e.g., changed guid's for newly created notes and notebooks). Return true if need to resync, false otherwise.
	bool sendClientChanges();
	bool getNoteResources(const evernote::edam::Note& note);
	
	//Get all notebooks associated with current account
	void getNotebooks(std::vector<evernote::edam::Notebook>& notes);

	//Get the names of all notes in notebook with given Guid, no Guid for all notes
	//Returned notes do not contain content!
	void getNotesInNotebook(evernote::edam::NotesMetadataList& ans, const evernote::edam::Guid& notebook);

	//Get a note
	void getNote(evernote::edam::Note& note, const evernote::edam::Guid& nGuid);

	//Update a note in the database
	void updateNote(const evernote::edam::Note& note);

	//Signal accessor. Name uses gtkmm conventions for convenience.
	sigc::signal<void>& signal_on_sync_complete();

private:
	//The database
	DatabaseClient *db;
	
	//Sync state info
	evernote::edam::Timestamp lastSyncTime;
	int lastUpdateCount;
	
	std::string devToken, devHost, devPath;
	boost::shared_ptr<evernote::edam::NoteStoreClient> nStore;
	evernote::limits::LimitsConstants lims;

	//SSL
	boost::shared_ptr<apache::thrift::transport::TSSLSocket> noteStoreSocket;
	boost::shared_ptr<apache::thrift::transport::TTransport> nStoreHttpClient;

	//Signals for client functions
	sigc::signal<void> signalOnSyncComplete;

	//Methods
	void setupClient();
	boost::shared_ptr<apache::thrift::transport::TSSLSocketFactory> getSslSocketFactory();
	std::vector<evernote::edam::Notebook> getNotebooks();
	void loadDbInfo();
	void updateSyncInfo(const evernote::edam::SyncState& nState);
	
	//Testing
	void authenticateDev();
	
};

#endif
