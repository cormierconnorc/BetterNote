/*
 *Connor Cormier
 *5/27/14
 *View for displaying notebook hierarchy
 */

#include "NotebookSelector.h"

using namespace std;
using namespace evernote::edam;

NotebookSelector::NotebookSelector(BaseObjectType *cObj, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::ScrolledWindow(cObj), needRename(false), disableRefresh(false)
{
	//Add the treeview to this scrolled window
	add(view);

	//Only show scroll bars when necessary
	set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	treeModel = NotebookTreeStore::create(colModel);

	//Set the treeModel's callback
	treeModel->signal_create_stack().connect(sigc::mem_fun(*this, &NotebookSelector::onModelCreateStack));
	treeModel->signal_change_stack().connect(sigc::mem_fun(*this, &NotebookSelector::onModelChangeStack));
	
	view.set_model(treeModel);

	//Make sure the signal is triggered with only a single click!
	view.set_activate_on_single_click(true);

	view.set_reorderable(true);
	
	//Connect row selection with appropriate method
	view.signal_row_activated().connect(sigc::mem_fun(*this, &NotebookSelector::onRowSelected));

	//Register signal handler for right click and popup menu signals.
	//Both must be handled prior to the default
	view.signal_button_press_event().connect(sigc::mem_fun(*this, &NotebookSelector::onButtonPressEvent), false);

	//Signal key press to rename notebook
	view.signal_key_press_event().connect(sigc::mem_fun(*this, &NotebookSelector::onKeyPress), false);

	//Show view column
	int colNum = view.append_column("Notebook", colModel.nameCol);
	
	//Get the cell renderer for the name column
	nameRenderer = (Gtk::CellRendererText*)view.get_column_cell_renderer(colNum - 1);

	//Set handler for editing
	nameRenderer->signal_edited().connect(sigc::mem_fun(*this, &NotebookSelector::onRenamed));

	//Set handler for redraw (needed to focus on update from Tree Store
	view.signal_draw().connect(sigc::mem_fun(*this, &NotebookSelector::onDraw));

	show_all_children();
}

NotebookSelector::~NotebookSelector()
{
}

void NotebookSelector::setNotebookList(const vector<Notebook>& notebooks)
{
	//Get currently selected row
	Glib::RefPtr<Gtk::TreeSelection> treeSelection = view.get_selection();
	Gtk::TreeModel::iterator iter = treeSelection->get_selected();
	string name;
	Guid guid;
	
	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;
		name = row.get_value(colModel.nameCol);
		guid = row.get_value(colModel.guid);
	}
	
	//Clear out existing rows
	treeModel->clear();

	//Add in each notebook
	for (size_t i = 0; i < notebooks.size(); i++)
		addNotebook(notebooks[i]);

	//Try to select the previously selected row if one existed
	if (iter)
		selectRow(treeSelection, treeModel->children(), name, guid);
}

bool NotebookSelector::selectRow(Glib::RefPtr<Gtk::TreeSelection> treeSelection, const Gtk::TreeModel::Children& child, const string& name, const Guid& guid, Gtk::TreeModel::iterator *pIt)
{
	for (Gtk::TreeModel::iterator it = child.begin(); it != child.end(); it++)
	{
		
		Gtk::TreeModel::Row tRow = *it;

		
		if (name == tRow.get_value(colModel.nameCol) && guid == tRow.get_value(colModel.guid))
		{
			//Expand parent if one exists
			if (pIt != NULL)
				view.expand_row(treeModel->get_path(*pIt), false);
			
			treeSelection->select(tRow);
			return true;
		}

		//Now pass the children in recursively
		bool retVal = selectRow(treeSelection, tRow.children(), name, guid, &it);

		if (retVal)
			return true;
	}

	return false;
}

Gtk::TreeModel::Row NotebookSelector::addNotebook(const Notebook notebook)
{
	Gtk::TreeModel::Row row;
	
	//Add the notebook to an existing stack row if one exists
	if (notebook.stack != "")
	{
		bool found = false;

		//Try to find the existing notebook stack
		for (Gtk::TreeModel::iterator outerIter = treeModel->children().begin(); outerIter != treeModel->children().end(); outerIter++)
		{
			if (outerIter->get_value(colModel.guid) == STACK_GUID &&
				outerIter->get_value(colModel.nameCol) == notebook.stack)
			{				
				row = *(treeModel->append(outerIter->children()));
				found = true;

				//Expand the stack after insertion
				view.expand_row(treeModel->get_path(outerIter), false);
				
				break;
			}
		}

		//Create a stack if none exists
		if (!found)
		{
			Gtk::TreeModel::Row stackRow = *(treeModel->append());
			stackRow[colModel.nameCol] = notebook.stack;
			stackRow[colModel.guid] = STACK_GUID;

			//Open the row
			view.expand_row(treeModel->get_path(stackRow), false);
			
			//Now set the row
			row = *(treeModel->append(stackRow.children()));
		}
		
	}
	//Otherwise, put it in as a top-lel notebook
	else
	{
		row = *(treeModel->append());
	}

	//Now set row values to those of notebook
	row[colModel.nameCol] = notebook.name;
	row[colModel.guid] = notebook.guid;

	//Return the row to allow further changes
	return row;
}

sigc::signal<void>& NotebookSelector::signal_notebook_change()
{
	return note_change_sig;
}

sigc::signal<void, const vector<Notebook>&>& NotebookSelector::signal_notebook_rename()
{
	return notebook_rename_sig;
}

sigc::signal<void, const vector<Notebook>&>& NotebookSelector::signal_notebook_delete()
{
	return notebook_delete_sig;
}

sigc::signal<Notebook, const string&>& NotebookSelector::signal_notebook_create()
{
	return notebook_create_sig;
}

const vector<Guid>& NotebookSelector::getActiveNotebooks()
{
	return activeNotebooks;
}

void NotebookSelector::showSelectedContext(GdkEventButton *event)
{
	Gtk::Menu *selMenu = Gtk::manage(new Gtk::Menu());

	Gtk::TreeModel::iterator iter;
	
	//If selection was a notebook
	if ((iter = treeModel->get_iter(clickPath)) && iter->get_value(colModel.guid) != STACK_GUID)
	{
		Gtk::MenuItem *item = Gtk::manage(new Gtk::MenuItem("_Rename Notebook", true));
		item->signal_activate().connect(sigc::mem_fun(*this, &NotebookSelector::onRename));
		selMenu->append(*item);

		item = Gtk::manage(new Gtk::MenuItem("_Delete Notebook", true));
		item->signal_activate().connect(sigc::mem_fun(*this, &NotebookSelector::onDelete));
		selMenu->append(*item);
	}
	//If selection was a stack
	else
	{
		Gtk::MenuItem *item = Gtk::manage(new Gtk::MenuItem("_Rename Stack", true));
		item->signal_activate().connect(sigc::mem_fun(*this, &NotebookSelector::onRename));
		selMenu->append(*item);

		item = Gtk::manage(new Gtk::MenuItem("_Delete Stack", true));
		item->signal_activate().connect(sigc::mem_fun(*this, &NotebookSelector::onDelete));
		selMenu->append(*item);

		item = Gtk::manage(new Gtk::MenuItem("_Create Notebook In Stack", true));
		item->signal_activate().connect(sigc::mem_fun(*this, &NotebookSelector::onNotebookCreateWithStack));
		selMenu->append(*item);
	}

	//Accelerate and show children for each menu
	selMenu->accelerate(*this);
	selMenu->show_all();

	selMenu->popup(event->button, event->time);

}

void NotebookSelector::showUnselectedContext(GdkEventButton *event)
{
	Gtk::Menu *nSelMenu = Gtk::manage(new Gtk::Menu());
	
	Gtk::MenuItem *item = Gtk::manage(new Gtk::MenuItem("_Create New Notebook", true));
	item->signal_activate().connect(sigc::mem_fun(*this, &NotebookSelector::onNotebookCreateWithoutStack));
	nSelMenu->append(*item);

	item = Gtk::manage(new Gtk::MenuItem("_Create New Stack", true));
	item->signal_activate().connect(sigc::mem_fun(*this, &NotebookSelector::onStackCreate));
	nSelMenu->append(*item);
	
	nSelMenu->accelerate(*this);
	nSelMenu->show_all();

	nSelMenu->popup(event->button, event->time);
}

bool NotebookSelector::onButtonPressEvent(GdkEventButton *event)
{
	//Disable editing upon click
	nameRenderer->stop_editing();
	
	//Handle right clicks only
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

void NotebookSelector::onStackCreate()
{
	Gtk::TreeModel::iterator iter = treeModel->append();

	iter->set_value(colModel.nameCol, DEFAULT_STACK);
	iter->set_value(colModel.guid, STACK_GUID);

	//Now set the click path and pass to rename
	clickPath = treeModel->get_path(iter);
	this->disableRefresh = true;
	onRename();
}

void NotebookSelector::onNotebookCreateWithStack()
{
	Gtk::TreeModel::iterator iter = treeModel->get_iter(clickPath);

	if (iter)
		onNotebookCreate(iter->get_value(colModel.nameCol));
	else
		onNotebookCreateWithoutStack();
}

void NotebookSelector::onNotebookCreateWithoutStack()
{
	onNotebookCreate("");
}

void NotebookSelector::onNotebookCreate(const string& stack)
{
	//Get a new notebook instance from the window via a signal
	//God, these signals are useful
	Notebook newNote = notebook_create_sig.emit(stack);

	Gtk::TreeModel::Row row = addNotebook(newNote);

	//Now get the path of the row and do rename
	clickPath = treeModel->get_path(row);

	//Rename
	onRename();
}

void NotebookSelector::onRename()
{
	//Make editable
	nameRenderer->property_editable() = true;

	//Start editing the appropriate cell
	view.set_cursor(clickPath, *view.get_column(0), *nameRenderer, true);
}

void NotebookSelector::onDelete()
{
	//Add all Guid's to a vector
	vector<Notebook> deleted;
	Notebook n;
	n.__isset.guid = true;

	Gtk::TreeModel::iterator iter = treeModel->get_iter(clickPath);

	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;
		
		if (row.get_value(colModel.guid) != STACK_GUID)
		{
			n.guid = row.get_value(colModel.guid);
			deleted.push_back(n);
		}
		else
		{
			for (Gtk::TreeModel::iterator cIt = row.children().begin(); cIt != row.children().end(); cIt++)
			{
				n.guid = cIt->get_value(colModel.guid);
				deleted.push_back(n);
			}
		}

		notebook_delete_sig.emit(deleted);
	}
}

void NotebookSelector::onRenamed(const Glib::ustring& path, const Glib::ustring& nText)
{
	//Disable editing for the renderer
	nameRenderer->property_editable() = false;
	
	//Update the model column's value
	Gtk::TreeModel::iterator iter = treeModel->get_iter(path);

	if (iter)
	{		
		Gtk::TreeModel::Row row = *iter;
		row[colModel.nameCol] = nText;

		//Now update the name of the notebook/stack
		vector<Notebook>  changeNotes;

		//Value changed was notebook, not stack. No need to update stack values
		if (row.get_value(colModel.guid) != STACK_GUID)
		{
			Notebook n;
			n.__isset.guid = true;
			n.__isset.name = true;
			
			n.guid = row.get_value(colModel.guid);
			n.name = row.get_value(colModel.nameCol);
			
			changeNotes.push_back(n);
		}
		else
		{
			for (Gtk::TreeModel::iterator iter = row.children().begin(); iter != row.children().end(); iter++)
			{
				Notebook n;
				n.__isset.guid = true;
				n.__isset.stack = true;

				Gtk::TreeModel::Row cRow = *iter;

				n.guid = cRow.get_value(colModel.guid);
				n.stack = row.get_value(colModel.nameCol);

				changeNotes.push_back(n);
			}
		}

		if (!disableRefresh)
			notebook_rename_sig.emit(changeNotes);
		else
			disableRefresh = false;
	}
}

void NotebookSelector::onModelCreateStack(Gtk::TreeModel::Path& path)
{
	clickPath = path;

	needRename = true;
}

void NotebookSelector::onModelChangeStack(Gtk::TreeModel::Path& path)
{
	Gtk::TreeModel::iterator iter = treeModel->get_iter(path);
	Gtk::TreeModel::iterator pIter;
	bool hasStack = false;

	Gtk::TreeModel::Path pathCopy = path;
	
	if (pathCopy.up() && pathCopy.up())
	{
		path.up();
		hasStack = true;
		pIter = treeModel->get_iter(path);
	}
	
	//Just get the stack name and switch it out in the database
	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;

		Notebook n;
		n.__isset.guid = true;
		n.__isset.stack = true;

		n.guid = row.get_value(colModel.guid);

		//Now get the associated stack
		if (hasStack)
			n.stack = pIter->get_value(colModel.nameCol);
		else
			n.stack = "";

		//Now update it in the notebook (placing in a vector, as necessary to use rename method
		vector<Notebook> changeNotes;
		changeNotes.push_back(n);
		notebook_rename_sig.emit(changeNotes);
	}
}

bool NotebookSelector::onDraw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	//Must wait until widget is redrawn/realized to invoke rename method
	if (needRename)
	{
		onRename();
		needRename = false;
	}

	return false;
}

void NotebookSelector::onRowSelected(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column)
{
	//Update the active notebooks prior to submitting update to parent (activeNotebooks)
	//Find location based on path iterator:
	Gtk::TreeModel::iterator iter = treeModel->get_iter(path);

	//Reset active
	activeNotebooks.clear();

	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;

		const Gtk::TreeNodeChildren& childIter = row.children();

		//Check if stack or notebook.
		//If notebook, simply place it's Guid into the vector
		if (row.get_value(colModel.guid) != STACK_GUID)
		{
			//This is the only one
			activeNotebooks.push_back(row[colModel.guid]);
		}
		else
		{
			for (Gtk::TreeIter it = childIter.begin(); it != childIter.end(); it++)
				activeNotebooks.push_back((*it)[colModel.guid]);
		}
	
		//Emit the signal to alert the parent
		note_change_sig.emit();
	}
}

bool NotebookSelector::onKeyPress(GdkEventKey *event)
{
	if (event->keyval == GDK_KEY_F2 || event->keyval == GDK_KEY_Delete)
	{
		vector<Gtk::TreeModel::Path> selected = view.get_selection()->get_selected_rows();
		if (selected.size() == 1)
		{
			clickPath = selected[0];

			if (event->keyval == GDK_KEY_Delete)
				onDelete();
			else
				onRename();
		}
		return true;
	}

	return false;
}
