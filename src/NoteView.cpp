/*
 *Connor Cormier
 *5/27/14
 *NoteView implementation
 */

#include "NoteView.h"
//#include <fstream>

using namespace std;
using namespace evernote::edam;

NoteView::NoteView(BaseObjectType* cObj, const Glib::RefPtr<Gtk::Builder>& builder) : Gtk::Box(cObj)
{
	loadWidgets(builder);

	showWebView();
	
	//TODO decide primary window or box for key listener.
	
	//Set the box to listen for key input
	this->signal_key_press_event().connect(sigc::mem_fun(*this, &NoteView::onKeyPress));
}

NoteView::~NoteView()
{
	gtk_widget_destroy(GTK_WIDGET(view));
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

void NoteView::showHtml(const string& html)
{
	webkit_web_view_load_string(view, html.c_str(), NULL, NULL, NULL);
}

string NoteView::getHtml()
{	
	//Get data from source
	//Code from Ivaldi: http://stackoverflow.com/questions/19763133/how-can-i-get-the-source-code-which-has-been-rendered-by-webkitgtk
	WebKitDOMDocument *doc = webkit_web_view_get_dom_document(view);
	WebKitDOMElement *root = webkit_dom_document_query_selector(doc, ":root", NULL);
	const gchar* content = webkit_dom_html_element_get_outer_html(WEBKIT_DOM_HTML_ELEMENT(root));

	//Alternate: use javascript to put html into title and then get directly.
	//No dom document generation
	//webkit_web_view_execute_script(view, "document.title=document.documentElement.innerHTML");
	//const gchar *content = webkit_web_view_get_title(view);

	if (!content)
		return "";

	return cleanHtml(content);
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

string NoteView::cleanHtml(string ret)
{
	size_t start = ret.find("<body>") + string("<body>").length(), len = ret.find("</body>") - start;

	//Cut the string down only to the body of the note, which is all that matters here.
	ret = ret.substr(start, len);

	size_t lPos;

	//Clear out additional </en-note> tags occuring before final one
	while ((lPos = ret.find("</en-note")) != ret.rfind("</en-note"))
		stripTag(ret, lPos);
	//Clear out additional <en-note> tags occuring after first one
	while (ret.find("<en-note") != (lPos = ret.rfind("<en-note")))
		stripTag(ret, lPos);
	
	//Add opening evernote tag if not present
	if ((lPos = ret.find("<en-note")) == string::npos)
		ret = "<en-note>" + ret;
	//If it doesn't start with the tag
	else if (lPos != 0)
	{
		cout << "Stripping start" << endl;
		stripTag(ret, "<en-note");
		ret = "<en-note>" + ret;
	}
		
	//Add ending tag if not present
	if ((lPos = ret.find("</en-note")) == string::npos)
		ret = ret + "</en-note>";
	else if (ret.find(">", lPos) != ret.length() - 1)
	{
		stripTag(ret, "</en-note");
		ret = ret + "</en-note>";
	}
	
	
	//Add in Evernote XML doctype statement
	ret = NOTE_HEAD + ret;

	/***Replace bad tags***/
	size_t lastLoc = 0;
	
	//Replace <p> with <p/> and <hr> with <hr/>
	while ((lastLoc = ret.find("<p>", lastLoc)) != string::npos)
		ret.replace(lastLoc, 3, "<p/>");

	//Reset lastLoc
	lastLoc = 0;
	
	while ((lastLoc = ret.find("<hr>", lastLoc)) != string::npos)
		ret.replace(lastLoc, 4, "<hr/>");

	//Reset again
	lastLoc = 0;
	
	//<br> should be <br /> for evernote to not complain :P. All <br> tags, regardless of content, must end in />
	string potentialDanger = "<br";
	
	while ((lastLoc = ret.find(potentialDanger, lastLoc)) != string::npos)
	{
		lastLoc = ret.find(">", lastLoc);

		if (ret[lastLoc - 1] != '/')
			ret.insert(lastLoc, "/");
	}
	
	return ret;
}

void NoteView::showNote(const Note& note)
{
	this->note = note;
	this->noteTitle->set_text(note.title);

	//Write input to file for debugging
	//ofstream out("in.html");
	//out << note.content;
	//out.close();

	this->showHtml(note.content);
}

const Note& NoteView::getNote()
{
	//Update note content and title
	this->note.content = getHtml();
	this->note.title = noteTitle->get_text();

	//Print out current note content
	//ofstream out("out.html");
	//out << note.content;
	//out.close();

	return note;
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
					cout << this->getHtml() << endl;
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
