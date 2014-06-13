/*
 *Connor Cormier
 *5/27/14
 *NoteView implementation
 */

#include "NoteView.h"
#include <iostream>

#define BUF_FLAG "I_AM_A_BUFFER_AMA"

using namespace std;
using namespace evernote::edam;

NoteView::NoteView(BaseObjectType* cObj, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Box(cObj), db(NULL)
{
	//Base url for relative purposes
	baseUrl = Glib::get_current_dir();
	
	loadWidgets(builder);

	showWebView();
	
	//Set the box to listen for key input
	this->signal_key_press_event().connect(sigc::mem_fun(*this, &NoteView::onKeyPress));
}

NoteView::~NoteView()
{
	gtk_widget_destroy(GTK_WIDGET(view));
}

void NoteView::setDatabase(DatabaseClient *db)
{
	this->db = db;
}

void NoteView::showEnml(string enml)
{
	this->toHtml(enml);
	
	webkit_web_view_load_string(view, enml.c_str(), NULL, NULL, "file://");
}

string NoteView::getEnml()
{	
	//Get data from source
	//Code from Ivaldi: http://stackoverflow.com/questions/19763133/how-can-i-get-the-source-code-which-has-been-rendered-by-webkitgtk
	WebKitDOMDocument *doc = webkit_web_view_get_dom_document(view);
	WebKitDOMElement *root = webkit_dom_document_query_selector(doc, ":root", NULL);
	const gchar* content = webkit_dom_html_element_get_outer_html(WEBKIT_DOM_HTML_ELEMENT(root));

	string sContent;
	
	if (!content)
		sContent = "";
	else
		sContent = content;

	//Convert content to enml
	toEnml(sContent);
	
	return sContent;
}

void NoteView::showNote(const Guid& noteGuid)
{
	if (db->getNoteByGuid(note, noteGuid))
	{
		this->noteTitle->set_text(note.title);
		this->showEnml(note.content);
	}
	//If the note couldn't be retrieved, set up a buffer instead.
	else
	{
		note.guid = BUF_FLAG;

		this->noteTitle->set_text("Buffer");
		this->showEnml("This buffer is for notes you don't want to save.<br/>Select or create a note if you wish to save your work.");
		//Image path example
		//this->showEnml("<img src=\"" + baseUrl + "/res/HavasuFalls.jpg\"/>");
	}
}

const Note& NoteView::getNote()
{
	return note;
}

void NoteView::updateNote()
{
	//Update note content and title
	this->note.content = getEnml();
	this->note.title = noteTitle->get_text();
}

void NoteView::saveNote()
{
	//Update the note
	updateNote();

	//Now place the updated content in the database if it is an actual note
	if (note.guid != BUF_FLAG)
	{
		db->updateNote(note);
		db->flagDirty(note);
	}
}

bool NoteView::execDom(const std::string& command, const std::string& commandVal)
{
	WebKitDOMDocument *domDoc = webkit_web_view_get_dom_document(view);
	
	return webkit_dom_document_exec_command(domDoc, command.c_str(), false, commandVal.c_str());
}

bool NoteView::queryDomComState(const std::string& command)
{
	WebKitDOMDocument *domDoc = webkit_web_view_get_dom_document(view);

	return webkit_dom_document_query_command_state(domDoc, command.c_str());
}

bool NoteView::isNoteFocus()
{
	return gtk_widget_is_focus(GTK_WIDGET(view));
}

void NoteView::tabAction()
{
	if (queryDomComState("insertUnorderedList") || queryDomComState("insertOrderedList"))
		this->execDom("indent");
	else
		this->execDom("insertHTML", "&nbsp;&nbsp;&nbsp;&nbsp;");
}

void NoteView::loadWidgets(const Glib::RefPtr<Gtk::Builder>& builder)
{
	//Set up other elements of note view gui
	builder->get_widget("NoteTitle", noteTitle);
	builder->get_widget("NoteView", primaryWindow);

	//Set up buttons
	Gtk::Button *temp;
	builder->get_widget("bOutdent", temp);
	temp->signal_clicked().connect(sigc::mem_fun(*this, &NoteView::onOutdentClick));
	builder->get_widget("bIndent", temp);
	temp->signal_clicked().connect(sigc::mem_fun(*this, &NoteView::onIndentClick));
	builder->get_widget("bInsertOrdered", temp);
	temp->signal_clicked().connect(sigc::mem_fun(*this, &NoteView::onInsertOrderedClick));
	builder->get_widget("bInsertUnordered", temp);
	temp->signal_clicked().connect(sigc::mem_fun(*this, &NoteView::onInsertUnorderedClick));
}

void NoteView::showWebView()
{
	//Create the webkit web view
	view = WEBKIT_WEB_VIEW(webkit_web_view_new());

	//Set to expand
	gtk_widget_set_hexpand(GTK_WIDGET(view), true);
	gtk_widget_set_vexpand(GTK_WIDGET(view), true);

	//Show
	gtk_widget_show(GTK_WIDGET(view));

	//Set editable
	webkit_web_view_set_editable(view, true);

	//Show it in the container
	gtk_container_add(GTK_CONTAINER(primaryWindow->gobj()), GTK_WIDGET(view));
}

void NoteView::toHtml(string& ret)
{
	//Remove everything before and after <en-note> tags
	size_t bodyStart = ret.find("<en-note");
	size_t bodyEnd = ret.find("</en-note>");

	//If not a complete note, just start at the beginning
	if (bodyStart == string::npos)
		bodyStart = 0;
	//If a valid note, seek the end of the <en-note tag
	else
		bodyStart = ret.find(">", bodyStart) + 1;

	//If not a complete note, just end at the end
	if (bodyEnd == string::npos)
		bodyEnd = ret.length();

	size_t len = bodyEnd - bodyStart;

	ret = ret.substr(bodyStart, len);

	//Now add html start and end tags
	ret = "<html><body>" + ret + "</body></html>";
}

void NoteView::toEnml(string& ret)
{
	size_t start = ret.find("<body>") + string("<body>").length(),
			len = ret.find("</body>") - start;

	//Cut the string down only to the body of the note, which is all that matters here.
	ret = ret.substr(start, len);

	//Add in the note formatting
	ret = NOTE_HEAD + "<en-note>" + ret + "</en-note>";

	//Close the tags that webkit doesn't like to close.
	insertTagClose(ret, "<p");
	insertTagClose(ret, "<hr");
	insertTagClose(ret, "<br");
}

void NoteView::insertTagClose(string& ret, const string& tagOpen, const string& insert)
{
	size_t lastLoc = 0;

	while ((lastLoc = ret.find(tagOpen, lastLoc)) != string::npos)
	{
		lastLoc = ret.find(">", lastLoc);

		if (ret.substr(lastLoc - insert.length(), insert.length()) != insert)
			ret.insert(lastLoc, insert);
	}
}

void NoteView::stripTag(string& ret, const string& remove)
{
	size_t start = ret.find(remove);

	if (start == string::npos)
		return;
	else
		stripTag(ret, start);
}

void NoteView::stripTag(string& ret, size_t tagPos)
{
	size_t end = ret.find(">", tagPos);

	ret.erase(tagPos, end - tagPos + 1);	
}

bool NoteView::onKeyPress(GdkEventKey *event)
{
	//Only capture control key events
	if (event->state & GDK_CONTROL_MASK)
	{
		//Ctrl+shift
		if (event->state & GDK_SHIFT_MASK)
		{
			//Ctrl+Shift options
			switch (event->keyval)
			{
				//Ctrl++ to sup
				case GDK_KEY_plus:
					execDom("superscript");
					return true;
				//Ctrl+Shift+b for bulleted list (unordered)
				case GDK_KEY_B:
					execDom("insertUnorderedList");
					return true;
				//Ctrl+Shift+o to insert ordered list
				case GDK_KEY_O:
					execDom("insertOrderedList");
					return true;
				//Ctrl+Shift+m to outdent
				case GDK_KEY_M:
					execDom("outdent");
					return true;
			}
			
		}
		//Ctrl+key
		else
		{
			//Ctrl options
			switch (event->keyval)
			{
				//Ctrl+s to save the note
				case GDK_KEY_s:
					this->saveNote();
					return true;
				//Ctrl+= to sub
				case GDK_KEY_equal:
					execDom("subscript");
					return true;
				//Ctrl+T to strike through
				case GDK_KEY_t:
					execDom("strikethrough");
					return true;
				//Ctrl+b to bold
				case GDK_KEY_b:
					execDom("bold");
					return true;
				//Ctrl+i to italicize
				case GDK_KEY_i:
					execDom("italic");
					return true;
				//Ctrl+u to underline
				case GDK_KEY_u:
					execDom("underline");
					return true;
				//Ctrl+e to center
				case GDK_KEY_e:
					execDom("justifyCenter");
					return true;
				//Ctrl+l to justify left
				case GDK_KEY_l:
					execDom("justifyLeft");
					return true;
				//Ctrl+r to justify right
				case GDK_KEY_r:
					execDom("justifyRight");
					return true;
				//Ctrl+m to indent
				case GDK_KEY_m:
					execDom("indent");
					return true;
				//Ctrl+p to print note text to console
				case GDK_KEY_p:
					cout << this->getEnml() << endl;
					return true;					
			}
		}
	}
	//No control, no action

	return false;
}

//The button listeners
void NoteView::onOutdentClick()
{
	execDom("outdent");
}

void NoteView::onIndentClick()
{
	execDom("indent");
}

void NoteView::onInsertOrderedClick()
{
	execDom("insertOrderedList");
}

void NoteView::onInsertUnorderedClick()
{
	execDom("insertUnorderedList");
}
