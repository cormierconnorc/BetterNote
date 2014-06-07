/*
 *Connor Cormier
 *5/26/14
 *Evernote client, handles all connections with Evernote SDK
 */

#include "EvernoteClient.h"
#include <iostream>

//Typedefs to clean up these long namespaces. Not used in header for clarity, but I'll be damned if I'm not going to shorten things up here.

using namespace std;
using namespace evernote::edam;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

EvernoteClient::EvernoteClient() : lastUpdateCount(0)
{
	db = new DatabaseClient(DATABASE_FILE);
	
	//Temporary
	authenticateDev();
	setupClient();

	//Temporary: synchronize upon creation
	synchronize();
}

EvernoteClient::~EvernoteClient()
{
	delete db;
}

void EvernoteClient::synchronize()
{
	cout << "Synchronizing!" << endl;
	
	loadDbInfo();
	
	SyncState ss;
	getSyncState(ss);
	
	if (ss.fullSyncBefore > lastSyncTime || ss.updateCount != lastUpdateCount)
	{
		bool fullSync = ss.fullSyncBefore > lastSyncTime;

		cout << "Doing " << (fullSync ? "full " : "incremental ") << "sync" << endl;

		vector<SyncChunk> chunkBuffer;

		getSyncChunks(chunkBuffer, fullSync);
		resolveServerChanges(chunkBuffer, fullSync);
	}

	//Now save sync state info
	updateSyncInfo(ss);
	
	bool resync = sendClientChanges();

	//Recursively try again to catch up with server
	if (resync)
		synchronize();
	//Otherwise, emit the complete signal
	else
		signalOnSyncComplete.emit();
}

void EvernoteClient::synchronizeAsync()
{
	//TODO
}

void EvernoteClient::getSyncState(SyncState& state)
{
	nStore->getSyncState(state, devToken);
}

void EvernoteClient::getSyncChunks(vector<SyncChunk>& cBuffer, bool fullSync)
{
	SyncChunkFilter filter;

	//Filters. These are currently all the client needs
	filter.__isset.includeNotes = true;
	//filter.__isset.includeNoteResources = true;	//Do not include in this version
	filter.__isset.includeNotebooks = true;
	filter.__isset.includeExpunged = !fullSync;		//Only need expunged on incremental sync

	filter.includeNotes = true;
	//filter.includeNoteResources = true;
	filter.includeNotebooks = true;
	filter.includeExpunged = !fullSync;

	int afterUSN = (fullSync ? 0 : lastUpdateCount);

	SyncChunk chunky;

	//Get first update chunk
	nStore->getFilteredSyncChunk(chunky, devToken, afterUSN, MAX_ENTRIES, filter);

	while (chunky.chunkHighUSN < chunky.updateCount)
	{
		//Buffer current chunk
		cBuffer.push_back(chunky);

		//update afterUSN
		afterUSN = chunky.chunkHighUSN;

		//Get next chunk
		nStore->getFilteredSyncChunk(chunky, devToken, afterUSN, MAX_ENTRIES, filter);
	}

	//Buffer final chunk
	cBuffer.push_back(chunky);
}

void EvernoteClient::resolveServerChanges(const vector<SyncChunk>& cBuffer, bool fullSync)
{
	for (vector<SyncChunk>::size_type t = 0; t < cBuffer.size(); t++)
		processChunk(cBuffer[t], fullSync);
}

void EvernoteClient::processChunk(const SyncChunk& chunk, bool fullSync)
{
	//Process notebooks
	for (vector<Notebook>::const_iterator it = chunk.notebooks.begin(); it != chunk.notebooks.end(); it++)
	{
		Notebook sBook = (*it);
		Notebook lBook;

		//Notebook exists on both client and server
		if (db->getNotebookByGuid(lBook, sBook.guid))
		{
			if (sBook.updateSequenceNum > lBook.updateSequenceNum)
			{
				//Whether dirty or not, just keep the server's version for the notebook. The worst case scenario is a change away from the client's new name, which won't break anything. Better than the unwieldy conflict resolution alternatives.
				db->updateNotebook(sBook);

				//Newly updated notebook cannot be dirty
				db->unflagDirty(lBook);
			}
		}
		//Notebook name conflict
		else if (db->getNotebookByName(lBook, sBook.name))
		{
			//Put the server notebook in first
			db->addNotebook(sBook);
			
			bool dirty = db->isNotebookDirty(lBook);

			//Notebook created here and online without syncing. Merge all notes belonging to this notebook into the server notebook and flag them dirty
			if (dirty)
			{
				vector<Note> mergeNotes;
				db->getNotesInNotebook(mergeNotes, lBook);

				for (vector<Note>::size_type t = 0; t < mergeNotes.size(); t++)
				{
					//Point note toward server notebook
					mergeNotes[t].notebookGuid = sBook.guid;

					//Tell the service to sync this note when the time comes
					db->flagDirty(mergeNotes[t]);

					//Update the note's guid in the database
					db->updateNote(mergeNotes[t]);
				}

				//Remove the old notebook
				db->removeNotebook(lBook);
			}
			//Old notebook needs to be renamed locally to avoid conflicts
			else
			{
				//Tell the db client to rename the notebook to avoid conflicts
				db->flagDirty(lBook);
				db->renameNotebook(lBook);
			}
		}
		//New notebook
		else
		{
			//New notebook, add to database
			db->addNotebook(sBook);
		}
	}
	
	//Process notes
	for (vector<Note>::const_iterator it = chunk.notes.begin(); it != chunk.notes.end(); it++)
	{
		//Server note in this chunk
		Note sNote = (*it);
		//Client note in db
		Note lNote;

		//Same note found on server and client
		if (db->getNoteByGuid(lNote, sNote.guid))
		{
			bool dirty = db->isNoteDirty(lNote);

			//Server is updated version of client. Just update the note, no conflict resolution
			if (sNote.updateSequenceNum > lNote.updateSequenceNum)
			{
				if (!dirty)
				{
					//Get full note info
					this->getNoteFromService(sNote, sNote.guid);

					//Update it in the database
					db->updateNote(sNote);
				}
				else
				{
					//Conflict found, take action

					//Move older copy into "Conflicting changes" notebook, treating it as a new note with a generated (later to be updated) guid.
					//TODO
				}
			}
		}
		//New note from server, no conflict found
		else
		{
			//Get full note info
			this->getNoteFromService(sNote, sNote.guid);

			//Add to the database
			db->addNote(sNote);
		}
	}


	//After processing, leave and send changes for full sync or remove expunged for incremental
	if (fullSync)
		return;

	//Clear away the expunged
	for (vector<Guid>::const_iterator it = chunk.expungedNotebooks.begin(); it != chunk.expungedNotebooks.end(); it++)
	{
		Notebook remove;
		if (db->getNotebookByGuid(remove, *it))
			db->removeNotebook(remove);
	}

	for (vector<Guid>::const_iterator it = chunk.expungedNotes.begin(); it != chunk.expungedNotes.end(); it++)
	{
		Note remove;
		if (db->getNoteByGuid(remove, *it))
			db->removeNote(remove);
	}

}

bool EvernoteClient::sendClientChanges()
{
	bool needResync = false;
	
	//Process new notes
	vector<Note> dirtyNotes;
	db->getDirtyNotes(dirtyNotes);

	for (size_t t = 0; t < dirtyNotes.size(); t++)
	{
		Note dNote = dirtyNotes[t];

		if (dNote.__isset.updateSequenceNum)
		{
			try
			{
				nStore->updateNote(dNote, devToken, dNote);

				//Update note in database
				db->updateNote(dNote);
				
				//Remove dirty flag
				db->unflagDirty(dNote);

				//Resync if required
				if (dNote.updateSequenceNum > lastUpdateCount + 1)
					needResync = true;
			}
			//Handle conflicts by requiring resync (note conflicts are handled in "processChunk", so this is easier than handling it all again here)
			catch (std::exception& ex)
			{
				cerr << ex.what() << endl;
				needResync = true;
			}
		}
		//New note. Create on server
		else
		{
			try
			{
				Note newNote;
				nStore->createNote(newNote, devToken, dNote);

				//Just remove the local note and add in the new one.
				db->removeNote(dNote);
				db->addNote(newNote);

				//More changes have occurred and another resync is required
				if (newNote.updateSequenceNum > lastUpdateCount + 1)
					needResync = true;
			}
			//Same system. Yes, it's lazy, but it should work.
			catch (std::exception& ex)
			{
				cerr << ex.what() << endl;
				needResync = true;
			}
		}
	}


	//Now onto the same algorithm for the notebooks
	vector<Notebook> dirtyNotebooks;
	db->getDirtyNotebooks(dirtyNotebooks);

	for (size_t t = 0; t < dirtyNotebooks.size(); t++)
	{
		Notebook dBook = dirtyNotebooks[t];

		if (dBook.__isset.updateSequenceNum)
		{
			try
			{
				nStore->updateNotebook(devToken, dBook);

				//Update local notebook
				db->updateNotebook(dBook);

				//Remove dirty flag
				db->unflagDirty(dBook);

				if (dBook.updateSequenceNum > lastUpdateCount + 1)
					needResync = true;
			}
			catch (std::exception& ex)
			{
				cerr << ex.what() << endl;
				needResync = true;
			}
		}
		else
		{
			try
			{
				Notebook newBook;
				nStore->createNotebook(newBook, devToken, dBook);

				//Delete old
				db->removeNotebook(dBook);

				//Add new
				db->addNotebook(newBook);

				if (newBook.updateSequenceNum > lastUpdateCount + 1)
					needResync = true;
			}
			catch (std::exception& ex)
			{
				cerr << ex.what() << endl;
				needResync = true;
			}
		}
	}


	return needResync;
}

void EvernoteClient::getNotebooks(vector<Notebook>& notes)
{
	db->getNotebooks(notes);
}

void EvernoteClient::getNotebooksFromService(vector<Notebook>& notes)
{
	try
	{
		nStore->listNotebooks(notes, devToken);
	}
	catch (std::exception& ex)
	{
		cout << ex.what() << endl;
	}
}

void EvernoteClient::getNotesInNotebook(NotesMetadataList& ans, const Guid& notebook)
{
	//Required to maintain database model
	Notebook note;
	note.guid = notebook;

	db->getNotesMetadataInNotebook(ans, note);
}

void EvernoteClient::getNotesInNotebookFromService(NotesMetadataList& ans, const Guid& notebook)
{
	//Define filter
	NoteFilter filter;
	filter.__isset.notebookGuid = true;
	filter.notebookGuid = notebook;

	//Define result spect
	NotesMetadataResultSpec spec;
	spec.__isset.includeTitle = true;
	spec.includeTitle = true;

	nStore->findNotesMetadata(ans, devToken, filter, 0, lims.EDAM_USER_NOTES_MAX, spec);
}

void EvernoteClient::getNote(Note& note, const Guid& nGuid)
{
	db->getNoteByGuid(note, nGuid);
}

void EvernoteClient::getNoteFromService(Note& note, const Guid& nGuid)
{
	nStore->getNote(note, devToken, nGuid, true, false, false, false);
}

void EvernoteClient::updateNote(const Note& note)
{
	db->updateNote(note);
	db->flagDirty(note);
}

sigc::signal<void>& EvernoteClient::signal_on_sync_complete()
{
	return signalOnSyncComplete;
}

void EvernoteClient::authenticateDev()
{
	devHost = "sandbox.evernote.com";
	devPath = "/shard/s1/notestore";
	devToken = "S=s1:U=8ea2c:E=14d910ac93d:C=14639599d40:P=1cd:A=en-devtoken:V=2:H=a43a17f849c04fba6ed8bad51ae2648a";
}

void EvernoteClient::setupClient()
{
	//Credit to "Rambus": https://discussion.evernote.com/topic/27583-evernote-c-example-code/
	//And baumgarr, from the Nixnote 2 source code.
	//SSL setup: DISABLED
	/*
	boost::shared_ptr<TSSLSocketFactory> factory = getSslSocketFactory();
	noteStoreSocket = boost::shared_ptr<TSSLSocket>(factory->createSocket(devHost, 443));
	boost::shared_ptr<TBufferedTransport> transport(new TBufferedTransport(noteStoreSocket));
	*/
	
	//Setup http client
	nStoreHttpClient = boost::shared_ptr<TTransport>(new THttpClient(devHost, 80, devPath));/*new THttpClient(transport, devHost, devPath));*/
	nStoreHttpClient->open();

	//Set protocol
	boost::shared_ptr<TProtocol> nStoreProt(new TBinaryProtocol(nStoreHttpClient));

	//Set store
	nStore = boost::shared_ptr<NoteStoreClient>(new NoteStoreClient(nStoreProt));
}


boost::shared_ptr<TSSLSocketFactory> EvernoteClient::getSslSocketFactory()
{
	boost::shared_ptr<TSSLSocketFactory> factory(new TSSLSocketFactory());

	//Load client trusted certificates
	factory->loadTrustedCertificates("certs/verisign_certs.pem");
	factory->loadTrustedCertificates("certs/thawte_certs.pem");

	//Unneeded
	//factory->loadCertificate("certs/my_cert.pem");
	//factory->loadPrivateKey("certs/my_key.pem");

	factory->authenticate(true);
	
	return factory;
}

void EvernoteClient::loadDbInfo()
{
	lastSyncTime = db->getSyncTime();
	lastUpdateCount = db->getLastUpdateCount();
}

void EvernoteClient::updateSyncInfo(const SyncState& nState)
{
	//Local updates
	lastSyncTime = nState.currentTime;
	lastUpdateCount = nState.updateCount;

	//Update info in database
	db->setSyncTime(lastSyncTime);
	db->setLastUpdateCount(lastUpdateCount);
}
