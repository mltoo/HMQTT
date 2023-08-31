
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "Server.h"
#include "Subscriber.h"
#include "Publisher.h"

using namespace std;
using namespace HMQTT;

int main() {
    string input;
    printf("Select an option:\n");
    printf("1) Start Server\n");
    printf("2) Start Subscriber\n");
    printf("3) Start Publisher\n");
    printf("4) Start Server (default config)\n");
    printf("5) Start Subscriber (default config)\n");
    printf("6) Start Publisher (default config)\n");
    printf(">> ");
    getline(cin, input);
    string topic;
    sockaddr_in addr = {0};
    sockaddr_in clientAddr = {};
    addr.sin_family = AF_INET;
    clientAddr.sin_family = AF_INET;
    Server* server;
    Subscriber* subscriber;
    Publisher* publisher;
    switch (stoi(input)) {
        case 1:
            printf("Enter the topic for the server\n");
            printf(">> ");
            getline(cin,topic);
            printf("Enter the IP for the server\n");
            printf(">> ");
            getline(cin,input);
            inet_pton(AF_INET, input.c_str(), &addr.sin_addr.s_addr);
            printf("Enter the port for the server\n");
            printf(">> ");
            getline(cin,input);
            addr.sin_port = htons(stoi(input));
            server = new Server(topic, &addr);
            server->startServer();
            break;
        case 2:
            printf("Enter the topic to subscribe to\n");
            printf(">> ");
            getline(cin,topic);
            printf("Enter the IP for the server\n");
            printf(">> ");
            getline(cin,input);
            inet_pton(AF_INET, input.c_str(), &addr.sin_addr.s_addr);
            printf("Enter the port for the server\n");
            printf(">> ");
            getline(cin,input);
            addr.sin_port = htons(stoi(input));
            printf("Enter the local IP for the subscriber\n");
            printf(">> ");
            getline(cin,input);
            inet_pton(AF_INET, input.c_str(), &clientAddr.sin_addr.s_addr);
            subscriber = new Subscriber(topic, &addr, &clientAddr);
            subscriber->start();
            break;
        case 3:
            printf("Enter the topic to publish to\n");
            printf(">> ");
            getline(cin,topic);
            printf("Enter the IP for the server\n");
            printf(">> ");
            getline(cin,input);
            inet_pton(AF_INET, input.c_str(), &addr.sin_addr.s_addr);
            printf("Enter the port for the server\n");
            printf(">> ");
            getline(cin,input);
            addr.sin_port = htons(stoi(input));
            printf("Enter the local IP of this publisher\n");
            printf(">> ");
            getline(cin, input);
            inet_pton(AF_INET, input.c_str(), &clientAddr.sin_addr.s_addr);
            publisher = new Publisher(topic, &addr, &clientAddr);
            publisher->start();
            break;
        case 4:
            printf("Enter the topic for the server\n");
            printf(">> ");
            getline(cin,topic);
            inet_pton(AF_INET, "192.168.0.34", &addr.sin_addr.s_addr);
            printf("Enter the port for the server\n");
            printf(">> ");
            getline(cin,input);
            addr.sin_port = htons(stoi(input));

            printf("Join an existing server? (y/N)\n");
            printf(">> ");
            getline(cin,input);
            server = new Server(topic, &addr);
            if(input.compare("y") == 0) {
                inet_pton(AF_INET, "192.168.0.34", &clientAddr.sin_addr.s_addr);
                printf("Enter the port of the other server\n");
                printf(">> ");
                getline(cin, input);
                clientAddr.sin_port = htons(stoi(input));
                server->join(&clientAddr);
            }
            printf("Starting Server\n");
            server->startServer();
            break;
        case 5:
            printf("Enter the topic to subscribe to\n");
            printf(">> ");
            getline(cin,topic);
            inet_pton(AF_INET, "192.168.0.34", &addr.sin_addr.s_addr);
            addr.sin_port = htons(25565);
            inet_pton(AF_INET, "192.168.0.34", &clientAddr.sin_addr.s_addr);
            subscriber = new Subscriber(topic, &addr, &clientAddr);
            subscriber->start();
            break;
        case 6:
            printf("Enter the topic to publish to\n");
            printf(">> ");
            getline(cin,topic);
            inet_pton(AF_INET, "192.168.0.34", &addr.sin_addr.s_addr);
            addr.sin_port = htons(25565);
            inet_pton(AF_INET, "192.168.0.34", &clientAddr.sin_addr.s_addr);
            publisher = new Publisher(topic, &addr, &clientAddr);
            publisher->start();
            break;
        default:
            printf("Invalid option. Exiting\n");
            break;
    }
    return 1;
}