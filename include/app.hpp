/**
 * @file app.hpp
 * @brief Dictionary UI
 * @author Giuseppe Roberti <inbox> <at> <roberti> <dot> <dev>
 */
#ifndef APP_HPP
#define APP_HPP

#include "dict.hpp"

#include <boost/log/trivial.hpp>

#include <gtkmm.h>

#include <ostream>
#include <future>
#include <thread>

#include <iomanip>

namespace logging = boost::log;

namespace app {

std::ostream& operator<< (std::ostream& out, dict::sense const& s) {
    Glib::ustring text = s.get_text().c_str();

    auto regexes = {
        Glib::Regex::create("\\{bc\\}"),
        Glib::Regex::create("\\{sx\\|(.+)\\|\\|\\}")
    };

    auto replaces = {
        Glib::UStringView("<b><tt> : </tt></b>"),
        Glib::UStringView("SEE \\1")
    };

    decltype(regexes)::const_iterator regex = regexes.begin();
    decltype(replaces)::const_iterator replace = replaces.begin();

    while (regex != regexes.end() && replace != replaces.end()) {
        BOOST_LOG_TRIVIAL(trace)
                << "Does <" << text << "> "
                << " match /" << (*regex)->get_pattern() << "/? "
                << (*regex)->match_all(text);

        text = (*regex)->replace(text.c_str(), text.length(), 0, *replace);

        BOOST_LOG_TRIVIAL(trace)
                << "After replace: " << text;

        ++regex;
        ++replace;
    }

    out << "<b><tt>"
        << std::setfill(' ') << std::setw(8)
        << (s.get_sn().has_value() ? s.get_sn().value().get().c_str() : "")
        << "</tt></b>"
        << text.c_str() << " "
        << "(" << s.get_type() << ")";

    return out;
}

std::ostream& operator<< (std::ostream& out, dict::entry const& e) {
    for (auto& sense : e.senses())
        out << sense << "\n";
    return out;
}

std::ostream& operator<< (std::ostream& out, dict::result const& r) {
    for (auto& entry : r.entries())
        out << "\n" << entry;

    return out;
}

class ResultView : public Gtk::Label {
private:
    Glib::ustring m_markup;

public:
    ResultView () {
        set_margin_start(42);
        set_margin_end(42);
        set_wrap();
        set_wrap_mode(Pango::WrapMode::WORD);
        get_layout()->set_alignment(Pango::Alignment::RIGHT);
    }

    void set_result (std::shared_ptr<dict::result> result) {
        std::ostringstream oss;
        oss << *result;
        m_markup = oss.str();
        set_markup(m_markup);

        BOOST_LOG_TRIVIAL(trace)
                << "Pango markup is\n"
                << m_markup;

        BOOST_LOG_TRIVIAL(trace)
                << "Shared ptr of result use count: "
                << result.use_count();
    }
};

#if !GTK_CHECK_VERSION(4,8,0)
static const Glib::SignalProxyInfo
Search_signal_activate_info =
{
  "activate",
  (GCallback) &Glib::SignalProxyNormal::slot0_void_callback,
  (GCallback) &Glib::SignalProxyNormal::slot0_void_callback
};
#endif

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

#if !GTK_CHECK_VERSION(4,8,0)
    Glib::SignalProxy<void()> signal_activate()
    {
      return Glib::SignalProxy<void()>
              (this, &Search_signal_activate_info);
    }
#endif

    void set_suggestions (dict::suggestions const& suggestions) {
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
            BOOST_LOG_TRIVIAL(trace) << "Application Started!";
            auto msg_id = m_status.push("Application Started!");
            Glib::signal_timeout().connect_once([this, msg_id]{
                m_status.remove_message(msg_id);
            }, 2500);
        });

        m_search.signal_activate().connect([this]{
            BOOST_LOG_TRIVIAL(trace)
                    << "User entered search term <"
                    << m_search.get_text() << ">";
            define(m_search.get_text());
        });

        m_search.signal_term_selected().connect(
                    [this](const Glib::VariantBase& parameter){
            using namespace Glib;
            BOOST_LOG_TRIVIAL(trace)
                    << "User selected search term <"
                    << m_search.get_text() << ">";
            ustring text =
                    VariantBase::cast_dynamic<Variant<ustring>>(parameter).get();
            m_search.set_text(text);
            define(text);
        });
    }

    void define (Glib::ustring const& term) {
        if (term.empty()) return;

        auto msg_id = m_status.push("Searching " + term + " ...");

        BOOST_LOG_TRIVIAL(trace)
                << "Starting search for term <" << term << ">";

        std::promise<std::shared_ptr<dict::result>> prm;
        std::future<std::shared_ptr<dict::result>> ftr = prm.get_future();
        std::thread([this, term](std::promise<std::shared_ptr<dict::result>>&& prm){
            BOOST_LOG_TRIVIAL(trace)
                    << "Search for term <" << term << "> started";
            try {
                BOOST_LOG_TRIVIAL(trace)
                        << "Search for term <" << term << "> finished";

                auto result = m_api.request(term);

                BOOST_LOG_TRIVIAL(trace)
                        << "Shared ptr of result use count: "
                        << result.use_count();

                prm.set_value(result);
            } catch (...) {
                BOOST_LOG_TRIVIAL(trace)
                        << "Search for term <" << term << "> throws";
                prm.set_exception(std::current_exception());
            }
        }, std::move(prm)).detach();

        Glib::signal_timeout().connect([this, msg_id, term, ftr = ftr.share()]{
            using namespace std::chrono_literals;

            if (ftr.wait_for(1ms) != std::future_status::ready)
                return true;

            try {
                auto result = ftr.get();

                BOOST_LOG_TRIVIAL(trace)
                        << "Shared ptr of result use count: "
                        << result.use_count();
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
        BOOST_LOG_TRIVIAL(trace) << "Application Created!";
    }

protected:
    bool shutdown () {
        BOOST_LOG_TRIVIAL(trace) << "Application Shutdown!";
        Gtk::Application::get_default()->quit();
        return true;
    }
};

} // namespace app

#endif // APP_HPP
