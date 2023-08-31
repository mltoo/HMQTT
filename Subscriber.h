#pragma once
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string>

using namespace std;


namespace HMQTT {
    class Subscriber {
        public:
            Subscriber(const string& topic, const sockaddr_in* serverAddr, const sockaddr_in* localAddr);

            void start();   
        private:
            string topic;
            sockaddr_in serverAddr;
            sockaddr_in localAddr;
            int fdSocket;

            void setupSocket(); 
    };
}