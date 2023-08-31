#include "Server.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/eventfd.h>
#include "external.pb.h"
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <iostream>


using namespace HMQTT;

Server::Server(const std::string& topic, const sockaddr_in* localAddr) {
	this->topic = topic;
	this->localAddr = *localAddr;
	setupSocket(localAddr);
	fdServerThreadWakeEvent = eventfd(0, 0);
}

void Server::setupSocket(const sockaddr_in* localAddr) {
	fdSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(fdSocket < 0) {
		perror("Error initialising socket");
		exit(1);
	}

	uint64_t yes = 1;
	if(setsockopt(fdSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		perror("Error setting SO_REUSEADDR.");
		exit(1);
	}

	sockaddr_in anyAddr = {0};
	anyAddr.sin_family = AF_INET;
	anyAddr.sin_port = localAddr->sin_port; //byte order already correct
	anyAddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fdSocket, (sockaddr*)&anyAddr, sizeof(anyAddr)) < 0) {
		perror("Error binding socket");
		exit(1);
	}
}


void Server::startServer() {
	while(!closing) {
		const int numfds = 2;
		struct pollfd pollfds[numfds];
		pollfds[0].fd = fdSocket;
		pollfds[0].events = POLLIN;
		pollfds[0].revents = 0;
		pollfds[1].fd = fdServerThreadWakeEvent;
		pollfds[1].events = POLLIN;
		pollfds[1].revents = 0;

		int status;
		status = poll(pollfds, numfds, -1);
		if (status < 0) { //error condition
			perror("poll()ing socket failed");
			exit(1);
		} else if (status > 0) { //event trigg'd
			if(pollfds[0].revents & POLLIN) {//data on socket
				char buffer[BUFFERLENGTH];
				sockaddr addr;
				socklen_t socklen;
				recvfrom(fdSocket, buffer, BUFFERLENGTH, 0, &addr, &socklen);
				EXTERNALMESSAGE message;
				message.ParseFromArray(buffer, BUFFERLENGTH);
				if(message.has_pubconnect()) {
					processPubConnect(message.pubconnect(), message);
				} else if (message.has_publish()) {
					processPublish(message.publish(), &addr, message);
				} else if (message.has_join()) {
					processJoin(message.join(), &addr, message);
				} else {
					processSubscribe(message.subscribe(), message);
				}
			}
		} else { //timeout 

		}
	}
}


void Server::processPubConnect(const PUBCONNECT& message, EXTERNALMESSAGE& wholeMessage) {
	const std::string& topic = message.topic();
	char buffer[BUFFERLENGTH];
	size_t start = 0;
	bool found = false;
	while((start = topic.find('/',start)) != std::string::npos) {
		std::unordered_map<std::string,sockaddr>::iterator delegate = delegations.find(topic.substr(0, start+1));
		if(delegate != delegations.end()) { //if there is a delegate appropriate for this
			wholeMessage.SerializeToArray(buffer, BUFFERLENGTH);
			sendto(fdSocket, buffer, BUFFERLENGTH, 0, &((*delegate).second), sizeof(sockaddr));
			found = true;
			break;
		}
		start+=1;
	}
	if(!found) {
		EXTERNALMESSAGE responseMessage;
		responseMessage.mutable_pubaccept()->set_topic(message.topic());
		responseMessage.SerializeToArray(buffer, BUFFERLENGTH);
		sockaddr_in addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = message.ip();
		addr.sin_port = message.port();
		sendto(fdSocket, buffer, BUFFERLENGTH, 0, (sockaddr*)&addr, sizeof(addr));
	}
}

void Server::processPublish(const PUBLISH& message, sockaddr* sender, EXTERNALMESSAGE& wholeMessage) {
	const std::string& topic = message.topic();
	printf("Processing message with topic: %s!\n", topic.c_str());
	char buffer[BUFFERLENGTH];
	size_t start = 0;
	bool found = false;
	while((start = topic.find('/',start)) != std::string::npos) {
		std::unordered_map<std::string, sockaddr>::iterator delegate = delegations.find(topic.substr(0, start));
		if(delegate != delegations.end()) {
			EXTERNALMESSAGE responseMessage;
			PUBACK* ack = responseMessage.mutable_puback();
			ack->set_topic(message.topic());
			ack->set_status(PUBACK_AckStatus::PUBACK_AckStatus_OUTOFJURIS);
			responseMessage.SerializeToArray(buffer, BUFFERLENGTH);
			sendto(fdSocket, buffer, BUFFERLENGTH, 0, sender, sizeof(sockaddr));
			found = true;
			break;
		}
		start += 1;
	}
	if(!found) {
		wholeMessage.SerializeToArray(buffer, BUFFERLENGTH);
		for (Subscription sub : subscriptions) {
			std::string& subTopic = sub.topic;
			size_t previousTokenSub = 1;
			size_t previousTokenPub = 1;
			size_t nextTokenPub = 1;
			size_t nextTokenSub = 1;
			bool difference = false;
			while(((nextTokenPub = topic.find('/', previousTokenPub)) != std::string::npos) &&
			      ((nextTokenSub = subTopic.find('/', previousTokenSub)) != std::string::npos)) {
				std::string subToken = subTopic.substr(previousTokenSub, nextTokenSub - previousTokenSub);
				std::string pubToken = topic.substr(previousTokenPub, nextTokenPub - previousTokenPub);
				if(subToken.compare(pubToken) != 0 && subToken.compare("+") != 0 && subToken.compare("*") != 0) { //if tokens are different
					difference = true;
					break;
				}
				if(subToken.compare("*") == 0) {
					break;
				}
				previousTokenPub = nextTokenPub + 1;
				previousTokenSub = nextTokenSub + 1;
			}
			if(!difference) {
				//send publish to subscriber
				sendto(fdSocket, buffer, BUFFERLENGTH, 0, &(sub.addr), sizeof(sockaddr));
			}
		}
		EXTERNALMESSAGE ackMessage;
		PUBACK* ack = ackMessage.mutable_puback(); 
		ack->set_topic(message.topic());
		ack->set_status(PUBACK_AckStatus::PUBACK_AckStatus_SUCCESS);
		ackMessage.SerializeToArray(buffer, BUFFERLENGTH);
		sendto(fdSocket, buffer, BUFFERLENGTH, 0, sender, sizeof(sockaddr));
	}
}

void Server::processSubscribe(const SUBSCRIBE& message, EXTERNALMESSAGE& wholeMessage) {
	const std::string& subTopic = message.topic();
	bool complete = false; //do delegations completely cover this subscription
	char buffer[BUFFERLENGTH];
	for ( auto it = delegations.begin(); it != delegations.end() && !complete; ++it ) {
		const std::string& delTopic = (*it).first;
		size_t previousTokenDel = 1;
		size_t previousTokenSub	= 1;
		size_t nextTokenDel = 1;
		size_t nextTokenSub = 1;
		bool wildcardFound = false; //does *this* subscription completely cover this subscription
		bool globalWildcardFound = false;
		bool match = true;
		while (((nextTokenDel = delTopic.find('/', previousTokenDel)) != std::string::npos) && 
		       ((nextTokenSub = subTopic.find('/', previousTokenSub)) != std::string::npos)) {
			std::string delToken = delTopic.substr(previousTokenDel, nextTokenDel - previousTokenDel);
			std::string subToken = subTopic.substr(previousTokenSub, nextTokenSub - previousTokenSub);
			if(subToken.compare("+") == 0) {
				wildcardFound = true;
			}
			if (subToken.compare("*") == 0) {
				globalWildcardFound = true;
				wildcardFound = true;
				break;
			} else if (!(subToken.compare("+") == 0 || subToken.compare(delToken) == 0)) { //if tokens dont match and not a SL-wildcard
				match == false; //mismatch found
				break; //not a match for this delegation
			}
			previousTokenDel = nextTokenDel+1;
			previousTokenSub = nextTokenSub+1;
		}

		if(nextTokenDel == std::string::npos || globalWildcardFound) { //if run out of tokens on delegation
			if(match) {
				wholeMessage.SerializeToArray(buffer, BUFFERLENGTH);
				sendto(fdSocket, buffer, BUFFERLENGTH, 0, &((*it).second), sizeof(sockaddr));
				if(!wildcardFound) {
					complete = true;
					break;
				}
			}
	 	}
	}

	if(!complete) { //if this server may need to handle some parts of this subscription
		Subscription sub;
		sockaddr_in& subAddr = (sockaddr_in&)sub.addr;
		subAddr.sin_addr.s_addr = message.ip();
		subAddr.sin_port = message.port();
		subAddr.sin_family = AF_INET;
		sub.topic = message.topic();
		subscriptions.push_back(sub);
	}
}


void Server::processJoin(const JOIN& message, sockaddr* sender, EXTERNALMESSAGE& wholeMessage) {
	std::pair<std::string, sockaddr> pair;
	pair.first = message.topic();
	sockaddr_in* pairAddr = (sockaddr_in*)&pair.second;
	pairAddr->sin_family = AF_INET;
	pairAddr->sin_port = message.port();
	pairAddr->sin_addr.s_addr = message.ip();
	delegations.insert(pair);
}

void Server::join(const sockaddr_in* parentAddr) {
	char buffer[BUFFERLENGTH];
	EXTERNALMESSAGE message;
	JOIN* joinMsg = message.mutable_join();
	joinMsg->set_topic(topic);
	joinMsg->set_ip(localAddr.sin_addr.s_addr);
	joinMsg->set_port(localAddr.sin_port);
	message.SerializeToArray(buffer,BUFFERLENGTH);
	sendto(fdSocket,buffer, BUFFERLENGTH, 0, (sockaddr*)parentAddr, sizeof(sockaddr_in));
}