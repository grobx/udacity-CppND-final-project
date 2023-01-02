/**
 * @file main.cpp
 * @brief Dictionary main entry
 * @author Giuseppe Roberti <inbox> <at> <roberti> <dot> <dev>
 */
#include "app.hpp"

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>

namespace logging = boost::log;

int main (int argc, char* argv[])
{
    logging::core::get()->set_filter
    (
        logging::trivial::severity >=
                logging::trivial::severity_level::error
    );
    const Glib::ustring app_id =
            "dev.roberti.udacity.cppnd.final-project";
    const Glib::ustring title = "Dictionary";
    const int width = 800, height = 800;
    auto app = Gtk::Application::create(app_id);
    return app->make_window_and_run<app::Window>
            (argc, argv, title, width, height);
}
