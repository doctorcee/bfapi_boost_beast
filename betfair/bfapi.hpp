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
#include <string>

namespace bfapi {
    
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
    
} // end of namespace bfapi
