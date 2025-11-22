/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gahmed <gahmed@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 14:30:35 by gahmed            #+#    #+#             */
/*   Updated: 2025/09/28 13:08:56 by gahmed           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Client.hpp"

Client::Client(int socket_fd) : fd(socket_fd), nickname(""), username(""), hostname(""), authenticated(false),
                                registered(false), inbuff(""), outbuff("") {}

Client::Client(const Client& other) {*this = other;}
Client& Client::operator=(const Client& other) {
    if (this != &other) {
        fd = other.fd;
        nickname = other.nickname;
        username = other.username;
        hostname = other.hostname;
        authenticated = other.authenticated;
        registered = other.registered;
        inbuff = other.inbuff;
        outbuff = other.outbuff;
        joinedChannels = other.joinedChannels;
    }
    return *this;
}

Client::~Client() { if (fd != -1) close(fd); }
int Client::getFd() const { return fd; }
std::string Client::getNickname() const { return nickname; }
std::string Client::getHostname() const { return hostname; }
bool Client::getAuthenticated() const { return authenticated; }
bool Client::isRegistered() const { return registered; }

void Client::setPassOk(bool ok) { 
    authenticated = ok;
    checkRegistrationComplete();
}

void Client::setNickname(const std::string& nick) {
    nickname = nick;
    checkRegistrationComplete();
}

void Client::setUsername(const std::string& user) {
    username = user;
    checkRegistrationComplete();
}

void Client::checkRegistrationComplete() {
    if (authenticated && !nickname.empty() && !username.empty()) {
        registered = true;
    }
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

void Client::appendInbuff(const char* data, size_t n) {
    if (n == 0 || !data) 
        return; 
    inbuff.append(data, n); 
}

bool Client::extractLine(std::string& line){
    size_t nl = inbuff.find('\n');
    if (nl == std::string::npos) {
        size_t cr = inbuff.find('\r');
        if (cr == std::string::npos) return false;          // no line yet
        line.assign(inbuff, 0, cr);                          // up to CR
        inbuff.erase(0, cr + 1);                            // drop CR
    } else {
        size_t end = (nl > 0 && inbuff[nl - 1] == '\r') ? nl - 1 : nl;
        line.assign(inbuff, 0, end);                         // up to before CR/LF
        inbuff.erase(0, nl + 1);                            // drop LF (and CR if present)
    }
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    line.erase(std::remove(line.begin(), line.end(), '\0'), line.end());
    return true;
}

bool Client::sendMessage(const std::string& message) {
    if (message.empty()) return true;
    outbuff += message;
    outbuff += CRLF;
    return flushOutput();
}

void Client::markForClose() { 
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

bool Client::flushOutput() {
    if (outbuff.empty() || fd < 0) return true;
    ssize_t sent = send(fd, outbuff.c_str(), outbuff.size(), MSG_DONTWAIT);
    if (sent > 0) {
        outbuff.erase(0, static_cast<size_t>(sent));
        return outbuff.empty();
    }
    if (sent == 0) return false; // try again later
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
        return false;
    std::perror("send");
    markForClose();
    return false;
}

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
        joinedChannels.erase(joinedChannels.begin() + idx);
    }
}

bool Client::hasOutput() const { return !outbuff.empty(); }
std::string& Client::getOutput() { return outbuff; }
std::vector<std::string> Client::getChannels() const { return joinedChannels; }
