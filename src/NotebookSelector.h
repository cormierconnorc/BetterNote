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
#include "NotebookTreeStore.h"

class NotebookSelector : public Gtk::ScrolledWindow
{
public:
	NotebookSelector(BaseObjectType *cObj, const Glib::RefPtr<Gtk::Builder>& builder);
	virtual ~NotebookSelector();
	
	//Set notebook list
	void setNotebookList(const std::vector<evernote::edam::Notebook>& notebooks);
	Gtk::TreeModel::Row addNotebook(const evernote::edam::Notebook notebook);

	//Alias for signal_notebook_change, used to connect
	sigc::signal<void>& signal_notebook_change();
	sigc::signal<void, const std::vector<evernote::edam::Notebook>&>& signal_notebook_rename();
	sigc::signal<void, const std::vector<evernote::edam::Notebook>&>& signal_notebook_delete();
	sigc::signal<evernote::edam::Notebook, const std::string&>& signal_notebook_create();
	sigc::signal<bool, const evernote::edam::Guid&, const evernote::edam::Guid&>& signal_note_change_notebook();

	//Way to update post-callback trigger
	const std::vector<evernote::edam::Guid>& getActiveNotebooks();

private:
	//Currently active notebook Guid's. Updated each time a row is selected
	std::vector<evernote::edam::Guid> activeNotebooks;

	//Path for finding click location
	Gtk::TreeModel::Path clickPath;

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

	Gtk::CellRendererText *nameRenderer;
	
	Gtk::TreeView view;
	Glib::RefPtr<NotebookTreeStore> treeModel;

	//Signal parent
	sigc::signal<void> note_change_sig;

	//Signal notebook name change
	sigc::signal<void, const std::vector<evernote::edam::Notebook>&> notebook_rename_sig;

	//Signal notebook deletion
	sigc::signal<void, const std::vector<evernote::edam::Notebook>&> notebook_delete_sig;

	//Signal (request) notebook creation
	sigc::signal<evernote::edam::Notebook, const std::string&> notebook_create_sig;

	//Boolean flag to tell draw method to invoke onRename
	bool needRename;
	//Flag to disable refresh on next onRenamed() call
	bool disableRefresh;
	
	//Handlers for menu-related events
	//Button press handler (used to detect right click as appropriate)
	bool onButtonPressEvent(GdkEventButton *event);
	void showSelectedContext(GdkEventButton *event);
	void showUnselectedContext(GdkEventButton *event);
	//Notebook and stack creation are two different things
	void onStackCreate();
	void onNotebookCreateWithStack();
	void onNotebookCreateWithoutStack();
	void onNotebookCreate(const std::string& stack);
	//Rename and delete methods are handled same for stacks and notebooks
	void onRename();
	//NOTE: DELETING ONLY HIDES NOTEBOOK ON CLIENT (and moves notes to trash). DOES NOT EXPUNGE.
	void onDelete();
	//Handler to update model column with rename
	void onRenamed(const Glib::ustring& path, const Glib::ustring& nText);

	//Invoked upon stack creation via notebook drop in model
	void onModelCreateStack(Gtk::TreeModel::Path& iter);
	//Stack change via notebook drop
	void onModelChangeStack(Gtk::TreeModel::Path& iter);

	//On draw method
	bool onDraw(const Cairo::RefPtr<Cairo::Context>& cr);
	
	//Method to emit change signal upon row selection
	void onRowSelected(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn *column);

	//Row selection method to allow proper selection on update
	bool selectRow(Glib::RefPtr<Gtk::TreeSelection> treeSelection, const Gtk::TreeModel::Children& child, const std::string& name, const evernote::edam::Guid& guid, Gtk::TreeModel::iterator *pIt = NULL);

	bool onKeyPress(GdkEventKey *event);
};

#endif
