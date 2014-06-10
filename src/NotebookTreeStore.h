/*
 *Connor Cormier
 *6/09/14
 *Custom tree store to control drag and drop functionality for NotebookSelector
 */

#ifndef NOTEBOOKTREESTORE_H
#define NOTEBOOKTREESTORE_H

#include <gtkmm.h>
#include "evernote_api/src/NoteStore_types.h"

const evernote::edam::Guid STACK_GUID = "stack";
const Glib::ustring DEFAULT_STACK = "New Stack";

class NotebookTreeStore : public Gtk::TreeStore
{
public:
	static Glib::RefPtr<NotebookTreeStore> create(const Gtk::TreeModel::ColumnRecord& columns);

	sigc::signal<void, Gtk::TreeModel::Path&>& signal_create_stack();
	sigc::signal<void, Gtk::TreeModel::Path&>& signal_change_stack();
	
protected:
	NotebookTreeStore(const Gtk::TreeModel::ColumnRecord& columns);

	//Draggable test
	virtual bool row_draggable_vfunc(const Gtk::TreeModel::Path& path) const;

	//Drag method
	virtual bool drag_data_get_vfunc(const TreeModel::Path& src, Gtk::SelectionData& selectionData) const;
	
	//Drop method
	virtual bool drag_data_received_vfunc(const Gtk::TreeModel::Path& dest, const Gtk::SelectionData& selectionData);

private:
	int getPathDepth(Gtk::TreeModel::Path path);
	bool srcBefore(const Gtk::TreeModel::Path& path);

	//Path of drag source, stored for posterity (srcBefore method, that is). Must be mutable to be altered in the drag_data_get_vfunc
	mutable Gtk::TreeModel::Path srcPath;

	sigc::signal<void, Gtk::TreeModel::Path&> stackCreateSignal, stackChangeSignal;
};

#endif
