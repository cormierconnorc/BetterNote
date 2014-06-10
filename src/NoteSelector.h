/*
 *Connor Cormier
 *5/27/14
 *View for displaying note hierarchy
 */

#ifndef NOTESELECTOR_H
#define NOTESELECTOR_H

#include <gtkmm.h>
#include "evernote_api/src/NoteStore_types.h"

class NoteSelector : public Gtk::ScrolledWindow
{
public:
	NoteSelector(BaseObjectType *cObj, const Glib::RefPtr<Gtk::Builder>& builder);
	virtual ~NoteSelector();

	//Refresh all notes or just add a single one
	void setNoteList(const std::vector<evernote::edam::NoteMetadata>& noteList);
	void addNote(const evernote::edam::NoteMetadata& note);
	void addNote(const evernote::edam::Note& note);
	
	//Aliases for signals.
	//Note: different naming convention is for consistency with gtkmm code
	sigc::signal<void>& signal_note_selected();
	sigc::signal<void, evernote::edam::Note&>& signal_note_deleted();
	sigc::signal<evernote::edam::Note>& signal_note_created();

	//Show active note
	const evernote::edam::Guid& getActiveNote();

private:
	//No need to explicitly store note metadata, all handled in rows.
	evernote::edam::Guid activeNote;
	
	
	//Column model
	class ModelColumns : public Gtk::TreeModel::ColumnRecord
	{
  	public:
		ModelColumns()
		{
			add(nameCol);
			add(guid);
		}

		Gtk::TreeModelColumn<Glib::ustring> nameCol;
		Gtk::TreeModelColumn<evernote::edam::Guid> guid;
	};

	ModelColumns colModel;

	Gtk::TreeView view;
	Glib::RefPtr<Gtk::ListStore> treeModel;

	//Signal parent of note selection
	sigc::signal<void> note_select_sig;

	//Context menu signals to parent
	sigc::signal<void, evernote::edam::Note&> note_delete_sig;
	sigc::signal<evernote::edam::Note> note_create_sig;

	//Path of last right click
	Gtk::TreeModel::Path clickPath;

	//Method to emit signal upon note selection
	void onRowSelected(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn *column);

	//Button press handler
	bool onButtonPressEvent(GdkEventButton *event);

	//Show appropriate context after button press
	void showSelectedContext(GdkEventButton *event);
	void showUnselectedContext(GdkEventButton *event);

	//Responses to context menu items
	void onDeleteNote();
	void onCreateNote();
};

#endif
