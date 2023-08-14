//==============================================================================
//
// Login to the Betfair API. Built upon from the Boost Beast example code 
// for synchronous HTTPS clients:
//
// https://www.boost.org/doc/libs/develop/libs/beast/example/http/client/sync-ssl/http_client_sync_ssl.cpp
//
// Details on the Betfair API certificate login process can be found here:
//
// https://docs.developer.betfair.com/display/1smk3cen4v3lu3yomq5qye0ni/Non-Interactive+%28bot%29+login
//
// For this code to work you will need to replace the variables us,pw,ak with your account
// username and password and application key respectively. You will also need to 
// change cert_path and key_path to point to the location of your certificate and key files
// generated according to the above Betfair documentation link.
//
//==============================================================================
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

namespace beast = boost::beast; 
namespace http = beast::http;   
namespace net = boost::asio;    
namespace ssl = net::ssl;       
using tcp = net::ip::tcp;      

int main(int argc, char** argv)
{
    try
    {                
        const std::string host = "identitysso-cert.betfair.com";	// Host
        const std::string port = "443";								// HTTPS port
        const std::string target = "/api/certlogin";				// target endpoint
        const int version = 11;										// HTTP 1.1
                
        const std::string un = "username";							// Betfair account username
        const std::string pw = "password";							// Betfair account password
        const std::string ak = "application_key";					// Betfair account API application key
        const std::string cert_path = "path_to_certificate_file";	// Path to certificate file
        const std::string key_path = "path_to_key_file";			// Path to key file                      

        // The io_context is required for all I/O
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx(ssl::context::tlsv12_client);

        // Set certificate and key files generated for the account according to Betfair documentation 
        ctx.use_certificate_file(cert_path, boost::asio::ssl::context::pem);
        ctx.use_rsa_private_key_file(key_path, boost::asio::ssl::context::pem);

        // Don't verify server certificate. Probably need to check this 
        ctx.set_verify_mode(ssl::verify_none);

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(stream.native_handle(), host.c_str()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream).connect(results);

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);   
                
        // Create HTTP login request
        http::request<http::string_body> req{http::verb::post, target, version};		        
        req.set(http::field::host, host);        
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        
        // Require a custom header field "X-Application" used to send our application key        
        req.set("X-Application",ak);
        req.set(http::field::content_type, "application/x-www-form-urlencoded");
        req.body() = "username=" + un + "&password=" + pw;
        req.prepare_payload();
                     
        std::cout << req.base() << std::endl; // Print headers to console for verification

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the message to standard out
        std::cout << res << std::endl;

        // Gracefully close the stream
        beast::error_code ec;
        stream.shutdown(ec);
        if(ec == net::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec = {};
        }
        if(ec)
            throw beast::system_error{ec};

        // If we get here then the connection is closed gracefully       
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
