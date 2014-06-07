/*
 *Connor Cormier
 *5/27/14
 *View for displaying notebook hierarchy
 */

#ifndef NOTEBOOKSELECTOR_H
#define NOTEBOOKSELECTOR_H

#include <gtkmm.h>
#include <vector>
#include "evernote_api/src/NoteStore_types.h"

class NotebookSelector : public Gtk::ScrolledWindow
{
public:
	NotebookSelector(BaseObjectType *cObj, const Glib::RefPtr<Gtk::Builder>& builder);
	virtual ~NotebookSelector();
	
	//Set notebook list
	void setNotebookList(const std::vector<evernote::edam::Notebook>& notebooks);
	void addNotebook(const evernote::edam::Notebook notebook);

	//Alias for signal_notebook_change, used to connect
	sigc::signal<void>& signal_notebook_change();

	//Way to update post-callback trigger
	const std::vector<evernote::edam::Guid>& getActiveNotebooks();

private:

	//The rows containing notebook stacks
	std::vector<Gtk::TreeModel::Row> notebookStackRows;

	//Currently active notebook Guid's. Updated each time a row is selected
	std::vector<evernote::edam::Guid> activeNotebooks;

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
	Glib::RefPtr<Gtk::TreeStore> treeModel;

	//Signal parent
	sigc::signal<void> note_change_sig;

	//Method to emit change signal upon row selection
	void onRowSelected(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn *column);
};

#endif
