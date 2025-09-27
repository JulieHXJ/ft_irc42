/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 15:49:16 by junjun            #+#    #+#             */
/*   Updated: 2025/09/27 17:53:34 by xhuang           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../inc/Server.hpp"
#include <csignal> // signal, SIGPIPE, SIG_IGN
#include <cstdlib> // std::atoi
#include <iostream>
#include <stdexcept>

#include <atomic>
volatile sig_atomic_t g_stop = 0;  // signal flag defination
static void onSig(int){ g_stop = 1; } //handler

/**
 * Main entry point for the IRC server.
 * Usage: ./ircserv [port]
 * Default port is 6667 if not specified.*/
int main(int argc, char** argv) {
    try
    {
        int port = (argc >= 2) ? std::atoi(argv[1]) : 6667; // default port 6667
        
        if (std::signal(SIGPIPE, SIG_IGN) == SIG_ERR ||
            std::signal(SIGINT,  onSig)    == SIG_ERR ||
            std::signal(SIGQUIT, onSig)    == SIG_ERR) {
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
