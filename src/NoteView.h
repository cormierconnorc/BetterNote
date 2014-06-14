/*
 *Connor Cormier
 *5/27/14
 *NoteView class: handles the display of notes
 */

#ifndef NOTEVIEW_H
#define NOTEVIEW_H

#include <gtkmm.h>
#include <string>
#include <map>
#include "evernote_api/src/NoteStore.h"
#include <webkit/webkit.h>
#include "DatabaseClient.h"

const std::string NOTE_HEAD = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE en-note SYSTEM \"http://xml.evernote.com/pub/enml2.dtd\">\n";
const std::string RES_DIR = "res/";
const std::string FILE_SRC = "file://";

//Structure to hold information on inflated file
struct ResInfo
{
	std::string fileLoc;
	std::string resGuid;
	std::string resName;
};

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
	std::map<std::string, ResInfo> resLocs;
	std::map<std::string, ResInfo> updateRes;

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

	//Media formatting
	void handleMediaInflate(std::string& ret);
	void handleMediaDeflate(std::string& ret);
	void doTagReplace(std::string& ret, const std::string& tabOpen, std::string (NoteView::*getReplacement)(std::string));
	std::string getInflateReplacementTag(std::string mediaTag);
	//Note: deflation replacement method also handles resource updating
	//in a bit of poor design that will likely be remedied later on
	std::string getDeflateReplacementTag(std::string mediaTag);
	std::string getProperty(std::string& mediaTag, std::string prop, bool strip = false);

	//Resource management methods
	void inflateResources(const evernote::edam::Note& note);
	std::string inflateResource(const evernote::edam::Resource& res);

	//Resource finalization
	void removeOrphans();
	void finalizeResources();
	//Delete a file only if it is in the resource directory,
	//return true if deleted, false otherwise
	bool deleteIfInRes(const std::string& fileLoc);

	//Key listener to change editing actions
	bool onKeyPress(GdkEventKey *event);

	//Button listeners
	void onOutdentClick();
	void onIndentClick();
	void onInsertOrderedClick();
	void onInsertUnorderedClick();

	//Web view signal handlers. Must be static, receiving this as a parameter.
	static gboolean handleNavigation(WebKitWebView *view,
							  WebKitWebFrame *frame,
							  WebKitNetworkRequest *request,
							  WebKitWebNavigationAction *navAction,
							  WebKitWebPolicyDecision *decision,
							  gpointer nView);

	static gboolean handleDownload(WebKitWebView *view,
								   WebKitDownload *download,
								   gpointer nView);

	static void handlePaste(GtkWidget *viewWidget,
							GdkDragContext *context,
							gint x,
							gint y,
							GtkSelectionData *data,
							guint info,
							guint time,
							gpointer nView);
};

#endif
