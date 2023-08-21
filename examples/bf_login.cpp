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
// For this code to work you will need an active Betfair account that has API access and you will need to 
// have created certificate and key files as described in the above Betfair documentation.
// 
// User credentials will be imported from a config file that needs to be created according to the 
// following format:
//
//         [General]
//         user=your_betfair_account_username
//         pw=your_betfair_account_password
//         ak=your_betfair_account_application_key
//         cert=path_to_certificate_file
//         key=path_to_key_file
//
// The path to this file must be supplied to the program as a command line parameter
//==============================================================================
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

namespace beast = boost::beast; 
namespace http = beast::http;   
namespace net = boost::asio;    
namespace ssl = net::ssl;       
using tcp = net::ip::tcp;      

int main(int argc, char** argv)
{
    // Load config file from user supplied path
    std::string ifname = "";
    if (argc == 2)
    {
        // Check supplied file path exists
        ifname = argv[1];
        if (FILE *file = fopen(ifname.c_str(), "r")) 
        {
            fclose(file);
        }
        else 
        {
            std::cerr << "Input file \"" << ifname << "\" could not be opened." << std::endl;
            return EXIT_FAILURE;
        }
    }     
    else
    {
        std::cerr << "Invalid parameters (must supply path to config file)" << std::endl;
        return EXIT_FAILURE;
    }
    
    const std::string host = "identitysso-cert.betfair.com";    // Host
    const std::string port = "443";                             // HTTPS port
    const std::string target = "/api/certlogin";                // target endpoint
    const int version = 11;                                     // HTTP 1.1

    // Read user credentials from supplied config file(see comments at head of file).    
    boost::property_tree::ptree pt;    
    boost::property_tree::ini_parser::read_ini(ifname, pt);
    
    const std::string un = pt.get<std::string>("General.user");
    const std::string pw = pt.get<std::string>("General.pw");
    const std::string ak = pt.get<std::string>("General.ak");
    const std::string cert_path = pt.get<std::string>("General.cert");
    const std::string key_path = pt.get<std::string>("General.key");
    
    try
    {                                        
        // The io_context is required for all I/O
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx(ssl::context::tlsv12_client);

        // Set certificate and key files generated for the account according to Betfair documentation 
        ctx.use_certificate_file(cert_path, boost::asio::ssl::context::pem);
        ctx.use_rsa_private_key_file(key_path, boost::asio::ssl::context::pem);

        // Verify server certificate 
        ctx.set_verify_mode(ssl::verify_peer);                
        ctx.set_default_verify_paths();

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
        {
            throw beast::system_error{ec};
        }
        // If we get here then the connection is closed gracefully       
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
