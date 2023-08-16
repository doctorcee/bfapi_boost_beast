#include "bfapi.hpp"
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
#include <boost/property_tree/json_parser.hpp>
#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

namespace beast = boost::beast; 
namespace http = beast::http;   
namespace net = boost::asio;    
namespace ssl = net::ssl;       

using tcp = net::ip::tcp;      
using boost::property_tree::ptree;

namespace bfapi {
    
//==============================================================================
bool extract_user_credentials(const std::string& filename, 
                              bfapi::accinfo& ainfo)
{
    if (FILE *file = fopen(filename.c_str(), "r")) 
    {
        fclose(file);
        boost::property_tree::ptree pt;    
        boost::property_tree::ini_parser::read_ini(filename, pt);
    
        ainfo.username  = pt.get<std::string>("General.user");
        ainfo.password  = pt.get<std::string>("General.pw");
        ainfo.appkey    = pt.get<std::string>("General.ak");
        ainfo.cert_path = pt.get<std::string>("General.cert");
        ainfo.key_path  = pt.get<std::string>("General.key");
        
        // TODO: Perhaps create a validate method of bfapi::accinfo to validate 
        // member strings.
        return true;
    }    
    return false;    
}

//==============================================================================
bool login(const bfapi::accinfo& user_info, 
           std::string& session_token, 
           std::string& error)           
{
    bool ok = false;
    
    const std::string host   = "identitysso-cert.betfair.com";    // Host
    const std::string port   = "443";                             // HTTPS port
    const std::string target = "/api/certlogin";                  // target endpoint
    const int version        = 11;                                // HTTP 1.1
    
    // Clear reference parameters
    session_token = "";
    error    = "";
    
    try
    {                                        
        // The io_context is required for all I/O
        net::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx(ssl::context::tlsv12_client);

        // Set certificate and key files generated for the account according to Betfair documentation 
        ctx.use_certificate_file(user_info.cert_path, boost::asio::ssl::context::pem);
        ctx.use_rsa_private_key_file(user_info.key_path, boost::asio::ssl::context::pem);

        // TODO: Do we really need to verify the server certificate ? 
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
        req.set("X-Application",user_info.appkey);
        req.set(http::field::content_type, "application/x-www-form-urlencoded");
        req.body() = "username=" + user_info.username + "&password=" + user_info.password;
        req.prepare_payload();                            

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response - use a string_body for ease of reading the response.
        // For very large responses we may wish to rethink this and use dynamic_body (see below)
        // https://github.com/boostorg/beast/issues/819
        
        //http::response<http::dynamic_body> res;
        http::response<http::string_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);
                        
        std::string bf_login_status = "";
        if (res.result_int() == 200)
        {
            // Extract session token from a successful response (HTTP 200)            
            // Response body is JSON of the form:
            // {
            //        "sessionToken":"abcdefghijklmnop",
            //        "loginStatus":"SUCCESS" (INVALID_USERNAME_OR_PASSWORD , CERT_AUTH_REQUIRED etc. See docs for full list)
            // }
            
            // parse with boost property_tree
            ptree pt;
            std::stringstream ss; 
            ss << res.body();
            read_json(ss, pt);
            
            int tc = pt.count("loginStatus");
            if (tc > 0)
            {
                bf_login_status = pt.get<std::string>("loginStatus");
                if (bf_login_status == "SUCCESS")
                {
                    session_token   = pt.get<std::string>("sessionToken");
                    std::cout << "Reponse \"loginStatus\"=" << bf_login_status << std::endl;
                }
                else
                {
                    error = "bfapi::login() error: Response \"loginStatus\" = " + bf_login_status;
                }
            }
            else
            {
                error = "bfapi::login() error: Response missing \"loginStatus\" field!";
            }
        }
        else
        {            
            // NOTE: must use explicit std::string constructor because res.reason() is actually a boost::string_view)        
            error = "bfapi::login() error: HTTPS response error " + std::to_string(res.result_int()) + " " + std::string(res.reason());
            std::cout << error << std::endl;            
            for (auto& h : res.base()) 
            {
                std::cout << "Field: " << h.name() << "/text: " << h.name_string() << ", Value: " << h.value() << "\n";
            }
            std::cout << res.body() << std::endl;
        }        
                
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
        // If we have assigned a value to the session token, then login was a success
        ok = session_token.size() > 0;
    }
    catch(std::exception const& e)
    {
        error = std::string("bfapi::login() exception occurred: ") + std::string(e.what());
        ok = false;
    }
    return ok;
}                   
    
}  // end of namespace bfapi
