/*
 *Connor Cormier
 *5/27/14
 *NoteView class: handles the display of notes
 */

#ifndef NOTEVIEW_H
#define NOTEVIEW_H

#include <gtkmm.h>
#include <string>
#include "evernote_api/src/NoteStore.h"
#include <webkit/webkitwebview.h>
#include "DatabaseClient.h"

const std::string NOTE_HEAD = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">\n";

class NoteView : public Gtk::Box
{
public:
	NoteView(BaseObjectType* cObj, const Glib::RefPtr<Gtk::Builder>& builder);
	virtual ~NoteView();

	//Set the database connection used by this NoteView, which allows it to manage its contents
	void setDatabase(DatabaseClient *db);
	
	//Display html
	void showEnml(std::string enml);
	
	//Get currently displayed html
	std::string getEnml();

	//Note access methods
	void showNote(const evernote::edam::Guid& noteGuid);
	const evernote::edam::Note& getNote();
	void updateNote();
	void saveNote();

	//Execute commands on DOM
	bool execDom(const std::string& command, const std::string& commandVal = "");
	bool queryDomComState(const std::string& command);

	//Check if note (webkitwebview) is focus
	bool isNoteFocus();

	//Special actions to take when tab is pressed, context dependent
	//Separate from key press listener since tab keys don't make it
	//past the window's default handler.
	void tabAction();
	
private:
	//Database connection
	DatabaseClient *db;

	//Base url (current working directory) used for relative path
	std::string baseUrl;
	
	//Currently displayed note
	evernote::edam::Note note;

	//Gtkmm widgets that belong to the note view, accessed via builder
	Gtk::Entry *noteTitle;
	Gtk::ScrolledWindow *primaryWindow;

	//WebKit elements
	WebKitWebView *view;

	//init methods
	void loadWidgets(const Glib::RefPtr<Gtk::Builder>& builder);
	void showWebView();

	//Formatting methods
	void toHtml(std::string& enml);
	void toEnml(std::string& html);
	void replaceTag(std::string& ret, const std::string& orig, const std::string& repl);
	void insertTagClose(std::string& ret, const std::string& tagOpen, const std::string& insert="/");
	void stripTag(std::string& ret, const std::string& remove);
	void stripTag(std::string& ret, size_t tagPos);

	//Key listener to change editing actions
	bool onKeyPress(GdkEventKey *event);

	//Button listeners
	void onOutdentClick();
	void onIndentClick();
	void onInsertOrderedClick();
	void onInsertUnorderedClick();
};

#endif
