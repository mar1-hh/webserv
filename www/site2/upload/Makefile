NAME = webserv

CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98

RM := rm -rf

SRC = server_core/server.cpp \
	server_core/server_core.cpp \
	server_core/ConfigFileParser/parser.cpp \
	request-responce/HttpRequest.cpp \
	request-responce/HttpResponce.cpp \
	cgi/env_builder.cpp

INC = server_core/server_core.hpp \
	server_core/ConfigFileParser/parser.hpp \
	request-responce/HttpRequest.hpp \
	request-responce/HttpResponce.hpp \
	request-responce/Session.hpp \
	cgi/cgi.hpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp $(INC)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
