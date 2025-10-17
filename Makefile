# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: junjun <junjun@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/08 15:42:03 by junjun            #+#    #+#              #
#    Updated: 2025/10/17 17:14:05 by junjun           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME        = ircserv

CXX         = c++

CXXFLAGS    = -Wall -Wextra -Werror -std=c++98

SRCS		= src/main.cpp src/Server.cpp src/Client.cpp src/Cmdhandler.cpp \
			  src/Channel.cpp src/Parser.cpp 


OBJS		= $(SRCS:.cpp=.o)

BOT_BIN     = ircbot              # lowercase to match the source name
BOT_SRCS    = src/Bot.cpp src/Parser.cpp   # link parser into the bot
BOT_OBJS    = $(BOT_SRCS:.cpp=.o)

all: $(NAME) $(BOT_BIN)

$(NAME): $(OBJS)
		$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(BOT_BIN): $(BOT_OBJS)
	$(CXX) $(CXXFLAGS) $(BOT_OBJS) -o $(BOT_BIN)

%.o:	 %.cpp
		$(CXX) $(CXXFLAGS) -Iinc -c $< -o $@
	
clean:
	rm -f $(OBJS) $(BOT_OBJS)
	
fclean:		clean
	rm -f $(NAME) $(BOT_BIN)

re:		fclean 
		@$(MAKE) all

.PHONY: all clean fclean re