/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 17:15:51 by junjun            #+#    #+#             */
/*   Updated: 2025/10/09 23:17:12 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>
#include <iomanip>
#include <string>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace Log {

inline std::string nowStr() {
    std::time_t t = std::time(NULL);
    char buf[32]; std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

// ANSI colors 
#define C_RESET  "\033[0m"
#define C_GREY   "\033[90m"
#define C_RED    "\033[31m"
#define C_GRN    "\033[32m"
#define C_YEL    "\033[33m"
#define C_BLU    "\033[34m"
#define C_MAG    "\033[35m"
#define C_CYN    "\033[36m"

inline void info (const std::string& m) { std::cout  << C_GRN << "[INFO "  << nowStr() << "] " << m << C_RESET << "\n"; }
inline void warn (const std::string& m) { std::cout  << C_YEL << "[WARN "  << nowStr() << "] " << m << C_RESET << "\n"; }
inline void error(const std::string& m) { std::cerr << C_RED << "[ERR  "  << nowStr() << "] " << m << C_RESET << "\n"; }
inline void dbg  (const std::string& m) { std::cout  << C_CYN << "[DBG  "  << nowStr() << "] " << m << C_RESET << "\n"; }

// ---- Specific events ----


// namespace {

// 	void logNew(int fd, const sockaddr_in& a){
//         char ip[INET_ADDRSTRLEN] = {0};
//         ::inet_ntop(AF_INET, &a.sin_addr, ip, sizeof(ip));
//         std::cout << "[+] New connection from " << ip << ":" << ntohs(a.sin_port)
//                   << ", fd=" << fd << "\n";
//     }
	
// 	void logClose(int fd){ std::cout << "[-] fd=" << fd << " closed\n"; }
// }
inline void newConnect(int fd, const sockaddr_in& a) {
    char ip[INET_ADDRSTRLEN] = {0};
    ::inet_ntop(AF_INET, &a.sin_addr, ip, sizeof(ip));
    std::cout << C_BLU << "[+] " << nowStr() << " New connection from "
              << ip << ":" << ntohs(a.sin_port) << ", fd=" << fd << C_RESET << "\n";
}

inline void closed(int fd) {
    std::cout << C_MAG << "[-] " << nowStr() << " fd=" << fd << " closed" << C_RESET << "\n";
}

inline void recvBytes(int fd, size_t n) {
    std::cout << C_GREY << "[RECV " << nowStr() << "] fd=" << fd << " +" << n << " bytes" << C_RESET << "\n";
}

inline void sendBytes(int fd, size_t n) {
    std::cout << C_GREY << "[SEND " << nowStr() << "] fd=" << fd << " -" << n << " bytes" << C_RESET << "\n";
}

inline void joinEvt(const std::string& nick, const std::string& chan) {
    info(nick + " JOIN " + chan);
}
inline void partEvt(const std::string& nick, const std::string& chan, const std::string& reason) {
    info(nick + " PART " + chan + (reason.empty()? "" : " : " + reason));
}
inline void kickEvt(const std::string& op, const std::string& chan, const std::string& target, const std::string& reason) {
    warn(op + " KICK " + chan + " " + target + (reason.empty()? "" : " : " + reason));
}
inline void topicEvt(const std::string& nick, const std::string& chan, const std::string& topic) {
    info(nick + " TOPIC " + chan + " : " + topic);
}
inline void modeEvt(const std::string& nick, const std::string& chan, const std::string& spec) {
    info(nick + " MODE " + chan + " " + spec);
}

} // namespace Log

#endif // LOG_HPP
