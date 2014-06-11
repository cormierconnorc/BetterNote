/*
 *Connor Cormier
 *6/11/14
 *Custom list store to control drag and drop functionality for NoteSelector
 */

#include "NoteListStore.h"

using namespace std;
using namespace evernote::edam;

#define NAME_COL 0
#define GUID_COL 1

NoteListStore::NoteListStore(const Gtk::TreeModel::ColumnRecord& columns) : Gtk::ListStore(columns)
{
	
}

Glib::RefPtr<NoteListStore> NoteListStore::create(const Gtk::TreeModel::ColumnRecord& columns)
{
	return Glib::RefPtr<NoteListStore>(new NoteListStore(columns));
}

bool NoteListStore::drag_data_get_vfunc(const Gtk::TreeModel::Path& src, Gtk::SelectionData& data) const
{
	Gtk::TreeModel::iterator iter = ((NoteListStore*)this)->get_iter(src);

	if (iter)
	{
		Guid guid;
		iter->get_value(GUID_COL, guid);
		data.set_text(guid);

		return true;
	}

	return false;
}
