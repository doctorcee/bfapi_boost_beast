//==============================================================================
// Login test example to obtain a session token using bfapi methods
//==============================================================================
#include "betfair/bfapi.hpp"
#include <iostream>

int main(int argc, char** argv)
{    
    if (argc != 2)
    {
		std::cerr << "Invalid parameters (must supply path to config file)" << std::endl;
        return EXIT_FAILURE;
	}

	// Attempt to extract user info from supplied config file			
	bfapi::accinfo user_info;
	if (bfapi::extract_user_credentials(argv[1], user_info))
	{
		std::string session_token = "";
		std::string error = "";			
		if (bfapi::login(user_info,session_token,error))
		{				
			std::cerr << "Login successful." << std::endl;
		}
		else
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
    return EXIT_SUCCESS;
}
