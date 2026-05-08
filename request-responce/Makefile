NAME = reqres
RED     := \033[0;31m
GREEN   := \033[0;32m
YELLOW  := \033[0;33m
RESET   := \033[0m

CXX := c++

CXXFLAGS := -Wall -Wextra -Werror -std=c++98

RM := rm -rf

SRC = HttpRequest.cpp main.cpp

INC := HttpRequest.hpp

OBJ := $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	@printf "$(GREEN)  Linking $(NAME)...$(RESET)\n"
	@$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)
	@printf "$(GREEN)  Successfully built $(NAME)$(RESET)\n"

%.o: %.cpp $(INC)
	@printf "$(YELLOW)  Compiling $<...$(RESET)\n"
	@$(CXX) $(CXXFLAGS) -c $< -o $@


clean:
	@printf "$(RED)  Cleaning object files...$(RESET)\n"
	@$(RM) $(OBJ)
	@printf "$(GREEN)  Object files removed$(RESET)\n"

fclean: clean
	@printf "$(RED)  Removing executable...$(RESET)\n"
	@rm -f *_shrubbery
	@$(RM) $(NAME)
	@printf "$(GREEN)  Executable file removed$(RESET)\n"

re: fclean all
	@printf "$(YELLOW)  Rebuilding project...$(RESET)\n"

.PHONY: all clean fclean re