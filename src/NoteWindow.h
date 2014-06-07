/*
 *Connor Cormier
 *5/25/14
 *Primary note window
 */

#ifndef NOTEWINDOW_H
#define NOTEWINDOW_H

#include <gtkmm.h>
#include <string>
#include <list>
#include <vector>
#include "EvernoteClient.h"
#include "NoteView.h"
#include "NotebookSelector.h"
#include "NoteSelector.h"

const std::string UI_FILE = "ui_files/betternote.glade";

class NoteWindow : public Gtk::Window
{
public:
	//Construct window using get_widget_derived
	NoteWindow(BaseObjectType* cObj, const Glib::RefPtr<Gtk::Builder>& builder);
	virtual ~NoteWindow();

private:
	//Fields
	EvernoteClient *client;
	NoteView *note;
	NotebookSelector *notebookSel;
	NoteSelector *noteSel;
	
	//Methods
	void loadObjs(const Glib::RefPtr<Gtk::Builder>& builder);

	//Visibility update methods
	void refresh();
	void showCurNotebooks(), showCurNotes(), showCurNote();
	void onNotebookSelected(), onNoteSelected();
	void updateVisibleNote();

	//Override default handler to capture tab key
	virtual bool on_key_press_event(GdkEventKey *event);
	
	//Listen for key press events
	bool onKeyPress(GdkEventKey *event);

};

#endif
