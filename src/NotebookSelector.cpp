/*
 *Connor Cormier
 *5/27/14
 *View for displaying notebook hierarchy
 */

#include "NotebookSelector.h"

using namespace std;
using namespace evernote::edam;

NotebookSelector::NotebookSelector(BaseObjectType *cObj, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::ScrolledWindow(cObj)
{
	//Add the treeview to this scrolled window
	add(view);

	//Only show scroll bars when necessary
	set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	treeModel = Gtk::TreeStore::create(colModel);
	view.set_model(treeModel);

	//Make sure the signal is triggered with only a single click!
	view.set_activate_on_single_click(true);
	
	//Connect row selection with appropriate method
	view.signal_row_activated().connect(sigc::mem_fun(*this, &NotebookSelector::onRowSelected));

	//Show view column
	view.append_column("Notebook", colModel.nameCol);

	show_all_children();
}

NotebookSelector::~NotebookSelector()
{
}

void NotebookSelector::setNotebookList(const vector<Notebook>& notebooks)
{
	//Clear out existing rows
	treeModel->clear();
	notebookStackRows.clear();

	//Add in each notebook
	for (size_t i = 0; i < notebooks.size(); i++)
		addNotebook(notebooks[i]);
}

void NotebookSelector::addNotebook(const Notebook notebook)
{
	Gtk::TreeModel::Row row;
	
	//Add the notebook to an existing stack row if one exists
	if (notebook.stack != "")
	{
		bool found = false;
		
		//Try to find the existing notebook stack
		for (size_t i = 0; i < notebookStackRows.size(); i++)
		{
			if (notebookStackRows[i][colModel.nameCol] == notebook.stack)
			{
				row = *(treeModel->append(notebookStackRows[i].children()));
				found = true;
				break;
			}
		}

		//Create a stack if none exists
		if (!found)
		{
			Gtk::TreeModel::Row stackRow = *(treeModel->append());
			stackRow[colModel.nameCol] = notebook.stack;
			notebookStackRows.push_back(stackRow);
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
}

sigc::signal<void>& NotebookSelector::signal_notebook_change()
{
	return note_change_sig;
}

const vector<Guid>& NotebookSelector::getActiveNotebooks()
{
	return activeNotebooks;
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
		if (childIter.begin() == childIter.end())
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
