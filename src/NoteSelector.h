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
	
	//Alias for selection signal
	//Note: different naming convention is for consistency with gtkmm code
	sigc::signal<void>& signal_note_selected();

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

	//Method to emit signal upon note selection
	void onRowSelected(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn *column);
};

#endif
