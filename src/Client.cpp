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

#include "./inc/Client.hpp"


Client::Client(int socket_fd) 
    : fd(socket_fd), authenticated(false), registered(false), inbuff(""), outbuff(""), joined("") {
    // Initialize client with socket file descriptor
}

Client::~Client() {
    if (fd != -1) {
        close(fd);
    }
}

bool Client::authenticate(const std::string& password) {
    // Implement password authentication logic



    // This depends on your IRC server's authentication requirements
    (void)password;
    authenticated = true;
    return authenticated;
}

void Client::setNickname(const std::string& nick) {
    nickname = nick;
    checkRegistrationComplete();
}

void Client::setUsername(const std::string& user, const std::string& real) {
    username = user;
    realname = real;
    checkRegistrationComplete();
}

void Client::checkRegistrationComplete() {
    if (!nickname.empty() && !username.empty()) {
        registered = true;
    }
}


void Client::appendToBuffer(const std::string& data) {
    inbuff += data;
}

std::vector<std::string> Client::extractMessages() {
    std::vector<std::string> messages;
    size_t pos = 0;
    
    // IRC messages are terminated with \r\n
    while ((pos = inbuff.find("\r\n")) != std::string::npos) {
        messages.push_back(inbuff.substr(0, pos));
        inbuff.erase(0, pos + 2);
    }
    
    return messages;
}

int Client::getFd() const {
    return fd;
}

std::string Client::getNickname() const {
    return nickname;
}

bool Client::isAuthenticated() const {
    return authenticated;
}

bool Client::isRegistered() const {
    return registered;
}

// receive message
IRCMessage Client::parseMessage(const std::string& raw) {
    IRCMessage msg;
    std::string line = raw;
    
    // Parse prefix (starts with :)
    if (!line.empty() && line[0] == ':') {
        size_t space = line.find(' ');
        if (space != std::string::npos) {
            msg.prefix = line.substr(1, space - 1);
            line = line.substr(space + 1);
        }
    }
    
    // Parse trailing (starts with :)
    size_t trailing_pos = line.find(" :");
    if (trailing_pos != std::string::npos) {
        msg.trailing = line.substr(trailing_pos + 2);
        line = line.substr(0, trailing_pos);
    }
    
    // Parse command and parameters
    std::istringstream iss(line);
    std::string token;
    bool first = true;
    
    while (iss >> token) {
        if (first) {
            msg.command = token;
            first = false;
        } else {
            msg.parameters.push_back(token);
        }
    }
    
    return msg;
}

void Client::receiveOnce() {
    char buffer[1024];
    int bytes_received = recv(fd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        appendToBuffer(std::string(buffer));
        
        std::vector<std::string> messages = extractMessages();
        for (size_t i = 0; i < messages.size(); ++i) {
            std::cout << "RECV: " << messages[i] << std::endl;
        }
    }
    else if (bytes_received == 0) {
        std::cout << "Connection closed by server" << std::endl;
    }
    else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Receive error: " << strerror(errno) << std::endl;
        }
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

bool Client::sendMessage(const std::string& message) {
    std::string msg = message + "\r\n";
    outbuff += msg;
    return flushOutput();
}

bool Client::flushOutput() {
    if (outbuff.empty()) return true;
    
    ssize_t sent = send(fd, outbuff.c_str(), outbuff.length(), MSG_DONTWAIT);
    if (sent > 0) {
        outbuff.erase(0, sent);
        return outbuff.empty();
    }
    return false;
}
