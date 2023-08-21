
## Betfair API interaction using C++ and boost beast libraries

This project provides some simple examples of how to communicate with the Betfair API using C++ and boost/beast libraries. 
It is intended ONLY to assist with getting started with C++ for Betfair API interaction and help with creating bespoke applications.

## Requirements
* An active Betfair account with access to the Betfair API
* C++11
* OpenSSL
* Boost
* Pthreads

## License

Distributed under the MIT License. Please take the time to read the details [here](https://github.com/doctorcee/betfair_beast/blob/main/LICENSE) for more 
information and before making use of the project.

## Build instructions

To build the login example in main.cpp on Linux using g++:

```bash
$ g++ examples/bf_login.cpp betfair/bfapi.cpp -o test_login.out -lpthread -lcrypto -lssl
```

## Running the example

Create a config file containing used credentials (according to the instructions in betfair/bfapi.hpp) and pass the path as a command line parameter to 
the executable built above. This should login to your account and return a session token which can then be used to perform other API operations.






