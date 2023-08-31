#include "Subscriber.h"
#include "external.pb.h"
#include "constants.h"

using namespace HMQTT;

Subscriber::Subscriber(const string& topic, const sockaddr_in* serverAddr, const sockaddr_in* localAddr) {
    this->localAddr = *localAddr;
    this->serverAddr = *serverAddr;
    this->topic = topic;

    setupSocket();
    sockaddr_in sockname;
    socklen_t socklen = sizeof(sockname);
    if(getsockname(fdSocket, (sockaddr*)&sockname, &socklen) < 0) {
        perror("Could not get socket name");
        exit(1);
    }
    this->localAddr.sin_port = sockname.sin_port; //copy across ephemeral port
}

void Subscriber::setupSocket() {
    fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(fdSocket < 0) {
        perror("Could not initialise socket");
        exit(1);
    }

    uint64_t yes = 1;
    if(setsockopt(fdSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("Error setting SO_REUSEADDR");
        exit(1);
    }

    sockaddr_in anyAddr = {0};
    anyAddr.sin_addr.s_addr = INADDR_ANY;
    anyAddr.sin_family = AF_INET;
    if(bind(fdSocket, (sockaddr*)&anyAddr, sizeof(sockaddr_in)) < 0) {
        perror("Could not bind socket");
        exit(1);
    }

}

void Subscriber::start() {
    char buffer[BUFFERLENGTH];
    EXTERNALMESSAGE wholeMessage;
    SUBSCRIBE* subMessage = wholeMessage.mutable_subscribe();
    subMessage->set_ip(localAddr.sin_addr.s_addr);
    subMessage->set_port(localAddr.sin_port);
    subMessage->set_topic(topic);
    wholeMessage.SerializeToArray(buffer, BUFFERLENGTH);
    sendto(fdSocket, buffer, BUFFERLENGTH, 0, (sockaddr*)&serverAddr, sizeof(sockaddr_in));
    while(true) {
        sockaddr_in sender;
        socklen_t senderLength = sizeof(sender);
        recvfrom(fdSocket, buffer, BUFFERLENGTH, 0, (sockaddr*)&sender, &senderLength);
        wholeMessage.Clear();
        wholeMessage.ParseFromArray(buffer, BUFFERLENGTH);
        if(wholeMessage.has_publish()) {
            const PUBLISH& pubMessage = wholeMessage.publish();
            printf("PUBLISH %s:\n%s\n", pubMessage.topic().c_str(), pubMessage.data().c_str());
        }
    }
}
