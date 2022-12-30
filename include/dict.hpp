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

void throw_invalid_kind(std::string const& expected, json::kind actual) {
    std::ostringstream oss;
    auto kind_str = json::to_string(actual);
    oss << "Invalid result value [expected:" << expected << " was:" << kind_str << "]";
    throw std::runtime_error(oss.str());
}

class sense {
private:
    json::object m_sense;

public:
    sense (json::value const& sense) {
        m_sense = sense.as_object();
    }
};

class def {
private:
    json::object const& m_def;
    std::vector<sense> m_senses;

public:
    def (json::value const& def):
        m_def(def.as_object())
    {
        auto sseq = m_def.at("sseq").as_array();
        for (auto& s1 : sseq) {
            for (auto& s2 : s1.as_array()) {
                auto& obj = s2.as_array();
                if (obj.at(0) != "sense") return;
                m_senses.emplace_back(obj.at(1));
            }
        }
    }

    auto& senses () const {
        return m_senses;
    }
};

class entry {
private:
    std::optional<std::vector<def>> m_def;

public:
    entry (json::value const& entry) {
        if (!entry.is_object()) {
            throw_invalid_kind("object", entry.kind());
        }

        json::object const& entry_obj = entry.get_object();
        if (!entry_obj.contains("def")) return;

        if (!entry_obj.at("def").is_array()) {
            throw_invalid_kind("array", entry_obj.at("def").kind());
        }

        json::array const& defs = entry_obj.at("def").get_array();

    }


    auto& definitions () const {
        return m_def;
    }

};

class suggestions : public std::exception, public std::vector<std::string> {
public:
    suggestions (json::value const& suggestions) {
        for (auto& s : suggestions.get_array())
            emplace_back(s.get_string());
    }
};

class result {
private:
    std::vector<entry> m_entries;

public:
    result (json::value const& result) {
        if (!result.is_array()) {
            throw_invalid_kind("array", result.kind());
        }

        json::array const& entries = result.get_array();

        if (entries.empty()) return;

        if (entries[0].is_object()) {
            for (auto& e : entries)
                m_entries.emplace_back(e);
        } else if (entries[0].is_string()){
            throw suggestions(entries);
        }
    }

    auto& entries () const {
        return m_entries;
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

        json::value json = res.body();

        // return the result

        return result(json);
    }
};

} // namespace dict

#endif // DICT_HPP
