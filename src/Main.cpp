/*
 *Connor Cormier
 *5/25/14
 *Main file for Betternote
 */

#include <gtkmm/application.h>
#include <iostream>
#include "NoteWindow.h"

int main(int argc, char **argv)
{
	Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv);

	//Set up a builder instance
	Glib::RefPtr<Gtk::Builder> builder = Gtk::Builder::create();

	try
	{
		//Try to load UI file
		builder->add_from_file(UI_FILE);
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Something broke: " << ex.what() << std::endl;
		return 1;
	}

	//Pointer to hold the window
	NoteWindow *win = NULL;
	builder->get_widget_derived("NoteWindow", win);

	if (!win)
	{
		std::cerr << "Failed to get window" << std::endl;
		return 2;
	}

	//Open window and return upon completion
	return app->run(*win);
}
