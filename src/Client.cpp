/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 16:59:35 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/10 18:02:10 by xhuang           ###   ########.fr       */
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

std::vector<std::string> Client::extractMessages() {
    std::vector<std::string> messages;
    size_t pos = 0;

    // Accept both LF and CRLF as line terminators; strip optional CR
    while ((pos = inbuff.find('\n')) != std::string::npos) {
        std::string line = inbuff.substr(0, pos);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        messages.push_back(line);
        inbuff.erase(0, pos + 1);
    }

    return messages;
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


void Client::sendMessage(const std::string& message) {
    if (message.empty()) return;
    if (outbuff.size() + message.size() + 2 > MAX_LINE_LEN) {
        std::cerr << "Warning: Output buffer overflow for client fd=" << fd << ". Message dropped.\n";
        return;
    }
    outbuff += message + CRLF;

    ssize_t sent = send(fd, outbuff.c_str(), outbuff.length(), MSG_DONTWAIT);
    if (sent > 0) {
        outbuff.erase(0, sent);
        return outbuff.empty();
    }
}

