//==============================================================================
//
// Same as place_bet.cpp but we place the bet via bfapi::placeOrders() method
//
//==============================================================================
#include "../betfair/bfapi.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <set>
  
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;   

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
    
    // Superbowl winner 23/24 market ID = 1.209995594
    // Carolina Panthers selection ID = 50198
    // Place specifically set orders at low stakes
    // Place bets
    std::set<double> odds = {1.01,1.02,1.03,1.04};
        
    int count = 0;
    const std::string market_id = "1.209995594";        // Superbowl Winner 2023/24 Season
    const std::int64_t selection_id = 50198;            // Carolina Panthers
    const double stake = 1.0;                           // Minimum stake £1
    std::vector<bfapi::orders::limit_order_instruction> order_list;
    for (const double& price : odds)
    {
        ++count;        
        const std::string co_ref = "TEST_CO_REF_" + std::to_string(count);
        bfapi::orders::limit_order_instruction bet(selection_id, true, stake, price, false, co_ref);
        order_list.emplace_back(bet);
    }                        
    
    bfapi::orders::place_limit_orders_request request(market_id,true,order_list);        
    std::string bf_status = "";
    std::string error = "";
    
    auto t1 = high_resolution_clock::now();  
    bool success = bfapi::placeOrders(user_info, session_token, bf_status, error, request);
    auto t2 = high_resolution_clock::now();  
    duration<double, std::milli> ms_double = t2 - t1;            
    std::cout << "bfapi::placeOrders() returned after " << ms_double.count() << "ms\n";
    
    std::cout << "placeOrders returned " << std::to_string(success) << ", BF status = " << bf_status << ", errorCode = " << error << std::endl;
                    
    return EXIT_SUCCESS;
}
