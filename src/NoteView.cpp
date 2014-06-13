/*
 *Connor Cormier
 *5/27/14
 *NoteView implementation
 */

#include "NoteView.h"
#include "BetternoteUtils.h"
#include <fstream>

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
		//Start by inflating resources associated with this note
		inflateResources(note);
		
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

	//Set editable
	webkit_web_view_set_editable(view, true);

	//Set signal handlers
	//Navigation handler
	g_signal_connect(G_OBJECT(view),
					 "navigation-policy-decision-requested",
					 G_CALLBACK(&NoteView::handleNavigation),
					 this);
	//Download handler
	g_signal_connect(G_OBJECT(view),
					 "download-requested",
					 G_CALLBACK(&NoteView::handleDownload),
					 this);
	
	//Show
	gtk_widget_show(GTK_WIDGET(view));

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

	//Now for the hard part: resouce replacement
	handleMedia(ret);
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

		if (lastLoc < insert.length() || ret.substr(lastLoc - insert.length(), insert.length()) != insert)
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

void NoteView::handleMedia(string& ret)
{
	//Replace <en-media> tags with their html equivalents
	size_t lastLoc = 0;

	while ((lastLoc = ret.find("<en-media", lastLoc)) != string::npos)
	{
		size_t endLoc = ret.find(">", lastLoc);

		//Needs to include ending tag, so add 1 to length
		size_t len = endLoc - lastLoc + 1;

		//Now carry out tag replacement
		ret.replace(lastLoc, len, getReplacementTag(ret.substr(lastLoc, len)));
	}
}

string NoteView::getReplacementTag(string mediaTag)
{
	//Get the identifying hash of the resource (and erase from tag)
	string hash = getProperty(mediaTag, "hash", true);

	//Convert hex to binary string:
	hash = Util::hexToBinaryString(hash);

	//Try to get the iterator to the hash
	std::map<string, ResInfo>::iterator it = resLocs.find(hash);

	//Invalid hash, strip tag
	if (it == resLocs.end())
	{
		cerr << "Could not get hash " << hash << endl;
		return "";
	}

	ResInfo res = it->second;
	
	//Get the mime type
	string mime = getProperty(mediaTag, "type", true);

	string media = "en-media";

	//Handle image
	if (mime.find("image") != string::npos)
	{
		//Carry out replacement of media tag with img
		mediaTag.replace(mediaTag.find(media), media.length(), "img");

		string srcInfo = "src=\"" + res.fileLoc + "\"";
		
		//Now add in the src tag at the end
		mediaTag.insert(mediaTag.find(">"), srcInfo);
	}
	//Currently, handle all other resources by just putting an anchor to them
	else
	{
		mediaTag.replace(mediaTag.find(media), media.length(), "a");

		//Plant a link with the 
		string hrefInfo = "href=\"" + res.fileLoc + "\" download=\"" + res.resName + "\">" + res.resName + "</a>";

		//Now insert the href at the end of the tag
		mediaTag.erase(mediaTag.find(">"));
		mediaTag = mediaTag + hrefInfo;
	}

	cout << mediaTag << endl;

	return mediaTag;
}

string NoteView::getProperty(string& mediaTag, string prop, bool strip)
{
	//Add in equals
	prop = prop + "=";

	//Find start of property value
	size_t propStart = mediaTag.find(prop);
	size_t start = propStart + prop.length();

	bool hasQuotes = (mediaTag[start] == '\"');

	if (hasQuotes)
		start++;
	
	size_t end;

	if (hasQuotes)
		end = mediaTag.find("\"", start);
	else
		end = min(mediaTag.find(" ", start), mediaTag.find(">", start));

	string propVal = mediaTag.substr(start, end - start);

	//Move end up one for space checking
	end++;

	//Strip trailing spaces from tag
	while (mediaTag[end] == ' ')
		end++;
	
	//Erase if strip is set
	if (strip)
		mediaTag.erase(propStart, end - propStart);
	
	return propVal;
}

void NoteView::inflateResources(const Note& note)
{
	//Clear out resource map
	resLocs.clear();
	
	//Get resources out of database
	vector<Resource> res;
	db->getResourcesInNote(res, note);

	//Inflate each resource
	for (vector<Resource>::iterator it = res.begin(); it != res.end(); it++)
	{
		string fileLoc = inflateResource(*it);

		ResInfo info;

		//In the resouce info, hold the location of the file and the guid of the resource
		info.fileLoc = fileLoc;
		info.resGuid = it->guid;
		info.resName = (it->attributes.__isset.fileName ? it->attributes.fileName : it->guid);

		//Place location in resource map for easy recovery. Helps with link swapping
		resLocs[it->data.bodyHash] = info;
	}
}

string NoteView::inflateResource(const Resource& res)
{
	//Determine file name
	string fileName = baseUrl + "/" + RES_DIR + (res.attributes.__isset.fileName ? res.attributes.fileName : res.guid);

	//Open file
	ofstream out;
	out.open(fileName.c_str(), ios::out | ios::binary);

	//Write to file
	if (out.is_open())
		out.write(res.data.body.c_str(), res.data.body.length());
	else
		cerr << "Could not write resource to file" << endl;

	out.close();

	return fileName;
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

gboolean NoteView::handleNavigation(WebKitWebView *view, WebKitWebFrame *frame, WebKitNetworkRequest *request, WebKitWebNavigationAction *navAction, WebKitWebPolicyDecision *decision, gpointer nView)
{
	WebKitWebNavigationReason reason = webkit_web_navigation_action_get_reason(navAction);

	//Download actually opens the file in the default program rather than downloading
	//Probably not the smartest way to handle things.
	if (reason == WEBKIT_WEB_NAVIGATION_REASON_LINK_CLICKED)
		webkit_web_policy_decision_download(decision);

	//Go default if navigation happened for some other reason
	return false;
}

gboolean NoteView::handleDownload(WebKitWebView *view, WebKitDownload *download, gpointer nView)
{
	//Launch program to view/alter
	gtk_show_uri(NULL, webkit_download_get_uri(download), GDK_CURRENT_TIME, NULL);
	
	return true;
}
