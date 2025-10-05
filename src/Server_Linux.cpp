/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server_Linux.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/28 19:07:59 by xhuang            #+#    #+#             */
/*   Updated: 2025/09/28 19:17:48 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../inc/Server.hpp"

#include <unistd.h>         // close, shutdown
#include <arpa/inet.h>      // inet_ntop, htons/ntohs
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/ioctl.h>      // ioctl, FIONBIO
#include <csignal>          // signal, SIG_IGN

extern volatile sig_atomic_t g_stop; // defined in main

namespace {
    const size_t MAX_OUTBUF = 256 * 1024; // 256 KB per client

    void logErr(const char* s){ ::perror(s); }

    void logNew(int fd, const sockaddr_in& a){
        char ip[INET_ADDRSTRLEN] = {0};
        ::inet_ntop(AF_INET, &a.sin_addr, ip, sizeof(ip));
        std::cout << "[+] New connection from " << ip << ":" << ntohs(a.sin_port)
                  << ", fd=" << fd << "\n";
    }

    void logClose(int fd){ std::cout << "[-] fd=" << fd << " closed\n"; }
}

/* ============================ helpers =================================== */

void Server::setNonBlocking(int fd){
    int on = 1;
    if (::ioctl(fd, FIONBIO, &on) == -1) {
        throw std::runtime_error(std::string("ioctl(FIONBIO): ") + std::strerror(errno));
    }
}

void Server::addPollFd(std::vector<pollfd>& pfds, int sckfd, short events) {
    struct pollfd pfd;
    pfd.fd = sckfd;
    pfd.events = events;
    pfd.revents = 0;
    pfds.push_back(pfd);
}

void Server::removePollFd(std::vector<pollfd>& pfds, size_t index) {
    pfds[index] = pfds.back();
    pfds.pop_back();
}

bool Server::getLine(std::string& inbuffer, std::string& line) {
    std::string::size_type pos = inbuffer.find('\n');
    if (pos == std::string::npos) return false;
    line = inbuffer.substr(0, pos);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    inbuffer.erase(0, pos + 1);
    return true;
}

/* ============================ Initialization ================================= */

void Server::closeFds(){
    for (size_t i = 0; i < pollfds.size(); ++i) {
        ::close(pollfds[i].fd);
    }
    pollfds.clear();
    inbuff.clear();
    outbuff.clear();
}

void Server::serverInit(int port){
    // 1) socket
    listenfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        throw std::runtime_error(std::string("socket: ") + std::strerror(errno));
    }

    // 2) options
    int yes = 1;
    if (::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        ::close(listenfd);
        throw std::runtime_error(std::string("setsockopt(SO_REUSEADDR): ") + std::strerror(errno));
    }
#ifdef SO_REUSEPORT
    if (::setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0) {
        ::close(listenfd);
        throw std::runtime_error(std::string("setsockopt(SO_REUSEPORT): ") + std::strerror(errno));
    }
#endif

    // 3) bind
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port        = htons(port);
    if (::bind(listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(listenfd);
        throw std::runtime_error(std::string("bind: ") + std::strerror(errno));
    }

    // 4) listen
    if (::listen(listenfd, 64) < 0) {
        ::close(listenfd);
        throw std::runtime_error(std::string("listen: ") + std::strerror(errno));
    }

    // 5) non-block + poll vector
    setNonBlocking(listenfd);
    pollfds.clear();
    addPollFd(pollfds, listenfd, POLLIN);

    std::cout << "Listening on 0.0.0.0:" << port << " ... (waiting clients)\n";
}



/* ============================ run loop herlpers ============================= */

void Server::acceptNew(){
    if (pollfds.empty() || !(pollfds[0].revents & POLLIN)) return;

    for(;;){
        sockaddr_in clientAddr; socklen_t clientLen = sizeof(clientAddr);
        int connfd = ::accept(listenfd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (connfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            if (errno == EINTR) continue;
            logErr("accept");
            break;
        }

        setNonBlocking(connfd);
        addPollFd(pollfds, connfd, POLLIN);

        // init buffers for this client
        inbuff[connfd].clear();
        outbuff[connfd] += ":Server NOTICE * :🎉🎉 Yo! Welcome to *Club42 Chatroom* 🍹\r\n";
        // enable write right away (the new entry is at the back)
        pollfds.back().events |= POLLOUT;

        logNew(connfd, clientAddr);
    }
}

void Server::cleanupIndex(size_t i){
    int fd = pollfds[i].fd;
    logClose(fd);
    ::close(fd);

    inbuff.erase(fd);
    outbuff.erase(fd);
    // TODO: also remove from fd2client, nick2client, channels once those are wired

    removePollFd(pollfds, i);
}

bool Server::handleReadable(size_t i){
    if (i >= pollfds.size()) return false;
    int fd = pollfds[i].fd;
    char buf[4096];

    // read until EAGAIN
    for(;;){
        ssize_t r = ::recv(fd, buf, sizeof(buf), 0);
        if (r > 0) {
            inbuff[fd].append(buf, static_cast<size_t>(r));
        } else if (r == 0) {
            // peer closed
            cleanupIndex(i);
            return true;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            logErr("recv");
            cleanupIndex(i);
            return true;
        }
    }

    // parse lines
    std::string line;
    while (getLine(inbuff[fd], line)) {
        // trim leading whitespace
        while (!line.empty() && (line[0]==' ' || line[0]=='\t')) line.erase(0,1);

        // hook for real command parsing later
        // handleCommand(fd, line);

        if (line == "KILL") {
            ::shutdown(fd, SHUT_RDWR);
            continue;
        }
        if (line == "WHO") {
            std::string reply = "USERS:";
            for (size_t k = 1; k < pollfds.size(); ++k)
                reply += " fd=" + std::to_string(pollfds[k].fd);// to_string not allowed
            outbuff[fd] += reply + "\r\n";
            pollfds[i].events |= POLLOUT;
            continue;
        }

        // broadcast to all other clients
        for (size_t j = 1; j < pollfds.size(); ++j) {
            int other = pollfds[j].fd;
            if (other == fd) continue;
            if (outbuff[other].size() + line.size() + 2 <= MAX_OUTBUF) {
                outbuff[other] += line;
                outbuff[other] += "\r\n";
                pollfds[j].events |= POLLOUT;
            }
        }
    }

    // if we queued something for this fd, make sure POLLOUT is set
    if (!outbuff[fd].empty())
        pollfds[i].events |= POLLOUT;

    return false;
}

bool Server::handleWritable(size_t i){
    if (i >= pollfds.size()) return false;
    int fd = pollfds[i].fd;
    std::string& ob = outbuff[fd];

    while (!ob.empty()){
#ifdef MSG_NOSIGNAL
        ssize_t w = ::send(fd, ob.data(), ob.size(), MSG_NOSIGNAL);
#else
        ssize_t w = ::send(fd, ob.data(), ob.size(), 0);
#endif
        if (w > 0) {
            ob.erase(0, static_cast<size_t>(w));
        } else if (w < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            // kernel/peer not ready for more
            return false;
        } else {
            logErr("send");
            cleanupIndex(i);
            return true;
        }
    }

    // everything sent: go back to read-only interest
    pollfds[i].events = POLLIN;
    return false;
}
