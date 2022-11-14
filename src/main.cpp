/**
 * @file main.cpp
 * @brief Dictionary main entry
 * @author Giuseppe Roberti <inbox> <at> <roberti> <dot> <dev>
 */
#include "app.hpp"

int main (int argc, char* argv[])
{
    const Glib::ustring app_id =
            "dev.roberti.udacity.cppnd.final-project";
    const Glib::ustring title = "Dictionary";
    const int width = 400, height = 800;
    auto app = Gtk::Application::create(app_id);
    return app->make_window_and_run<app::Window>
            (argc, argv, title, width, height);
}
