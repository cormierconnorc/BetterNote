/*
 *Connor Cormier
 *5/27/14
 *NoteView implementation
 */

#include "NoteView.h"
#include "BetternoteUtils.h"
#include <cstdio>

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
	//Deflate resources
	finalizeResources();
	
	gtk_widget_destroy(GTK_WIDGET(view));
}

void NoteView::setDatabase(DatabaseClient *db)
{
	this->db = db;
}

void NoteView::showEnml(string enml)
{
	this->toHtml(enml);
	
	webkit_web_view_load_string(view, enml.c_str(), NULL, NULL, FILE_SRC.c_str());
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
	//Finalize the previous note
	finalizeResources();
	
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

	//Show
	gtk_widget_show(GTK_WIDGET(view));

	//Show it in the container
	gtk_container_add(GTK_CONTAINER(primaryWindow->gobj()), GTK_WIDGET(view));

	
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
	//Bind node insertion
	g_signal_connect(G_OBJECT(view),
					 "should-insert-node",
					 G_CALLBACK(&NoteView::handleInsert),
					 this);
	//Bind text insertion
	g_signal_connect(G_OBJECT(view),
					 "should-insert-text",
					 G_CALLBACK(&NoteView::handleInsertText),
					 this);
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
	handleMediaInflate(ret);
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

	//Deflate all resources, updating em-media tags and the database
	handleMediaDeflate(ret);
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

void NoteView::handleMediaInflate(string& ret)
{
	//Replace <en-media> tags with their html equivalents
	doTagReplace(ret, "en-media", &NoteView::getInflateReplacementTag);
}

void NoteView::handleMediaDeflate(string& ret)
{
	//Remove all <img> tags first, replacing them with en-media
	doTagReplace(ret, "img", &NoteView::getDeflateReplacementTag);

	//Replace all <a> tags with file:// href's
	doTagReplace(ret, "a", &NoteView::getDeflateReplacementTag);

	//Remove orphaned resources
	removeOrphans();

	//Legitimize update map
	resLocs = updateRes;

	//Now clear away updates
	updateRes.clear();
}

void NoteView::doTagReplace(string& ret, const string& tag, string (NoteView::*getReplacement)(string))
{
	//Replace <en-media> tags with their html equivalents
	size_t lastLoc = 0;

	while ((lastLoc = ret.find("<" + tag, lastLoc)) != string::npos)
	{
		//Find the close location of this tag
		size_t endLoc = ret.find(">", lastLoc);

		//This tag is not closed. Find the matching close
		if (ret[endLoc - 1] != '/')
		{
			int level = 0;
			
			for (size_t i = endLoc; i < ret.length() - (tag.length() + 3); i++)
			{
				//Tag change. update level
				if (ret[i] == '<')
				{
					if (ret[i + 1] == '/' && ret.substr(i + 2, tag.length()) == tag)
						level--;
					else if (ret.substr(i + 1, tag.length()) == tag)
						level++;
				}

				//We've found the end when level is < 0
				if (level < 0)
				{
					endLoc = ret.find(">", i);
					break;
				}
			}
		}

		//Needs to include ending tag, so add 1 to length
		size_t len = endLoc - lastLoc + 1;

		//Now carry out tag replacement
		ret.replace(lastLoc, len, (this->*getReplacement)(ret.substr(lastLoc, len)));

		//Add one to lastLoc to prevent tag refinds in absence of replace
		lastLoc++;
	}
}

string NoteView::getInflateReplacementTag(string mediaTag)
{
	string media = "en-media";

	size_t eLoc;
	
	//Remove the closing tag or /
	if ((eLoc = mediaTag.rfind("</" + media)) != string::npos)
		mediaTag.erase(eLoc);
	else if ((eLoc = mediaTag.rfind("/")) != string::npos)
		mediaTag.erase(eLoc, 1);
	
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

	//Handle image
	if (mime.find("image") != string::npos)
	{
		//Carry out replacement of media tag with img
		mediaTag.replace(mediaTag.find(media), media.length(), "img");

		//Strip the current width and height (if exists) and insert widget width
		if (mediaTag.find("width") != string::npos)
			getProperty(mediaTag, "width", true);
		if (mediaTag.find("height") != string::npos)
			getProperty(mediaTag, "height", true);

		ostringstream out;
		out << "width=\"" << gtk_widget_get_allocated_width(GTK_WIDGET(view)) << "\" ";

		mediaTag.insert(mediaTag.find(">"), out.str());
		
		string srcInfo = "src=\"" + FILE_SRC + res.fileLoc + "\" /";
		
		//Now add in the src tag at the end
		mediaTag.insert(mediaTag.find(">"), srcInfo);
	}
	//Currently, handle all other resources by just putting an anchor to them
	else
	{
		mediaTag.replace(mediaTag.find(media), media.length(), "a");

		//Plant a link with the 
		string hrefInfo = "href=\"" + FILE_SRC + res.fileLoc + "\">" + res.resName + "</a>";

		//Now insert the href at the end of the tag
		mediaTag.erase(mediaTag.find(">"));
		mediaTag = mediaTag + hrefInfo;
	}
	
	return mediaTag;
}

string NoteView::getDeflateReplacementTag(std::string mediaTag)
{
	bool isAnchor = mediaTag.find("<a") != string::npos;
	bool isLocal = mediaTag.find(FILE_SRC) != string::npos;

	//Don't touch the hyperlinks
	if (isAnchor && !isLocal)
		return mediaTag;

	string source = getProperty(mediaTag, (isAnchor ? "href" : "src"), true).substr(FILE_SRC.length());

	//Read in source file
	string fileData;
	Util::readFile(source, fileData);

	//Now get the hash
	string hash = Util::getBinaryChecksum(fileData);
	string mimeType = Util::getMimeType(source, fileData);

	string mediaInfo = " hash=\"" + Util::binaryToHexString(hash) + "\" type=\"" + mimeType + "\" /";
	
	//Insert media tag completion into mediaTag
	mediaTag.insert(mediaTag.find(">"), mediaInfo);

	//Erase superfluous
	mediaTag.erase(mediaTag.find(">") + 1);

	string name = source.substr(source.rfind("/") + 1);

	//Replace tag with en-media
	if (isAnchor)
		mediaTag.replace(mediaTag.find("a"), 1, "en-media");
	else
		mediaTag.replace(mediaTag.find("img"), 3, "en-media");

	//insert into database if new, otherwise just get info from reslocs
	std::map<string, ResInfo>::iterator it = resLocs.find(hash);

	//In old map, and thus in database. Just move info to new map and remove from old
	if (it != resLocs.end())
	{
		ResInfo curInfo = it->second;

		updateRes[hash] = curInfo;

		resLocs.erase(it);

		//TODO remove
		cout << "Held reference to resource at " << curInfo.fileLoc << endl;
	}
	//Not in old map or database, insert
	else
	{
		Resource r;
		db->prepareResource(r, true);

		//Resource data
		r.guid = Util::genGuid();
		r.noteGuid = note.guid;
		r.data.bodyHash = hash;
		r.data.size = fileData.length();
		r.data.body = fileData;
		r.mime = mimeType;
		r.attributes.fileName = name;

		//No USN
		r.__isset.updateSequenceNum = false;
		r.updateSequenceNum = -1;

		//Add to database and flag dirty
		db->addResource(r);
		db->flagDirty(r);

		//Create ResInfo and place in updateRes
		ResInfo info;
		info.fileLoc = source;
		info.resGuid = r.guid;
		info.resName = r.attributes.fileName;

		updateRes[hash] = info;

		//TODO remove
		cout << "Created reference to resource at " << info.fileLoc << endl;
	}

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

	Util::writeFile(fileName, res.data.body);

	return fileName;
}

void NoteView::removeOrphans()
{
	//Remove all resources still in the resLocs map from both the database and existence
	//But only delete if in resource folder.
	for (std::map<string, ResInfo>::iterator it = resLocs.begin(); it != resLocs.end(); it++)
	{
		//Only remove if file path is not in the map
		bool found = false;

		for (std::map<string, ResInfo>::iterator mIt = updateRes.begin(); mIt != updateRes.end(); mIt++)
		{
			if (mIt->second.fileLoc == it->second.fileLoc)
			{
				found = true;
				break;
			}
		}
		
		bool didDelete;

		//Only delete if no other resource references the file
		if (!found)
			didDelete = deleteIfInRes(it->second.fileLoc);
		else
			didDelete = false;

		//Remove from database
		Resource r;
		r.guid = it->second.resGuid;

		db->removeResource(r);

		//TODO remove
		cout << (didDelete ? "Deleted resource at " : "Failed to delete resource at ") << it->second.fileLoc << endl;
	}
}

void NoteView::finalizeResources()
{
	//Iterate over resource map, deleting but not removing from db
	for (std::map<string, ResInfo>::iterator it = resLocs.begin(); it != resLocs.end(); it++)
	{
		bool didDelete = deleteIfInRes(it->second.fileLoc);

		//TODO remove
		cout << (didDelete ? "Deflated " : "Failed to deflate ") << "resource at " << it->second.fileLoc << endl;
	}
}

bool NoteView::deleteIfInRes(const string& fileLoc)
{
	//Not a res file, don't delete
	if (fileLoc.find(baseUrl + "/" + RES_DIR) == string::npos)
		return false;

	//Otherwise, delete
	if (std::remove(fileLoc.c_str()) == 0)
		return true;
	return false;
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

string NoteView::getFileTag(string inputUri)
{
	//Format path
	size_t ll;
	while ((ll = inputUri.find("%20")) != string::npos)
		inputUri.replace(ll, 3, " ");
	inputUri = inputUri.substr(0, inputUri.find_last_not_of(" \t\r\n") + 1);
	
	size_t srcLoc = inputUri.find(FILE_SRC);
	bool isUriFormat = srcLoc != string::npos;

	//If in URI format, remove file://, otherwise leave as is
	string filePath = isUriFormat ? inputUri.substr(srcLoc + FILE_SRC.length()) : inputUri;
	//Append proper opener if not in uri format
	string fileUri = isUriFormat ? inputUri : "file://" + inputUri;
	
	string fileData;
	Util::readFile(filePath, fileData);
	string mime = Util::getMimeType(filePath, fileData);

	ostringstream tag;
	
	//Link this file in an image tag
	if (mime.find("image") != string::npos)
	{
		tag << "<img src=\"" << fileUri << "\" width=\"" << gtk_widget_get_allocated_width(GTK_WIDGET(view)) << "\" />";
	}
	//Else link it in an anchor
	else
	{
		tag << "<a href=\"" << fileUri << "\">" << filePath.substr(filePath.rfind("/") + 1) << "</a>";
	}

	return tag.str();
}

WebKitDOMNode *NoteView::getFileDomTag(string inputUri)
{
	//Format path
	size_t ll;
	while ((ll = inputUri.find("%20")) != string::npos)
		inputUri.replace(ll, 3, " ");
	inputUri = inputUri.substr(0, inputUri.find_last_not_of(" \t\r\n") + 1);
	
	size_t srcLoc = inputUri.find(FILE_SRC);
	bool isUriFormat = srcLoc != string::npos;

	//If in URI format, remove file://, otherwise leave as is
	string filePath = isUriFormat ? inputUri.substr(srcLoc + FILE_SRC.length()) : inputUri;
	//Append proper opener if not in uri format
	string fileUri = isUriFormat ? inputUri : "file://" + inputUri;
	
	string fileData;
	Util::readFile(filePath, fileData);
	string mime = Util::getMimeType(filePath, fileData);

	WebKitDOMDocument *doc = webkit_web_view_get_dom_document(view);
	WebKitDOMNode *node;

	if (mime.find("image") != string::npos)
	{
		WebKitDOMHTMLImageElement *image = (WebKitDOMHTMLImageElement *)webkit_dom_document_create_element(doc, "img", NULL);

		//Set image source
		webkit_dom_html_image_element_set_src(image, fileUri.c_str());
		webkit_dom_html_image_element_set_width(image, gtk_widget_get_allocated_width(GTK_WIDGET(view)));

		node = (WebKitDOMNode *)image;
	}
	else
	{
		WebKitDOMHTMLAnchorElement *anchor = (WebKitDOMHTMLAnchorElement *)webkit_dom_document_create_element(doc, "a", NULL);

		webkit_dom_html_anchor_element_set_href(anchor, fileUri.c_str());

		string fileName = filePath.substr(filePath.rfind("/") + 1);
		
		//Text child for this node
		WebKitDOMText *text = (WebKitDOMText *)webkit_dom_document_create_text_node(doc, fileName.c_str());

		//Insert the file name as a child of the node
		webkit_dom_node_append_child((WebKitDOMNode*)anchor, (WebKitDOMNode*)text, NULL);
		
		node = (WebKitDOMNode *)anchor;
	}

	return node;
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

void NoteView::handleInsert(WebKitWebView *view, WebKitDOMNode *node, WebKitDOMRange *range, WebKitInsertAction action, gpointer data)
{
	gchar *str = webkit_dom_node_get_text_content(node);
	string s(str);
	g_free(str);

	//Pasting a local file URI or referece to a valid file
	if (s.find(FILE_SRC) != string::npos || Util::isValidFile(s))
	{		
		//Empty out the text content and insert a custom node instead
		webkit_dom_node_set_text_content(node, "", NULL);

		//Insert dom tag
		webkit_dom_range_insert_node(range, ((NoteView*)data)->getFileDomTag(s), NULL);
	}
}

void NoteView::handleInsertText(WebKitWebView *view, gchar *mStr, WebKitDOMRange *range, WebKitInsertAction action, gpointer data)
{
	//Immediately return if typed, not pasted. Just use default handler.
	if (action == WEBKIT_INSERT_ACTION_TYPED)
		return;

	//Create a string object off of the gchar
	string s(mStr);

	//Now do the same thing as above:
	if (s.find(FILE_SRC) != string::npos || (s[0] == '/' && s.length() > 1 && Util::isValidFile(s)))
	{
		//Insert a text node containing a new line before the file path.
		//This is necessary to make this bug look more like a feature.
		//Justification: I can't remove the default handler, or I'll have to
		//manually handle all text insertion operations. I also can't alter
		//the data to render the default handler inert, as I did with "handleInsert".
		//This is because the default handler is set to come before any custom handlers.
		//Basically, I'm out of luck, so this works for now.
		webkit_dom_range_insert_node(range, (WebKitDOMNode*)webkit_dom_document_create_element(webkit_web_view_get_dom_document(view), "br", NULL), NULL);

		//Insert dom tag
		webkit_dom_range_insert_node(range, ((NoteView*)data)->getFileDomTag(s), NULL);
	}
}
