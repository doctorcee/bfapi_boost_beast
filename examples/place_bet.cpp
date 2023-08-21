//==============================================================================
// Fire off a sequence of low risk bets on a long term market.
// Specifically lay Carolina Panthers for £2 at 1.01,1.02,1.03,1.04 for 23/24 Superbowl
// This results in a risk of losing 20 pence although the bets are unlikely to ever
// be matched. 
// This is just for test and illustrative purposes.
//==============================================================================
#include "../betfair/bfapi.hpp"
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
#include <chrono>
#include <thread>
#include <set>

namespace beast = boost::beast; 
namespace http = beast::http;   
namespace net = boost::asio;    
namespace ssl = net::ssl;       
using tcp = net::ip::tcp;      

int main(int argc, char** argv)
{   
    if (argc != 2)
    {
        std::cerr << "Invalid parameters (must supply path to config file)" << std::endl;
        return EXIT_FAILURE;
    }

    std::string session_token = "";
    // Attempt to extract user info from supplied config file            
    bfapi::accinfo user_info;
    if (bfapi::extract_user_credentials(argv[1], user_info))
    {       
        std::string error = "";            
        if (false == bfapi::login(user_info,session_token,error))
        {                      
            std::cerr << "Betfair login failed: " << error << std::endl;
            return EXIT_FAILURE;
        }                            
    }
    else
    {
        std::cerr << "Unable to extract user credentials from supplied config filename." << std::endl;
        return EXIT_FAILURE;
    }
    
    // If we get here we logged in successfully                 
    try
    {                                        
        // The io_context is required for all I/O
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx(ssl::context::tlsv12_client);

        // Only need cert and key files on login      

        // Verify server certificate 
        ctx.set_verify_mode(ssl::verify_peer);                
        ctx.set_default_verify_paths();
        
        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(stream.native_handle(), bfapi::bf_host.c_str()))
        {
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }

        // Look up the domain name
        auto const results = resolver.resolve(bfapi::bf_host, bfapi::port);

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream).connect(results);

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);   
        
        // Sleep for 1 second
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));           
                
        // Place bets
        // Superbowl winner 23/24 market ID = 1.209995594
        // Carolina Panthers selection ID = 50198
        // Place specifically set orders (that won't match) at low stakes    
        std::set<std::string> odds = {"1.01","1.02","1.03"};        
        
        using std::chrono::high_resolution_clock;
        using std::chrono::duration_cast;
        using std::chrono::duration;
        using std::chrono::milliseconds;
                      
        bool use_async_placement = true;
        const std::string po_base = "{\"marketId\":\"1.209995594\",\"instructions\":[{\"selectionId\":50198,\"side\":\"LAY\",\"orderType\":\"LIMIT\",\"limitOrder\":{\"size\":2.0,\"price\":";
        for (const std::string& price : odds)
        {
            
            auto t1 = high_resolution_clock::now();        
            
            http::request<http::string_body> req{http::verb::post, bfapi::place_orders_endpoint, bfapi::http_version};                
            req.set(http::field::host, bfapi::bf_host);        
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            
            // Require a custom header field "X-Application" used to send our application key        
            req.set("X-Application",user_info.appkey);
            req.set("X-Authentication",session_token);
            req.set("Connection","keep-alive");
            req.set("accept","application/json");
            req.set(http::field::content_type, "application/json");
                        
            std::string req_body = po_base + price + ",\"persistenceType\":\"LAPSE\"}}]";
            if (use_async_placement)
            {
                req_body += ",\"async\":true";
            }
            req_body += "}";            
            req.body() = req_body;
            req.prepare_payload();                                                    

            // Send the HTTP request to the remote host
            http::write(stream, req);

            // This buffer is used for reading and must be persisted
            beast::flat_buffer buffer;

            // Declare a container to hold the response
            http::response<http::string_body> res;

            // Receive the HTTP response
            http::read(stream, buffer, res);        
            
            // Dump JSON respons to console
            std::cout << res.body() << std::endl;
            
            auto t2 = high_resolution_clock::now();        

            /* Getting number of milliseconds as a double. */
            duration<double, std::milli> ms_double = t2 - t1;    
            
            std::cout << "Bet placement took: " << ms_double.count() << "ms\n";
        }            
        
        // Gracefully close the stream after sleeping for 5 seconds
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));           
        
        beast::error_code ec;
        stream.shutdown(ec);
        
        // Shutdown always seems to result in a "Stream truncated" error. We log this to console while trying to 
        // work out what the solution is
        if(ec == net::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec = {};
        }
        if (ec)
        {
            std::cerr << "Shutdown error: " << ec.message() << std::endl;        
            //throw beast::system_error{ec};
        }        
    }
    catch(std::exception const& e)
    {
        std::cerr << "ERROR: Exception thrown (" << e.what() << ")" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
