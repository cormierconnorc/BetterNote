/*
 *Connor Cormier
 *6/09/14
 *Custom tree store to control drag and drop functionality for NotebookSelector
 */

#include "NotebookTreeStore.h"
#include "evernote_api/src/NoteStore_types.h"
#include <iostream>

#define NAME_COL 0
#define GUID_COL 1

using namespace std;
using namespace evernote::edam;

NotebookTreeStore::NotebookTreeStore(const Gtk::TreeModel::ColumnRecord& columns) : Gtk::TreeStore(columns)
{
}

Glib::RefPtr<NotebookTreeStore> NotebookTreeStore::create(const Gtk::TreeModel::ColumnRecord& columns)
{
	return Glib::RefPtr<NotebookTreeStore>(new NotebookTreeStore(columns));
}

sigc::signal<void, Gtk::TreeModel::Path&>& NotebookTreeStore::signal_create_stack()
{
	return stackCreateSignal;
}

sigc::signal<void, Gtk::TreeModel::Path&>& NotebookTreeStore::signal_change_stack()
{
	return stackChangeSignal;
}

sigc::signal<bool, const Guid&, const Guid&>& NotebookTreeStore::signal_note_change_notebook()
{
	return noteChangeNotebookSignal;
}

bool NotebookTreeStore::row_draggable_vfunc(const Gtk::TreeModel::Path& path) const
{
	NotebookTreeStore *unconst = (NotebookTreeStore*)this;
	const_iterator iter = unconst->get_iter(path);

	if (iter)
	{
		Guid name;
		iter->get_value(GUID_COL, name);

		//Do not allow stacks to be dragged
		return name != STACK_GUID;
	}

	//Return default
	return Gtk::TreeStore::row_draggable_vfunc(path);
}

bool NotebookTreeStore::row_drop_possible_vfunc(const Gtk::TreeModel::Path& path, const Gtk::SelectionData& data) const
{
	/*None of this works as intended. Yay. I'll figure it out in a later version
	//External drag. Do not allow drops on stacks
	if (data.get_target() == "STRING")
	{
		Gtk::TreeModel::iterator it = ((NotebookTreeStore*)this)->get_iter(path);

		if (it)
		{
			Guid guid;
			it->get_value(GUID_COL, guid);

			if (guid == STACK_GUID)
				return false;
			else
				return true;
		}
		else
			return false;
				
	}*/
	
	//Just return default for internal drags
	return Gtk::TreeStore::row_drop_possible_vfunc(path, data);
}

bool NotebookTreeStore::drag_data_get_vfunc(const TreeModel::Path& src, Gtk::SelectionData& selectionData) const
{
	srcPath = src;

	return TreeStore::drag_data_get_vfunc(src, selectionData);
}

bool NotebookTreeStore::drag_data_received_vfunc(const Gtk::TreeModel::Path& dest, const Gtk::SelectionData& selectionData)
{
	//Internal row drag
	if (selectionData.get_target() == "GTK_TREE_MODEL_ROW")
		return dragRowDataReceived(dest, selectionData);
	//External string drag
	else if (selectionData.get_target() == "STRING")
		return dragStringDataReceived(dest, selectionData);

	return false;
}

int NotebookTreeStore::getPathDepth(Gtk::TreeModel::Path path)
{
	int depth = 0;

	while (path.up())
		depth++;

	return depth;
}

bool NotebookTreeStore::srcBefore(const TreeModel::Path& path)
{
	return srcPath < path;
}

bool NotebookTreeStore::dragRowDataReceived(const Gtk::TreeModel::Path& dest, const Gtk::SelectionData& selectionData)
{
	NotebookTreeStore *unconst = (NotebookTreeStore*)this;

	//Copy the path for testing
	Gtk::TreeModel::Path cpPath = dest;

	//Do not allow drop to be 3 deep (no stacks within stacks)
	if (getPathDepth(cpPath) == 3)
		cpPath.up();

	Guid guid;
	Glib::ustring name;
	bool needReinsert = false;
	
	if (getPathDepth(cpPath) == 2)
	{
		Gtk::TreeModel::Path parentPath = cpPath;
		parentPath.up();

		//Check if the parent is a stack. If it is, do nothing. If it is a notebook, merge them into a stack
		Gtk::TreeModel::iterator iter = unconst->get_iter(parentPath);

		if (iter)
		{
			iter->get_value(GUID_COL, guid);

			//We need to change some things up now, ya hear? Notebooks can't stack with other notebooks
			if (guid != STACK_GUID)
			{
				//Back up parent row information
				iter->get_value(NAME_COL, name);

				//Flag for reinsertion
				needReinsert = true;
				
				//Change parent row GUID to stack and name to "New Stack" (temp name prior to user editing)
				iter->set_value(NAME_COL, DEFAULT_STACK);
				iter->set_value(GUID_COL, STACK_GUID);
			}
		}
	}
	
	bool ret = Gtk::TreeStore::drag_data_received_vfunc(cpPath, selectionData);


	//Stack created via drag and drop: need to go through rename process
	if (needReinsert)
	{
		cpPath.up();
		Gtk::TreeModel::iterator pIter = unconst->get_iter(cpPath);		
		Gtk::TreeModel::iterator iter = unconst->prepend(pIter->children());
		iter->set_value(GUID_COL, guid);
		iter->set_value(NAME_COL, name);

		//If the last removed row was before this, the iterator is invalid
		if (srcBefore(cpPath))
		{
			//Move the path back to account for removal
			cpPath.prev();
		}
		

		stackCreateSignal.emit(cpPath);
	}
	//Single notebook change, just emit the notification signal
	else
	{
		stackChangeSignal.emit(cpPath);
	}

	return ret;
}

bool NotebookTreeStore::dragStringDataReceived(const Gtk::TreeModel::Path& dest, const Gtk::SelectionData& selectionData)
{
	Guid noteGuid = selectionData.get_text();

	Guid notebookGuid;
	Gtk::TreeModel::iterator it = ((NotebookTreeStore*)this)->get_iter(dest);

	if (it)
	{
		it->get_value(GUID_COL, notebookGuid);

		//Can't add note directly to stack.
		if (notebookGuid == STACK_GUID)
			return false;

		return noteChangeNotebookSignal.emit(noteGuid, notebookGuid);
	}
	else
		return false;
}
