#pragma once

#include <string>
#include <sys/socket.h>

namespace HMQTT {
    struct Subscription {
        std::string topic;
        sockaddr addr = {0};
    };
}