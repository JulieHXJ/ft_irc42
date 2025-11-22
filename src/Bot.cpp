/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Bot.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/17 13:43:43 by junjun            #+#    #+#             */
/*   Updated: 2025/10/17 22:36:02 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include "../inc/Parser.hpp"
#include "../inc/Log.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static volatile sig_atomic_t g_stop = 0;
static void on_sigint(int) { g_stop = 1; }

// --- tiny utils ---
static void trimCRLF(std::string& s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
		s.erase(s.size()-1);
	}
}
static void sendln(int fd, const std::string& s) {
    std::string line = s + "\r\n";
    ::send(fd, line.c_str(), line.size(), 0);
}

static int toInt(const std::string& s, int defv) {
    char* end = 0; long v = std::strtol(s.c_str(), &end, 10);
    if (!end || *end)
        return defv;
    return (int)v;
}
static int dice(int n) {
    if (n < 1) n = 6;
    return 1 + (std::rand() % n);
}

// --- Bot core ---
// Listens to messages.
// Reacts to certain patterns or commands.
// Optionally performs automated actions.
int main(int argc, char** argv) {
    std::srand((unsigned)std::time(0));
    std::signal(SIGINT, on_sigint);
    std::signal(SIGTERM, on_sigint);

    // config from CLI or defaults
    const char* host = (argc > 1) ? argv[1] : "127.0.0.1";
    int         port = (argc > 2) ? std::atoi(argv[2]) : 6667;
    std::string pass = (argc > 3) ? argv[3] : "mypass";
    std::string nick = (argc > 4) ? argv[4] : "ircbot";
    // std::string user = (argc > 5) ? argv[5] : "ircbot";

reconnect:
    if (g_stop) return 0;

    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    sockaddr_in a; std::memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET; a.sin_port = htons(port);
    if (::inet_pton(AF_INET, host, &a.sin_addr) != 1) {
        std::cerr << "Invalid host\n"; return 1;
    }

	Log::info("[bot] connecting to " +  std::string(host) + ":" + std::to_string(port) + "...");
    if (::connect(fd, (sockaddr*)&a, sizeof(a)) < 0) {
        perror("connect"); ::close(fd);
        if (!g_stop) { sleep(2); goto reconnect; }
        return 1;
    }

    // register
    sendln(fd, "PASS " + pass);
    sendln(fd, "NICK " + nick);
    sendln(fd, "USER ircbot 0 * :BoBot");

    // recv loop
    std::string inbuf;
    char buf[4096];
    while (!g_stop) {
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
        if (n == 0) {
            Log::closed(fd);
			::close(fd);
            if (!g_stop) { sleep(2); goto reconnect; }
            break;
        }
        if (n < 0) {
            if (errno == EINTR) continue;
            std::perror("recv");
			::close(fd);
            if (!g_stop) { sleep(2); goto reconnect; }
            break;
        }
        inbuf.append(buf, buf + n);

        // process lines
        for (;;) {
            size_t eol = inbuf.find('\n');
            if (eol == std::string::npos) { // need more
                if (inbuf.size() > 65536) inbuf.erase(0, inbuf.size()-1024);
                break;
            }
            std::string line = inbuf.substr(0, eol + 1);
            inbuf.erase(0, eol + 1);
            trimCRLF(line);
            if (line.empty()) continue;

            std::cout << "[ircserv] " << line << "\n";
            IRCMessage m = parseMessage(line);

            // PING → PONG
            if (m.command == "PING") {
                const std::string arg = (m.params.empty()? "" : m.params[0]);
                sendln(fd, "PONG :" + arg);
                continue;
            }

            // auto JOIN if is invited
            // :Alice INVITE IRCBot :#tea
            if (m.command == "INVITE" && m.params.size() >= 2) {
                const std::string invitedNick = m.params[0];
                const std::string invitedChan = m.params[1];
                if (invitedNick == nick) {
                    sendln(fd, "JOIN " + invitedChan);
                    sendln(fd, "PRIVMSG " + invitedChan + " :Thanks for the invite!");
                }
                continue;
            }

            // PRIVMSG <nick> / #tea :message
            if (m.command == "PRIVMSG" && m.params.size() >= 2) {
                std::string target = m.params[0]; // "#tea" or bot nick
                const std::string text   = m.params[1];

				if (target == nick) {
					target = m.prefix;
				}
				// respond to commands
                if (text == "!help") {
                    sendln(fd, "PRIVMSG " + target + " :Commands: !ping, !echo <text>, !roll [N], !time, !help");
                } else if (text == "!ping") {
                    sendln(fd, "PRIVMSG " + target + " :pong");
                } else if (text.rfind("!echo ", 0) == 0) {
                    sendln(fd, "PRIVMSG " + target + " :" + text.substr(6));
                } else if (text.rfind("!roll", 0) == 0) {
                    int N = 6;
                    if (text.size() > 5) {
                        std::string t = text.substr(5);
                        while (!t.empty() && t[0]==' ') t.erase(0,1);
                        if (!t.empty()) N = toInt(t, 6);
                    }
                    int r = dice(N);
                    std::ostringstream oss; oss << "rolled " << r << " / " << (N<1?6:N);
                    sendln(fd, "PRIVMSG " + target + " :" + oss.str());
                } else if (text == "!time") {
                    sendln(fd, "PRIVMSG " + target + " :" + Log::nowStr());
                }
				else if (text == "!quit") {
					sendln(fd, "PRIVMSG " + target + " :Bye ;)");
					g_stop = 1;
					break; // break out of message loop
				}
				continue;
            }
        }
    }
    ::close(fd);
    return 0;
}

/**
 * Commands for ircbot:
 * INVITE ircbot #channel
 * PRIVMSG ircbot :!help → list commands
 * PRIVMSG ircbot :!ping → respond "pong"
 * PRIVMSG ircbot :!echo <text> → respond with <text>
 * PRIVMSG ircbot :!roll [N] → roll a dice 1-N
 * PRIVMSG ircbot :!time → respond with current time
 * PRIVMSG ircbot :!quit → bot quits
 * (replace ircbot with #channel to broadcast in channel)
 */