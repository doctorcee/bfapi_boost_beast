
## Betfair API interaction using C++ and boost beast libraries

This project will ultimately provide simple examples of how to interact with the Betfair API using C++ and boost/beast libraries.

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

To build the login example on Linux using g++:

```bash
$ g++ bf_login.cpp -o test_bf_login.out -lpthread -lcrypto -lssl
```






