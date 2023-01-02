/**
 * @file dict.hpp
 * @brief Dictionary HTTP client for Merriam-Webster https://www.dictionaryapi.com/
 * @author Giuseppe Roberti <inbox> <at> <roberti> <dot> <dev>
 */
#ifndef DICT_HPP
#define DICT_HPP

#include "json_body.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/json.hpp>
#include <boost/log/trivial.hpp>

#include <unordered_map>
#include <algorithm>
#include <thread>
#include <optional>

namespace dict {

namespace beast = boost::beast;
namespace system = boost::system;
namespace http = boost::beast::http;
namespace json = boost::json;
namespace asio = boost::asio;

namespace ssl = asio::ssl;
namespace ip = asio::ip;

using tcp = ip::tcp;

class sense {
public:
    enum class type {
        noun,
        verb,
        sls
    };

private:
    json::object const& m_sense;
    type const m_type;
    std::optional<std::reference_wrapper<const json::string>> m_sn;
    json::string const& m_text;

public:
    sense (json::value const& sense, type const sense_type):
        m_sense(sense.as_object()),
        m_type(sense_type),
        m_sn(find_sn()),
        m_text(find_text())
    {}

    const char* get_type () const {
        switch (m_type) {
        case type::noun:
            return "noun";
        case type::verb:
            return "verb";
        case type::sls:
            return "sls";
        }
    }

    std::optional<std::reference_wrapper<const json::string>> get_sn () const {
        return m_sn;
    }

    json::string const& get_text () const {
        return m_text;
    }

private:
    std::optional<std::reference_wrapper<const json::string>> find_sn () {
        if (m_sense.contains("sn"))
            return m_sense.at("sn").as_string();
        else
            return std::nullopt;
    }

    json::string const& find_text () {
        auto& dt_a = m_sense.at("dt").as_array();
        auto val = std::find_if(dt_a.begin(), dt_a.end(), [](json::value const& v) {
                return v.as_array().at(0).as_string().compare("text") == 0;
        });
        return val->as_array().at(1).as_string();
    }
};

class entry {
private:
    json::object const& m_entry;
    std::vector<sense> m_senses;

public:
    entry (json::value const& entry):
        m_entry(entry.as_object())
    {
        json::array const& defs = m_entry.at("def").as_array();

        for (auto& def : defs)
            parse_def (def.as_object());

        BOOST_LOG_TRIVIAL(trace)
                << "Costructed an entry with "
                << defs.size() << " definitions and "
                << m_senses.size() << " senses";
    }

    auto& senses () const {
        return m_senses;
    }

private:
    void parse_def (json::object const& def) {
        sense::type sense_type;

        if (auto* vd = def.if_contains("vd")) {
            sense_type = sense::type::verb;

            BOOST_LOG_TRIVIAL(trace)
                    << "Found verb divider "
                    << vd->as_string();
        } else if (auto* sls = def.if_contains("sls")) {
            sense_type = sense::type::sls;

            BOOST_LOG_TRIVIAL(trace)
                    << "Found sls:\n"
                    << *sls;
        } else {
            sense_type = sense::type::noun;

            BOOST_LOG_TRIVIAL(trace)
                    << "It should be a noun";
        }

        auto& sseq = def.at("sseq");

        for (auto& senses : sseq.as_array())
            populate_senses (senses, sense_type);
    }

    void populate_senses (json::value const& senses, sense::type sense_type) {
        for (auto& sense : senses.as_array()) {
            auto& s2a = sense.as_array();
            auto& s2a0 = s2a.at(0).as_string();
            auto& s2a1 = s2a.at(1);
            if (s2a0.compare("sense") == 0) {
                m_senses.emplace_back(s2a1, sense_type);
            } else if (s2a0.compare("pseq") == 0) {
                populate_senses (s2a1, sense_type);
            }
        }
    }
};

class result {
private:
    const json::value m_result;
    std::vector<entry> m_entries;

public:
    result (json::value const&& result):
        m_result(std::move(result))
    {
        for (auto& e : m_result.as_array()) {
            try {
                m_entries.emplace_back(e);
            } catch (std::exception& e) {
                BOOST_LOG_TRIVIAL(trace)
                        << "Unable to parse an entry";
            }
        }

        BOOST_LOG_TRIVIAL(trace)
                << "Costructed a result with "
                << m_result.as_array().size()
                << " entries";
    }

    auto& entries () const {
        return m_entries;
    }
};

class suggestions : public std::exception, public std::vector<std::string> {
public:
    suggestions (json::value const& suggestions)
    {
        for (auto& s : suggestions.as_array())
            emplace_back(s.as_string());
    }
};

class api {
private:
    asio::io_context m_io_context;
    ssl::context m_ssl_context;
    const std::string m_host, m_port, m_base_path, m_api_key;

public:
    api (std::string api_key) :
        m_ssl_context(ssl::context::sslv23_client)
      , m_host("www.dictionaryapi.com"), m_port("https")
      , m_base_path("/api/v3/references/collegiate/json")
      , m_api_key(api_key)
    {
        m_ssl_context.set_default_verify_paths();
    }

    auto request (std::string word) {
        // creating client socket

        BOOST_LOG_TRIVIAL(trace)
                << "Request term <" << word << ">";

        ssl::stream<tcp::socket> m_ssl_sock
                (m_io_context, m_ssl_context);
        m_ssl_sock.set_verify_mode(ssl::verify_peer);
        m_ssl_sock.set_verify_callback(ssl::host_name_verification(m_host));

        // resolving host:port

        tcp::resolver m_resolver = tcp::resolver(m_io_context);
        ip::basic_resolver_results<tcp> m_resolver_results =
                m_resolver.resolve(m_host.c_str(), m_port.c_str());

        BOOST_LOG_TRIVIAL(trace)
                << "Host:service resolved to "
                << m_resolver_results->host_name() << ":"
                << m_resolver_results->service_name();

        // connecting

        tcp::endpoint m_endpoint =
                asio::connect(m_ssl_sock.lowest_layer(), m_resolver_results);

        BOOST_LOG_TRIVIAL(trace)
                << "Connected to endpoint "
                << m_endpoint.address() << ":"
                << m_endpoint.port();

        // ssl handshake

        m_ssl_sock.handshake(asio::ssl::stream_base::client);

        BOOST_LOG_TRIVIAL(trace) << "Handshake done!";

        // creating request

        std::string resource =
                m_base_path + "/" + word + "?key=" + m_api_key;
        http::request<json_body> req{http::verb::get, resource, 11};
        req.set(http::field::host, m_host);
        req.set(http::field::user_agent, "Dictionary/0.99");

        // sending request

        std::size_t sent = http::write(m_ssl_sock, req);

        BOOST_LOG_TRIVIAL(trace) << "Wrote " << sent << " bytes";

        // read reply

        beast::flat_buffer buffer;
        http::response<json_body> res{};
        std::size_t read = http::read(m_ssl_sock, buffer, res);

        BOOST_LOG_TRIVIAL(trace) << "Read " << read << " bytes";

        json::value& json = res.body();

        // if the result is an array of strings, throws suggestions

        if (json.as_array().at(0).is_string())
            throw suggestions(json);

        // return the result

        return std::make_unique<result>(std::move(json));
    }
};

} // namespace dict

#endif // DICT_HPP
