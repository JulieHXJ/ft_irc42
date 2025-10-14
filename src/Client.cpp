/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 16:59:35 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/14 17:58:01 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Client.hpp"

void Client::setRegistered() {
    if (!registered && !nickname.empty() && !username.empty() && pass_ok) {
        registered = true;
        std::cout << "Client fd=" << fd << " (" << nickname << ") has completed registration.\n";
    }
}

//extract a complete lines from client inbuff without \r\n
bool Client::extractLine(std::string& line){
    std::string::size_type pos = inbuff.find(CRLF);
    if (pos == std::string::npos) return false;
    line = inbuff.substr(0, pos);//pop out the line without \r\n
    inbuff.erase(0, pos + 2);//remove the line with \r\n
    return true;
}

void Client::detectHostname() {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getpeername(fd, (struct sockaddr*)&addr, &addr_len) == 0) {
        hostname = inet_ntoa(addr.sin_addr);
    } else {
        hostname = "unknown";
    }
}

//to do
void Client::sendMessage(const std::string& message) {
    if (message.empty()) return;
    if (outbuff.size() + message.size() + 2 > MAX_LINE_LEN) {
        std::cerr << "Warning: Output buffer overflow for client fd=" << fd << ". Message dropped.\n";
        return;
    }
    outbuff += message;
    outbuff += CRLF;

    // ssize_t sent = send(fd, outbuff.c_str(), outbuff.length(), MSG_DONTWAIT);
    // if (sent > 0) {
    //     outbuff.erase(0, sent);
    //     return outbuff.empty();
    // }
}


// ========== Channel Management Methods ==========
size_t Client::findChannelIndex(const std::string& channel) const {
    for (size_t i = 0; i < joinedChannels.size(); ++i) {
        if (joinedChannels[i] == channel) return i;
    }
    return static_cast<size_t>(-1); // npos
}

void Client::joinChannel(const std::string& channel) {
    if (channel.empty()) return;
    if (findChannelIndex(channel) == static_cast<size_t>(-1)) {
        joinedChannels.push_back(channel); 
    }
}

void Client::leaveChannel(const std::string& channel) {
    if (channel.empty()) return;
    size_t idx = findChannelIndex(channel);
    if (idx != static_cast<size_t>(-1)) {
        joinedChannels.erase(joinedChannels.begin() + static_cast<long>(idx));
    }
}

bool Client::isInChannel(const std::string& channel) const {
    return findChannelIndex(channel) != static_cast<size_t>(-1);
}
