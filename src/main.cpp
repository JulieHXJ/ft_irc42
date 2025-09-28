/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 15:49:16 by junjun            #+#    #+#             */
/*   Updated: 2025/09/28 19:09:48 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include <csignal> // signal, SIGPIPE, SIG_IGN
#include <cstdlib> // std::atoi
#include <iostream>
#include <stdexcept>

volatile sig_atomic_t g_stop = 0;  // signal flag defination
static void onSig(int){ g_stop = 1; } //handler

static int parsePort(int argc, char** argv) {
    int port = (argc >= 2) ? std::atoi(argv[1]) : 6667;
    if (port <= 0 || port > 65535) {
        throw std::invalid_argument("invalid port (must be 1..65535)");
    }
    return port;
}


/**
 * Main entry point for the IRC server.
 * Usage: ./ircserv [port]
 * Default port is 6667 if not specified.*/
int main(int argc, char** argv) {
    try
    {
        const int port = parsePort(argc, argv);
        
        // Ignore SIGPIPE (safe even if send() uses MSG_NOSIGNAL)
        if (std::signal(SIGPIPE, SIG_IGN) == SIG_ERR)
            throw std::runtime_error("failed to set SIGPIPE");

        // Graceful stop on Ctrl-C, SIGQUIT, SIGTERM
        if (std::signal(SIGINT,  onSig) == SIG_ERR ||
            std::signal(SIGQUIT, onSig) == SIG_ERR ||
            std::signal(SIGTERM, onSig) == SIG_ERR) {
            throw std::runtime_error("failed to set signal handlers");
        }
        
        Server server;
        std::cout << "Starting server on port " << port << "...\n";
        server.serverInit(port);
        server.run();
        server.closeFds();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    }
	return 0;
}
