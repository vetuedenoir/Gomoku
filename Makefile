
# =============================================================================
# PLATFORM DETECTION
# =============================================================================

UNAME_S   := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	CXX        := c++
	CXXFLAGS_OS := -D_GNU_SOURCE
else ifeq ($(UNAME_S),Darwin)
	CXX        := c++
	CXXFLAGS_OS :=
endif

# =============================================================================
# PROJECT VARIABLES
# =============================================================================

NAME      := gomoku
SRC_DIR   := src
INC_DIR   := include
OBJ_DIR   := .build

SRCS      := main.cpp

# =============================================================================
# COMPILER FLAGS
# =============================================================================

CXXFLAGS  := -std=c++17 -Wall -Wextra -Werror \
             -I$(INC_DIR) \
             $(CXXFLAGS_OS) \
             -MMD -MP

LDFLAGS   := -lsfml-graphics -lsfml-window -lsfml-system

# =============================================================================
# OBJECT & DEPENDENCY FILES
# =============================================================================

OBJS      := $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS      := $(OBJS:.o=.d)

# =============================================================================
# COLORS
# =============================================================================

NOC    = \033[0m
GREEN  = \033[1;32m
CYAN   = \033[1;36m
YELLOW = \033[1;33m
RED    = \033[1;31m

# =============================================================================
# RULES
# =============================================================================

DOCKER_IMAGE := gomoku
DOCKER_TMP   := gomoku-tmp

.PHONY: all clean fclean re \
        docker-build docker-run docker-extract docker-clean

all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(CYAN)Linking $(NAME)...$(NOC)"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)$(NAME) built successfully$(NOC)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "$(CYAN)Compiling $<...$(NOC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# =============================================================================
# CLEANING RULES
# =============================================================================

clean:
	@echo "$(CYAN)Removing build artifacts...$(NOC)"
	rm -rf $(OBJ_DIR)
	@echo "$(GREEN)Done$(NOC)"

fclean: clean
	@echo "$(CYAN)Removing binary...$(NOC)"
	rm -f $(NAME)
	@echo "$(GREEN)Done$(NOC)"

re: fclean all

# =============================================================================
# DOCKER RULES
# =============================================================================

docker-build:
	@echo "$(CYAN)Building Docker image '$(DOCKER_IMAGE)'...$(NOC)"
	docker build -t $(DOCKER_IMAGE) .
	@echo "$(GREEN)Image '$(DOCKER_IMAGE)' ready$(NOC)"

docker-run: docker-build
	@echo "$(CYAN)Allowing local Docker connections to X11...$(NOC)"
	xhost +local:docker
	@echo "$(CYAN)Running $(DOCKER_IMAGE) with X11 forwarding...$(NOC)"
	docker run --rm \
		-e DISPLAY=$(DISPLAY) \
		-v /tmp/.X11-unix:/tmp/.X11-unix \
		$(DOCKER_IMAGE)

docker-extract: docker-build
	@echo "$(CYAN)Extracting binary from image...$(NOC)"
	docker create --name $(DOCKER_TMP) $(DOCKER_IMAGE)
	docker cp $(DOCKER_TMP):/app/$(NAME) ./$(NAME)
	docker rm $(DOCKER_TMP)
	@echo "$(GREEN)Binary extracted: ./$(NAME)$(NOC)"

docker-clean:
	@echo "$(CYAN)Removing Docker image '$(DOCKER_IMAGE)'...$(NOC)"
	docker rmi -f $(DOCKER_IMAGE)
	@echo "$(GREEN)Done$(NOC)"

-include $(DEPS)
