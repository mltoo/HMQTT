#include "Publisher.h"
#include "external.pb.h"
#include "constants.h"
#include <iostream>

using namespace HMQTT;

Publisher::Publisher(const string& topic, const sockaddr_in* serverAddr, const sockaddr_in* localAddr) {
    this->topic = topic;
    this->serverIp = *serverAddr;
    this->localIp = *localAddr;
    
    setupSocket();
    sockaddr_in sockname;
    socklen_t socklen = sizeof(sockname);
    if(getsockname(fdSocket, (sockaddr*)&sockname, &socklen) < 0) {
        perror("Could not get socket name");
        exit(1);
    }
    this->localIp.sin_port = sockname.sin_port; //copy across ephemeral port
}

void Publisher::setupSocket() {
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

void Publisher::start() {
    char buffer[BUFFERLENGTH];
    EXTERNALMESSAGE wholeMessage;
    PUBCONNECT* conMessage = wholeMessage.mutable_pubconnect();
    conMessage->set_topic(topic);
    conMessage->set_ip(localIp.sin_addr.s_addr);
    conMessage->set_port(localIp.sin_port);
    wholeMessage.SerializeToArray(buffer, BUFFERLENGTH);
    sendto(fdSocket, buffer, BUFFERLENGTH, 0, (sockaddr*)&serverIp, sizeof(sockaddr_in));
    socklen_t connAddrLen = sizeof(sockaddr_in);
    recvfrom(fdSocket, buffer, BUFFERLENGTH, 0, &connectedIp, &connAddrLen);
    wholeMessage.Clear();
    wholeMessage.ParseFromArray(buffer,BUFFERLENGTH);
    if(wholeMessage.has_pubaccept()) {
        printf("Connected! Enter values to publish\n");
        string input;
        while(true) {
            printf(">> ");
            getline(cin, input);
            wholeMessage.Clear();
            PUBLISH* message = wholeMessage.mutable_publish();
            message->set_topic(topic);
            message->set_data(input.c_str());
            wholeMessage.SerializeToArray(buffer,BUFFERLENGTH);
            sendto(fdSocket, buffer, BUFFERLENGTH, 0, &connectedIp, sizeof(sockaddr_in));
            sockaddr_in sender;
            socklen_t senderLength = sizeof(sockaddr_in);
            recvfrom(fdSocket, buffer, BUFFERLENGTH, 0, (sockaddr*)&sender, &senderLength);
            wholeMessage.Clear();
            wholeMessage.ParseFromArray(buffer,BUFFERLENGTH);
            if(wholeMessage.has_puback()) {
                if (wholeMessage.puback().status() == PUBACK::AckStatus::PUBACK_AckStatus_SUCCESS) {
                    printf("Publish Success!\n");
                }
            }
        }
    }
}