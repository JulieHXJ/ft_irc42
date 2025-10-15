/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 16:59:35 by xhuang            #+#    #+#             */
/*   Updated: 2025/10/15 17:41:11 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Client.hpp"

void Client::markForClose() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

void Client::setRegistered() {
    if (!registered && !nickname.empty() && !username.empty() && pass_ok) {
        registered = true;
        std::cout << "Client fd=" << fd << " (" << nickname << ") has completed registration.\n";
    }
}

//extract a complete lines from client inbuff without \r\n
bool Client::extractLine(std::string& line){
    
    // std::string::size_type pos = inbuff.find(CRLF);
    // if (pos == std::string::npos) return false;
    // line = inbuff.substr(0, pos);//pop out the line without \r\n
    // inbuff.erase(0, pos + 2);//remove the line with \r\n
    // return true;

    // Find '\n' first (covers \r\n and lone \n)
    size_t nl = inbuff.find('\n');
    if (nl == std::string::npos) {
        // Maybe there's a lone '\r' (some clients)
        size_t cr = inbuff.find('\r');
        if (cr == std::string::npos) return false;          // no line yet
        line.assign(inbuff, 0, cr);                          // up to CR
        inbuff.erase(0, cr + 1);                            // drop CR
    } else {
        size_t end = (nl > 0 && inbuff[nl - 1] == '\r') ? nl - 1 : nl;
        line.assign(inbuff, 0, end);                         // up to before CR/LF
        inbuff.erase(0, nl + 1);                            // drop LF (and CR if present)
    }

    // Strip any stray \r or \n (paranoia)
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    // Optional: trim embedded NULs
    line.erase(std::remove(line.begin(), line.end(), '\0'), line.end());
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
    printf("host detected: %s\n", hostname.c_str());
}

//to do
bool Client::sendMessage(const std::string& message) {
    if (message.empty()) return true;

    // Append CRLF as required by IRC spec
    outbuff += message;
    outbuff += CRLF;

    return flushOutput();
}

bool Client::flushOutput() {
    if (outbuff.empty() || fd < 0) return true;

    ssize_t sent = send(fd, outbuff.c_str(), outbuff.size(), MSG_DONTWAIT);
    if (sent > 0) {
        outbuff.erase(0, sent);
        return outbuff.empty();
    }

    // EAGAIN means would block — leave data in buffer for later
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        return false;

    // Any other error -> mark for close
    perror("send");
    markForClose();
    return false;
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
