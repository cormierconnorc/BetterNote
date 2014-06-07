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
	//Setup client
	client = new EvernoteClient;

	//Have the client notify the note window upon sync complete
	client->signal_on_sync_complete().connect(sigc::mem_fun(this, &NoteWindow::refresh));
	
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

	//Children of the NoteWindow are managed thanks to the builder, so no need to
	//explicitly delete them.
}

void NoteWindow::loadObjs(const Glib::RefPtr<Gtk::Builder>& builder)
{
	//Get the notebook and note list view
	builder->get_widget_derived("NotebookSelector", notebookSel);
	builder->get_widget_derived("NoteSelector", noteSel);

	//Set callbacks for each
	notebookSel->signal_notebook_change().connect(sigc::mem_fun(*this, &NoteWindow::onNotebookSelected));
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
	client->getNotebooks(notebooks);

	//Now set the notebook selector's notebook list to this list
	notebookSel->setNotebookList(notebooks);
}

void NoteWindow::showCurNotes()
{
	const vector<Guid> activeNotes = notebookSel->getActiveNotebooks();
	vector<NoteMetadata> relData;

	for (size_t i = 0; i < activeNotes.size(); i++)
	{
		NotesMetadataList list;

		client->getNotesInNotebook(list, activeNotes[i]);
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
	client->getNote(note, noteSel->getActiveNote());

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
	client->updateNote(note->getNote());

	/*Only relevant for direct update
	//Get the note associated with the metadata in newNote
	client->getNote(newNote, newNote.guid);
	
	note->showNote(newNote);
	*/
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
