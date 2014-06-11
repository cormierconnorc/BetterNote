/*
 *Connor Cormier
 *5/28/14
 *Note displayer
 */

#include "NoteSelector.h"
#include "NoteListStore.h"

using namespace std;
using namespace evernote::edam;

NoteSelector::NoteSelector(BaseObjectType *cObj, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::ScrolledWindow(cObj)
{
	//Add treeview
	add(view);
	
	//Auto show and hide scroll bars
	set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	//Using a list store this time, no "child elements"
	treeModel = NoteListStore::create(colModel);
	view.set_model(treeModel);
	
	//Trigger with single click
	view.set_activate_on_single_click(true);

	view.enable_model_drag_source();

	//Drag source. For strings. Data get functions defined in NoteListStore
	vector<Gtk::TargetEntry> dragTargets;
	dragTargets.push_back(Gtk::TargetEntry("STRING", Gtk::TARGET_SAME_APP|Gtk::TARGET_OTHER_WIDGET));
	view.enable_model_drag_source(dragTargets);

	//Listen for clicks
	view.signal_button_press_event().connect(sigc::mem_fun(*this, &NoteSelector::onButtonPressEvent), false);
	
	//Connect row selection
	view.signal_row_activated().connect(sigc::mem_fun(*this, &NoteSelector::onRowSelected));

	//Connect key press listener
	view.signal_key_press_event().connect(sigc::mem_fun(*this, &NoteSelector::onKeyPress), false);

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

void NoteSelector::addNote(const Note& note)
{
	Gtk::TreeModel::Row row = *(treeModel->append());
	row[colModel.nameCol] = note.title;
	row[colModel.guid] = note.guid;	
}


sigc::signal<void>& NoteSelector::signal_note_selected()
{
	return note_select_sig;
}

sigc::signal<void, evernote::edam::Note&>& NoteSelector::signal_note_deleted()
{
	return note_delete_sig;
}

sigc::signal<evernote::edam::Note>& NoteSelector::signal_note_created()
{
	return note_create_sig;
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

bool NoteSelector::onButtonPressEvent(GdkEventButton *event)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
	{
		if (view.get_path_at_pos((int)event->x, (int)event->y, clickPath))
			showSelectedContext(event);
		else
			showUnselectedContext(event);
		
		return true;
	}

	return false;
}

void NoteSelector::showSelectedContext(GdkEventButton *event)
{
	Gtk::Menu *m = Gtk::manage(new Gtk::Menu());

	Gtk::MenuItem *i = Gtk::manage(new Gtk::MenuItem("_Delete Note", true));
	i->signal_activate().connect(sigc::mem_fun(*this, &NoteSelector::onDeleteNote));
	m->append(*i);

	m->accelerate(*this);
	m->show_all();

	m->popup(event->button, event->time);
}

void NoteSelector::showUnselectedContext(GdkEventButton *event)
{
	Gtk::Menu *m = Gtk::manage(new Gtk::Menu());

	Gtk::MenuItem *i = Gtk::manage(new Gtk::MenuItem("_Create New Note", true));
	i->signal_activate().connect(sigc::mem_fun(*this, &NoteSelector::onCreateNote));
	m->append(*i);

	m->accelerate(*this);
	m->show_all();

	m->popup(event->button, event->time);
}

void NoteSelector::onDeleteNote()
{
	Note toDelete;
	toDelete.__isset.guid = true;

	//Get the guid of the note to delete
	Gtk::TreeModel::iterator martyr = treeModel->get_iter(clickPath);

	toDelete.guid = martyr->get_value(colModel.guid);

	note_delete_sig.emit(toDelete);

	//Now remove row from view
	treeModel->erase(martyr);
}

void NoteSelector::onCreateNote()
{
	//Get the note from signal emit
	try
	{
		Note toShow = note_create_sig.emit();
		addNote(toShow);
	}
	catch (std::exception& ex)
	{
		cerr << ex.what() << endl;

		//Do something
	}
}

bool NoteSelector::onKeyPress(GdkEventKey *event)
{
	//Selection dependent options: Note creation (possibly renaming as well in future version)
	if (event->keyval == GDK_KEY_Delete)
	{
		vector<Gtk::TreeModel::Path> selected = view.get_selection()->get_selected_rows();

		if (selected.size() == 1)
		{
			clickPath = selected[0];

			onDeleteNote();
		}

		return true;
	}
	//Non-selection dependent options. Requires control mask
	else if (event->state & GDK_CONTROL_MASK)
	{
		if (event->keyval == GDK_KEY_n)
		{
			onCreateNote();
			return true;
		}
	}

	return false;
}
