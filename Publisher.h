#pragma once

#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


using namespace std;


namespace HMQTT {
    class Publisher {
        public:
            Publisher(const string& topic, const sockaddr_in* serverAddr, const sockaddr_in* localAddr);

            void start();
        private:
            string topic;
            int fdSocket;
            sockaddr_in localIp;
            sockaddr_in serverIp; //ip of the root server
            sockaddr connectedIp; //ip of the server hosting the desired topic

            void setupSocket();
    };
}