//------------------------------------------------------------------------------
//
// Example: HTTP SSL client, coroutine
//
//------------------------------------------------------------------------------

#include "root_certificates.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl; // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>
using boost::asio::awaitable;
using boost::asio::detached;
using boost::asio::use_awaitable;
using namespace std;

//------------------------------------------------------------------------------

// Report a failure
void fail(beast::error_code ec, char const *what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Performs an HTTP GET and prints the response
awaitable<void> do_session(
    std::string const &host,
    std::string const &port,
    std::string const &target,
    int version,
    net::io_context &ioc,
    ssl::context &ctx)
{
    try
    {
        beast::error_code ec;

        cout << "init"
             << "\thost = " << host
             << endl;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            ec.assign(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category());
            std::cerr << ec.message() << "\n";
            co_return;
        }

        // Look up the domain name
        auto const results = co_await resolver.async_resolve(host, port, use_awaitable);
        if (ec)
            co_return fail(ec, "resolve");

        // Set the timeout.
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        co_await get_lowest_layer(stream).async_connect(results, net::use_awaitable);
        if (ec)
            co_return fail(ec, "connect");

        // Set the timeout.
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

        // Perform the SSL handshake
        co_await stream.async_handshake(ssl::stream_base::client, net::use_awaitable);
        if (ec)
            co_return fail(ec, "handshake");

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Set the timeout.
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        co_await http::async_write(stream, req, use_awaitable);
        if (ec)
            co_return fail(ec, "write");

        // This buffer is used for reading and must be persisted
        beast::flat_buffer b;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        co_await http::async_read(stream, b, res, use_awaitable);
        if (ec)
            co_return fail(ec, "read");

        // Write the message to standard out
        std::cout << res << std::endl;

        // Set the timeout.
        beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

        // Gracefully close the stream
        beast::get_lowest_layer(stream).cancel();

        co_await stream.async_shutdown(use_awaitable);
        //stream.shutdown(ec);

        if (ec == net::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec = {};
        }
        //if (ec)
        //    co_return fail(ec, "shutdown");

        // If we get here then the connection is closed gracefully
    }
    catch (const std::exception &e)
    {
        std::cerr << "catch an exception in fun do_session, " << e.what() << ", type is " << typeid(e).name() << '\n';
    }
}

//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
    // Check command line arguments.
    if (argc != 4 && argc != 5)
    {
        std::cerr << "Usage: http-client-coro-ssl <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n"
                  << "Example:\n"
                  << "    http-client-coro-ssl www.example.com 443 /\n"
                  << "    http-client-coro-ssl www.example.com 443 / 1.0\n";
        return EXIT_FAILURE;
    }
    string host = argv[1];
    string port = argv[2];
    string target = argv[3];
    int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

    try
    {
        boost::asio::io_context io_context(1);
        // The SSL context is required, and holds certificates
        ssl::context ctx{ssl::context::tlsv12_client};

        // This holds the root certificate used for verification
        load_root_certificates(ctx);

        // Verify the remote server's certificate
        ctx.set_verify_mode(ssl::verify_none);

        // boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        // signals.async_wait([&](auto, auto) { io_context.stop(); });

        // Launch the asynchronous operation
        co_spawn(io_context,
                 [=, &io_context, &ctx]() mutable {
                     return do_session(host, port, target, version, std::ref(io_context), std::ref(ctx));
                 },
                 detached);

        io_context.run();
    }
    catch (std::exception &e)
    {
        std::printf("Exception: %s\n", e.what());
    }

    return EXIT_SUCCESS;
}