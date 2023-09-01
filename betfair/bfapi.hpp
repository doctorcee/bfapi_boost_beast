//==============================================================================
//
// C++ methods for interaction with Betfair API using boost beast 
//
// Details on the Betfair API certificate login process can be found here:
//
// https://docs.developer.betfair.com/display/1smk3cen4v3lu3yomq5qye0ni/Non-Interactive+%28bot%29+login
//
// For methods in this API to work you will need an active Betfair account that 
// has approved API access. You will also need to have created certificate and key 
// files as described in the above Betfair documentation link.
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
// The path to this file must be supplied to some methods in this API
//==============================================================================
#ifndef BFAPI_HPP
#define BFAPI_HPP

#include <string>
#include <vector>
#include "orders.hpp"

namespace bfapi {
    
    const std::string bf_host = "api.betfair.com";                                                     // Host          
    const std::string place_orders_endpoint = "/exchange/betting/rest/v1.0/placeOrders/";              // endpoint for placeOrders
    const std::string list_event_types_endpoint = "/exchange/betting/rest/v1.0/listEventTypes/";       // endpoint for listEventTypes
    
    
    const std::string port = "443";   // HTTPS port    
    const int http_version = 11;      // HTTP 1.1
    
    struct accinfo {
        std::string username;
        std::string password;
        std::string appkey;
        std::string cert_path;
        std::string key_path;
    };
    
    
    bool extract_user_credentials(const std::string& filename, bfapi::accinfo& ainfo);
    bool login(const bfapi::accinfo& user_info,
               std::string& session_token, 
               std::string& error);                  
                                           
    bool placeOrders(const bfapi::accinfo& user_info,
                     const std::string& session_token,
                     std::string& bf_status,
                     std::string& error,
                     const bfapi::orders::place_limit_orders_request& request);
    
} // end of namespace bfapi

#endif
