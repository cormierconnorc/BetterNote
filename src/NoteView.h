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

class NoteView : public Gtk::Box
{
public:
	NoteView(BaseObjectType* cObj, const Glib::RefPtr<Gtk::Builder>& builder);
	virtual ~NoteView();

	//Display html
	void showHtml(const std::string& html);
	
	//Get currently displayed html
	std::string getHtml();
	
	void showNote(const evernote::edam::Note& note);
	const evernote::edam::Note& getNote();

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
	evernote::edam::Note note;

	//Gtkmm widgets that belong to the note view, accessed via builder
	Gtk::Entry *noteTitle;
	Gtk::ScrolledWindow *primaryWindow;

	//WebKit elements
	WebKitWebView *view;

	//init methods
	void loadWidgets(const Glib::RefPtr<Gtk::Builder>& builder), showWebView();

	//Format html to evernote standard
	void stripTag(std::string& ret, const std::string& remove), stripTag(std::string& ret, size_t tagPos);
	std::string cleanHtml(std::string ret);

	//Key listener to change editing actions
	bool onKeyPress(GdkEventKey *event);

	//Button listeners
	void onOutdentClick(), onIndentClick(), onInsertOrderedClick(), onInsertUnorderedClick();
};

#endif
