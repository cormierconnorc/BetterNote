/*
 *Connor Cormier
 *5/28/14
 *Note displayer
 */

#include "NoteSelector.h"

using namespace std;
using namespace evernote::edam;

NoteSelector::NoteSelector(BaseObjectType *cObj, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::ScrolledWindow(cObj)
{
	//Add treeview
	add(view);
	
	//Auto show and hide scroll bars
	set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	//Using a list store this time, no "child elements"
	treeModel = Gtk::ListStore::create(colModel);
	view.set_model(treeModel);
	
	//Trigger with single click
	view.set_activate_on_single_click(true);

	//Connect row selection
	view.signal_row_activated().connect(sigc::mem_fun(*this, &NoteSelector::onRowSelected));

	//Show note column
	view.append_column("Note", colModel.nameCol);

	show_all_children();
}

NoteSelector::~NoteSelector()
{
}

void NoteSelector::setNoteList(const vector<NoteMetadata>& noteList)
{
	//Clear out existing notes
	treeModel->clear();

	//Leave old note active until further notice
	//activeNote = "";

	for (vector<NoteMetadata>::const_iterator it = noteList.begin(); it != noteList.end(); it++)
		addNote(*it);

	//Select the previously active note if possible
	if (activeNote != "")
	{
		for (Gtk::TreeModel::iterator it = treeModel->children().begin(); it != treeModel->children().end(); it++)
			if (activeNote == it->get_value(colModel.guid))
				view.get_selection()->select(*it);
	}
}

void NoteSelector::addNote(const NoteMetadata& note)
{
	Gtk::TreeModel::Row row = *(treeModel->append());
	row[colModel.nameCol] = note.title;
	row[colModel.guid] = note.guid;	
}


sigc::signal<void>& NoteSelector::signal_note_selected()
{
	return note_select_sig;
}

const Guid& NoteSelector::getActiveNote()
{
	return activeNote;
}

void NoteSelector::onRowSelected(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn *column)
{
	Gtk::TreeModel::iterator it = treeModel->get_iter(path);

	if (it)
	{
		Gtk::TreeModel::Row row = *it;

		activeNote = row[colModel.guid];

		//Emit signal to record update
		note_select_sig.emit();
	}
}


