/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gahmed <gahmed@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/24 14:30:35 by gahmed            #+#    #+#             */
/*   Updated: 2025/09/28 13:03:48 by gahmed           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "./inc/ClientA.hpp"


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