#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "constants.h"
#include "external.pb.h"
#include <unordered_map>
#include <vector>
#include "Subscription.h"

namespace HMQTT{
    class Server {
        public:
            Server(const std::string& topic, const sockaddr_in* localAddr);
            
            void join(const sockaddr_in* parentAddr);
            void startServer(); 
        private:
            std::string topic;
			int fdSocket;
            int fdServerThreadWakeEvent;
            bool closing = false;
            sockaddr_in localAddr;

			void setupSocket(const sockaddr_in* localAddr);
            void processPubConnect(const PUBCONNECT& message, EXTERNALMESSAGE& wholeMessage);
            void processPublish(const PUBLISH& message, sockaddr* sender, EXTERNALMESSAGE& wholeMessage);
            void processSubscribe(const SUBSCRIBE& message, EXTERNALMESSAGE& wholeMessage);
            void processJoin(const JOIN& message, sockaddr* sender, EXTERNALMESSAGE& wholeMessage);

            std::unordered_map<std::string, sockaddr> delegations;
            std::vector<Subscription> subscriptions;
    };
}
