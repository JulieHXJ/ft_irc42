# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: xhuang <xhuang@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/08 15:42:03 by junjun            #+#    #+#              #
#    Updated: 2025/09/14 18:02:07 by xhuang           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME        = ircserv

CXX         = c++

CXXFLAGS    = -Wall -Wextra -Werror -std=c++98

SRCS		= src/main.cpp src/Server.cpp

OBJS		= $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
		$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME) 

%.o:	 %.cpp
		$(CXX) $(CXXFLAGS) -c $< -o $@
	
clean:
	rm -f $(OBJS)
	
fclean:		clean
	rm -f $(NAME)

re:		fclean 
		@$(MAKE) all

.PHONY: all clean fclean re