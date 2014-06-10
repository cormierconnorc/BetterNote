/*
 *Connor Cormier
 *5/25/14
 *NoteWindow implementation
 */

#include "NoteWindow.h"
#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;
using namespace evernote::edam;

NoteWindow::NoteWindow(BaseObjectType* cObj, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Window(cObj)
{
	db = new DatabaseClient(DATABASE_FILE);
	
	//Setup client
	client = new EvernoteClient(db);

	//Have the client notify the note window upon sync complete
	client->signal_on_sync_complete().connect(sigc::mem_fun(*this, &NoteWindow::refresh));
	
	loadObjs(builder);

	//Temporary while service down
	refresh();

	//Listen for escape key press
	this->add_events(Gdk::KEY_PRESS_MASK);
	this->signal_key_press_event().connect(sigc::mem_fun(*this, &NoteWindow::onKeyPress));
}

NoteWindow::~NoteWindow()
{	
	delete client;

	delete db;
	//Children of the NoteWindow are managed thanks to the builder, so no need to
	//explicitly delete them.
}

void NoteWindow::loadObjs(const Glib::RefPtr<Gtk::Builder>& builder)
{
	//Get the notebook and note list view
	builder->get_widget_derived("NotebookSelector", notebookSel);
	builder->get_widget_derived("NoteSelector", noteSel);

	//Set callbacks for notebook selector 
	notebookSel->signal_notebook_change().connect(sigc::mem_fun(*this, &NoteWindow::onNotebookSelected));
	notebookSel->signal_notebook_rename().connect(sigc::mem_fun(*this, &NoteWindow::onNotebookRename));
	notebookSel->signal_notebook_delete().connect(sigc::mem_fun(*this, &NoteWindow::onNotebookDelete));
	notebookSel->signal_notebook_create().connect(sigc::mem_fun(*this, &NoteWindow::createNotebook));

	//Now for note selector
	noteSel->signal_note_selected().connect(sigc::mem_fun(*this, &NoteWindow::onNoteSelected));
	
	//Get text view
	builder->get_widget_derived("NoteBox", note);	
}

void NoteWindow::refresh()
{
	showCurNotebooks();
	showCurNotes();
	showCurNote();
}

void NoteWindow::showCurNotebooks()
{
	//Update notebooks using evernote client
	vector<Notebook> notebooks;
	db->getNotebooks(notebooks);

	//Now set the notebook selector's notebook list to this list
	notebookSel->setNotebookList(notebooks);
}

void NoteWindow::showCurNotes()
{
	const vector<Guid> activeNotes = notebookSel->getActiveNotebooks();
	vector<NoteMetadata> relData;

	Notebook n;
	
	for (size_t i = 0; i < activeNotes.size(); i++)
	{
		NotesMetadataList list;

		n.guid = activeNotes[i];

		db->getNotesMetadataInNotebook(list, n);
		relData.insert(relData.end(), list.notes.begin(), list.notes.end());
	}

	//Set the note selector to display note metadata
	noteSel->setNoteList(relData);
}

void NoteWindow::showCurNote()
{
	if (noteSel->getActiveNote() == "")
	{
		note->showHtml("No note selected</br>This buffer can be used for text you don't wish to save.");
		return;
	}

	//Get the note
	Note note;
	db->getNoteByGuid(note, noteSel->getActiveNote());

	//Give it to the buffer
	this->note->showNote(note);
}

/*
 *Update note list each time a new notebook is selected
 */
void NoteWindow::onNotebookSelected()
{
	//Update currently visible notes
	showCurNotes();
}

/*
 *Update note view each time a new note is selected
 */
void NoteWindow::onNoteSelected()
{
	//Show the current note
	showCurNote();
}

void NoteWindow::updateVisibleNote()
{
	const Note& n = note->getNote();
	
	db->updateNote(n);
	db->flagDirty(n);

	/*Only relevant for direct update
	//Get the note associated with the metadata in newNote
	client->getNote(newNote, newNote.guid);
	
	note->showNote(newNote);
	*/
}

void NoteWindow::onNotebookRename(const vector<Notebook>& changed)
{
	for (vector<Notebook>::const_iterator it = changed.begin(); it != changed.end(); it++)
	{
		Notebook n = *it;

		Notebook ref;

		//Only continue with renaming if notebook was found in database
		if (db->getNotebookByGuid(ref, n.guid))
		{
			//Consolidate reference notebook (from db) with new material in "n"
			if (n.__isset.name)
			{
				ref.__isset.name = true;
				ref.name = n.name;
			}
			
			if (n.__isset.stack)
			{
				ref.__isset.stack = true;
				ref.stack = n.stack;
			}

			//Now update in the database using the reference notebook
			db->updateNotebook(ref);

			//Rename to avoid conflicts (does nothing if no conflict)
			db->renameNotebook(ref);
			
			db->flagDirty(ref);
		}
	}

	//Refresh notebook list
	showCurNotebooks();
}

void NoteWindow::onNotebookDelete(const vector<Notebook>& deleted)
{
	for (size_t t = 0; t < deleted.size(); t++)
	{
		//Remove notebook from database.
		//Note: Just removes, does not actually delete on server (which I can't do with the permissions afforded to this application)
		db->removeNotebook(deleted[t]);

		//Now flag each note as deleted
		db->deleteNotesInNotebook(deleted[t]);

		//Make sure each is flaged as dirty
		db->flagNotesInNotebookDirty(deleted[t]);
	}

	//Refresh entire list this time, unlike the simple rename.
	refresh();
}

Notebook NoteWindow::createNotebook(const string& stackName)
{
	Notebook newNote;

	newNote.__isset.guid = true;
	newNote.__isset.name = true;
	newNote.__isset.stack = (stackName != "");
	newNote.stack = stackName;

	Notebook ref;

	//Continually try to generate the GUID while it is not unique (VERY unlikely to have conflicts, but worth checking since my generation scheme might not be random enough)
	do
	{
		newNote.guid = Util::genGuid();
	} while (db->getNotebookByGuid(ref, newNote.guid));

	db->addNotebook(newNote);
	db->flagDirty(newNote);

	return newNote;
}

bool NoteWindow::on_key_press_event(GdkEventKey *event)
{
	//Force tab behavior by executing the appropriate command in the note window
	//But only if the note is in focus
	if (event->keyval == GDK_KEY_Tab && note->isNoteFocus())
	{
		note->tabAction();
		return true;
	}
	//Another override for the F9 key
	else if (event->keyval == GDK_KEY_F9)
	{
		client->synchronize();
		return true;
	}

	return Gtk::Window::on_key_press_event(event);
}

bool NoteWindow::onKeyPress(GdkEventKey *event)
{	
	//Actions to take if control modifier has been pressed
	if (event->state & GDK_CONTROL_MASK)
	{
		//Ctrl-S: Take save action
		switch (event->keyval)
		{
			case GDK_KEY_s:
				updateVisibleNote();
				return true;
		}
	}
	
	return false;
}
