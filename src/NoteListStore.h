/*
 *Connor Cormier
 *6/11/14
 *Custom list store to control drag and drop functionality for NoteSelector
 */

#ifndef NOTELISTSTORE_H
#define NOTELISTSTORE_H

#include <gtkmm.h>
#include "evernote_api/src/NoteStore_types.h"

class NoteListStore : public Gtk::ListStore
{
public:
	static Glib::RefPtr<NoteListStore> create(const Gtk::TreeModel::ColumnRecord& columns);
	
protected:
	NoteListStore(const Gtk::TreeModel::ColumnRecord& columns);

	//Method to get string note guid from row in order to change notebook
	virtual bool drag_data_get_vfunc(const Gtk::TreeModel::Path& src, Gtk::SelectionData& data) const;
};

#endif
