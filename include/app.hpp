/**
 * @file app.hpp
 * @brief Dictionary UI
 * @author Giuseppe Roberti <inbox> <at> <roberti> <dot> <dev>
 */
#ifndef APP_HPP
#define APP_HPP

#include "dict.hpp"

#include <gtkmm.h>

#include <ostream>
#include <future>
#include <thread>

namespace app {

std::ostream& operator<< (std::ostream& out, dict::sense const& s) {
    out << "      sense here";
    return out;
}

std::ostream& operator<< (std::ostream& out, dict::def const& d) {
    out << "    Def:\n";
    for (auto& sense : d.senses())
        out << "    " << sense << "\n";
    return out;
}

std::ostream& operator<< (std::ostream& out, dict::entry const& e) {
    out << "  <i>Entry</i>:" << "\n";
    if (e.definitions()) for (auto& def : *e.definitions())
        out << def << "\n";
    return out;
}

std::ostream& operator<< (std::ostream& out, dict::result const& r) {
    for (auto& entry : r.entries())
        out << "<b><span foreground=\"red\" size=\"large\">Result</span></b>:\n"
            << entry;
    return out;
}

class ResultView : public Gtk::Label {
private:
    Glib::ustring m_markup;

public:
    void set_result (dict::result const& res) {
        std::ostringstream oss;
        oss << res;
        m_markup = oss.str();
        set_markup(m_markup);
    }
};

class Search : public Gtk::SearchEntry {
private:
    Glib::RefPtr<Gio::Menu> m_menu;
    Glib::RefPtr<Gio::SimpleAction> m_action;
    Gtk::PopoverMenu m_popover;

public:
    Search():
        m_menu(Gio::Menu::create())
      , m_action(Gio::SimpleAction::create("define", Glib::VariantType("s")))
    {
        Gtk::Application::get_default()->add_action(m_action);

        m_popover.set_menu_model(m_menu);
        m_popover.set_parent(*this);
    }

    auto signal_term_selected () {
        return m_action->signal_activate();
    }

    auto set_suggestions (const dict::suggestions & suggestions) {
        m_menu->remove_all();
        for (auto & s : suggestions) {
          m_menu->append(s, "app.define::" + s);
        }
        m_popover.popup();
    }
};

class Layout : public Gtk::Box {
private:
    Gtk::Window& m_window;
    dict::api m_api;

    Search m_search;
    Gtk::HeaderBar m_headerbar;

    Gtk::Box m_box;
    Gtk::Stack m_stack;
    Gtk::ScrolledWindow m_scroll;

    Gtk::Statusbar m_status;
    std::unique_ptr<Gtk::MessageDialog> m_error_dialog;

public:
    Layout (Gtk::Window& window) : Box(Gtk::Orientation::VERTICAL)
      , m_window(window)
      , m_api(Glib::getenv("DICTIONARY_API_KEY"))
    {
        /// HEADER WIDGETS

        m_headerbar.set_title_widget(m_search);
        append(m_headerbar);

        /// CENTRAL WIDGETS

        m_stack.set_expand();
        m_scroll.set_child(m_stack);
        m_box.set_orientation(Gtk::Orientation::HORIZONTAL);

        m_box.append(m_scroll);
        append(m_box);

        /// BOTTOM WIDGETS

        append(m_status);

        /// INITIALIZE SIGNALS

        init_signals();
    }

private:
    void init_signals () {
        signal_realize().connect([this]{
            auto msg_id = m_status.push("Application Started!");

            Glib::signal_timeout().connect_once([this, msg_id]{
                m_status.remove_message(msg_id);
            }, 2500);
        });

        m_search.signal_activate().connect([this]{
            define(m_search.get_text());
        });

        m_search.signal_term_selected().connect(
                    [this](const Glib::VariantBase& parameter){
            using namespace Glib;
            ustring text =
                    VariantBase::cast_dynamic<Variant<ustring>>(parameter).get();
            m_search.set_text(text);
            define(text);
        });
    }

    void define (Glib::ustring const& term) {
        if (term.empty()) return;

        auto msg_id = m_status.push("Searching " + term + " ...");

        auto ftr = std::async(std::launch::async,
                              std::bind(&dict::api::request, &m_api, term));

        Glib::signal_timeout().connect([this, msg_id, term, ftr = ftr.share()]{
            if (ftr.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
                return true;

            try {
                auto result = ftr.get();

                auto* widget = m_stack.get_child_by_name("page");
                if (widget != nullptr)
                    m_stack.remove(*widget);

                auto page = m_stack.add(*Gtk::make_managed<ResultView>(), "page");
                auto result_view = static_cast<ResultView*>(page->get_child());
                page->set_title(term);

                result_view->set_result(result);
                m_search.set_text("");
            } catch (dict::suggestions const& suggestions) {
                m_search.set_suggestions(suggestions);
            } catch (std::exception const& e) {
                std::ostringstream oss;
                oss << e.what();
                m_error_dialog.reset(new Gtk::MessageDialog(
                                         m_window,
                                         oss.str(),
                                         true,
                                         Gtk::MessageType::ERROR));
                m_error_dialog->set_modal();
                m_error_dialog->show();
                m_error_dialog->signal_response().connect([this](int response_id){
                    m_search.set_text("");
                    m_error_dialog->hide();
                });
            }

            m_status.remove_message(msg_id);
            return false;
        }, 1000);
    }
};

class Window : public Gtk::ApplicationWindow {
private:
    Layout m_layout;

    const Glib::ustring m_title = "Application Window";
    const int m_width = 800;
    const int m_height = 600;

public:
    Window (Glib::ustring title, int width, int height):
        m_layout(*this)
      , m_title(title), m_width(width), m_height(height)
    {
        set_title(m_title);
        set_default_size(m_width, m_height);
        set_child(m_layout);
        signal_close_request().connect(
                    sigc::mem_fun(*this, &Window::shutdown), false);
    }

protected:
    bool shutdown () {
        Gtk::Application::get_default()->quit();
        return true;
    }
};

}

#endif // APP_HPP
