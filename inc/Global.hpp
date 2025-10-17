/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Global.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: junjun <junjun@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/09 16:43:33 by junjun            #+#    #+#             */
/*   Updated: 2025/10/16 13:17:13 by junjun           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <string>
#include <map>
#include <vector>
#include <iostream>

// ========================
//  IRC Server Global Config
// ========================

// ---- Basic Info ----
#ifndef SERVER_NAME
# define SERVER_NAME "ircserv"
#endif

#ifndef DEFAULT_PORT
# define DEFAULT_PORT 6667
#endif

// ---- Limits ----
#define MAX_CLIENTS     256
#define MAX_CHANNELS    128
#define MAX_NICK_LEN    32
#define MAX_USER_LEN    32
#define MAX_TOPIC_LEN   256
#define MAX_LINE_LEN    510  // per RFC 1459
#define CRLF            "\r\n"


// ========================
//  IRC Numeric Replies (Partial)
// ========================
// --- Welcome block ---
#define RPL_WELCOME        "001"
#define RPL_YOURHOST       "002"
#define RPL_CREATED        "003"
#define RPL_MYINFO         "004"

// --- topic ---
#define RPL_NOTOPIC        "331"
#define RPL_TOPIC          "332"
#define RPL_TOPICWHOTIME   "333"
#define RPL_CHANNELMODEIS  "324"
// Names
#define RPL_INVITING	  "341"
#define RPL_NAMREPLY       "353"
#define RPL_ENDOFNAMES     "366"

// Errors & replies
#define ERR_NOSUCHNICK     "401"
#define ERR_NOSUCHCHANNEL  "403"
#define ERR_CANNOTSENDTOCHAN "404" 
#define ERR_NONICKNAMEGIVEN "431"
#define ERR_NICKNAMEINUSE  "433"
#define ERR_NOTREGISTERED  "451"
#define ERR_NEEDMOREPARAMS "461"
#define ERR_ALREADYREGISTRED "462"
#define ERR_PASSWDMISMATCH "464"
#define ERR_CHANOPRIVSNEEDED "482"

// --- join / part / kick ---
#define ERR_USERNOTINCHANNEL "441"
#define ERR_NOTONCHANNEL     "442"
#define ERR_USERONCHANNEL  "443"
#define ERR_CHANNELISFULL  "471"
#define ERR_INVITEONLYCHAN "473"
#define ERR_BADCHANNELKEY  "475"

// --- privmsg / misc ---
#define ERR_NORECIPIENT    "411"
#define ERR_NOTEXTTOSEND   "412"
#define ERR_UNKNOWNCOMMAND "421"
#define ERR_UNKNOWNMODE "472"
#define ERR_UMODEUNKNOWNFLAG "501"

// ========================
//  Common Types
// ========================
#include <string>
#include <map>
#include <vector>
#include <iostream>

typedef std::map<std::string, class Channel*> ChannelMap;
typedef std::map<int, class Client*>           ClientMap;

#endif // COMMON_HPP