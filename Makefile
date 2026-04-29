
-include .env

# =============================================================================
# PLATFORM DETECTION
# =============================================================================

UNAME_S   := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	NPROC := $(shell nproc)
	CXX         := c++
	CXXFLAGS_OS := -D_GNU_SOURCE
	LDFLAGS_OS  :=
else ifeq ($(UNAME_S),Darwin)
	NPROC := $(shell sysctl -n hw.ncpu)
	CXX         := clang++
	CXXFLAGS_OS := -arch arm64 -I$(SFML_ROOT)/include
	LDFLAGS_OS  := -L$(SFML_ROOT)/build/lib \
	               -Wl,-rpath,$(SFML_ROOT)/build/lib \
	               -framework Cocoa \
	               -framework OpenGL \
	               -framework IOKit \
	               -framework CoreVideo
endif

# =============================================================================
# PROJECT VARIABLES
# =============================================================================

NAME      := gomoku
SRC_DIR   := src
INC_DIR   := include
OBJ_DIR   := .build

ALL_SRCS := $(wildcard $(SRC_DIR)/*.cpp) \
	$(wildcard $(SRC_DIR)/game/*.cpp) \
	$(wildcard $(SRC_DIR)/ai/*.cpp) \
	$(wildcard $(SRC_DIR)/logger/*.cpp) \
	$(wildcard $(SRC_DIR)/optimization/*.cpp) \
	$(wildcard $(SRC_DIR)/bitboard/*.cpp) \
	$(wildcard $(SRC_DIR)/ui/*.cpp)

SRCS      := $(patsubst $(SRC_DIR)/%,%,$(ALL_SRCS))

# =============================================================================
# BUILD MODE  —  usage: make MODE=debug / make MODE=release (default)
# =============================================================================

MODE      ?= release

ifeq ($(MODE),debug)
	CXXFLAGS_MODE := -g -O0 -DDEBUG
else
	CXXFLAGS_MODE := -O2 -DNDEBUG
endif

# =============================================================================
# COMPILER FLAGS
# =============================================================================

MAKEFLAGS += -j$(NPROC)

CXXFLAGS  := -std=c++17 -Wall -Wextra -Werror \
             -I$(INC_DIR) \
             $(CXXFLAGS_OS) \
             $(CXXFLAGS_MODE) \
             -MMD -MP

LDFLAGS   := $(LDFLAGS_OS) \
             -lsfml-graphics -lsfml-window -lsfml-system

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
        test run_tests \
        docker-build docker-run docker-extract docker-clean \
	podman-build podman-run podman-extract podman-clean \
	help

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
# TEST RULES  —  no SFML required; only pure-logic sources are compiled
# =============================================================================

TEST_BIN      := tests_runner
TEST_OBJ_DIR  := .build/tests
TEST_DIR      := tests
EXT_INC_DIR   := external/doctest
FILTER        ?=

# Sources compiled for tests: pure logic only — no main, no UI, no SFML
TEST_GAME_SRCS := $(wildcard $(SRC_DIR)/game/*.cpp) $(wildcard $(SRC_DIR)/ai/*.cpp) \
                  $(wildcard $(SRC_DIR)/logger/*.cpp) \
                  $(wildcard $(SRC_DIR)/optimization/*.cpp) \
                  $(wildcard $(SRC_DIR)/bitboard/*.cpp)
TEST_SRCS      := $(wildcard $(TEST_DIR)/*.cpp)

TEST_GAME_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(TEST_OBJ_DIR)/%.o,$(TEST_GAME_SRCS))
TEST_OBJS      := $(patsubst $(TEST_DIR)/%.cpp,$(TEST_OBJ_DIR)/%.o,$(TEST_SRCS))

CXXFLAGS_TEST  := -std=c++17 -Wall -Wextra \
                  -I$(INC_DIR) -I$(EXT_INC_DIR) \
                  $(CXXFLAGS_MODE) \
                  -MMD -MP

$(TEST_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_TEST) -c $< -o $@

$(TEST_OBJ_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS_TEST) -c $< -o $@

$(TEST_BIN): $(TEST_GAME_OBJS) $(TEST_OBJS)
	@echo "$(CYAN)Linking $(TEST_BIN)...$(NOC)"
	$(CXX) $(CXXFLAGS_TEST) -o $@ $^
	@echo "$(GREEN)$(TEST_BIN) built successfully$(NOC)"

test: $(TEST_BIN)

run_tests: $(TEST_BIN)
	@echo "$(CYAN)Running tests...$(NOC)"
	@if [ -n "$(FILTER)" ]; then \
		./$(TEST_BIN) -tc="$(FILTER)"; \
	else \
		./$(TEST_BIN); \
	fi

-include $(TEST_GAME_OBJS:.o=.d)
-include $(TEST_OBJS:.o=.d)

# =============================================================================
# CLEANING RULES
# =============================================================================

clean:
	@echo "$(CYAN)Removing build artifacts...$(NOC)"
	rm -rf $(OBJ_DIR)
	@echo "$(GREEN)Done$(NOC)"

fclean: clean
	@echo "$(CYAN)Removing binary...$(NOC)"
	rm -f $(NAME) $(TEST_BIN)
	@echo "$(GREEN)Done$(NOC)"

re: fclean all


# =============================================================================
# PODMAN RULES
# =============================================================================


podman-build:
	@echo "$(CYAN)Building Podman image '$(DOCKER_IMAGE)'...$(NOC)"
	podman build -t $(DOCKER_IMAGE) .
	@echo "$(GREEN)Image '$(DOCKER_IMAGE)' ready$(NOC)"

podman-run: podman-build
	@echo "$(CYAN)Allowing local Podman connections to X11...$(NOC)"
	xhost +local:
	@echo "$(CYAN)Running $(DOCKER_IMAGE) with X11 forwarding...$(NOC)"
	podman run --rm \
		-e DISPLAY=$(DISPLAY) \
		-v /tmp/.X11-unix:/tmp/.X11-unix \
		$(DOCKER_IMAGE)

podman-extract: podman-build
	@echo "$(CYAN)Extracting binary from image...$(NOC)"
	podman create --name $(DOCKER_TMP) $(DOCKER_IMAGE)
	podman cp $(DOCKER_TMP):/app/$(NAME) ./$(NAME)
	podman rm $(DOCKER_TMP)
	@echo "$(GREEN)Binary extracted: ./$(NAME)$(NOC)"

podman-clean:
	@echo "$(CYAN)Removing Podman image '$(DOCKER_IMAGE)'...$(NOC)"
	podman rmi -f $(DOCKER_IMAGE)
	@echo "$(GREEN)Done$(NOC)"


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

# =============================================================================
# HELP
# =============================================================================

help:
	@echo ""
	@echo "$(CYAN)Usage:$(NOC)  make $(YELLOW)[target]$(NOC) $(GREEN)[MODE=debug|release]$(NOC) $(GREEN)[FILTER=<test_case>]$(NOC)"
	@echo ""
	@echo "$(CYAN)Build targets:$(NOC)"
	@echo "  $(YELLOW)all$(NOC)              	Build the $(NAME) binary (default)"
	@echo "  $(YELLOW)re$(NOC)               	Full rebuild (fclean + all)"
	@echo "  $(YELLOW)clean$(NOC)            	Remove build artifacts (.build/)"
	@echo "  $(YELLOW)fclean$(NOC)           	Remove build artifacts and binaries"
	@echo ""
	@echo "$(CYAN)Test targets:$(NOC)"
	@echo "  $(YELLOW)test$(NOC)             	Build the test runner ($(TEST_BIN))"
	@echo "  $(YELLOW)run_tests$(NOC)        	Build and run all tests"
	@echo "  $(YELLOW)run_tests FILTER=X$(NOC)	Run only test cases matching X"
	@echo ""
	@echo "$(CYAN)Docker targets:$(NOC)"
	@echo "  $(YELLOW)docker-build$(NOC)     	Build the Docker image"
	@echo "  $(YELLOW)docker-run$(NOC)       	Build and run with X11 forwarding"
	@echo "  $(YELLOW)docker-extract$(NOC)   	Extract the binary from the image"
	@echo "  $(YELLOW)docker-clean$(NOC)     	Remove the Docker image"
	@echo ""
	@echo "$(CYAN)Podman targets:$(NOC)"
	@echo "  $(YELLOW)podman-build$(NOC)     	Build the Podman image"
	@echo "  $(YELLOW)podman-run$(NOC)       	Build and run with X11 forwarding"
	@echo "  $(YELLOW)podman-extract$(NOC)   	Extract the binary from the image"
	@echo "  $(YELLOW)podman-clean$(NOC)     	Remove the Podman image"
	@echo ""
	@echo "$(CYAN)Options:$(NOC)"
	@echo "  $(GREEN)MODE=release$(NOC)     	Optimised build (default)"
	@echo "  $(GREEN)MODE=debug$(NOC)       	Debug build (-g -O0 -DDEBUG)"
	@echo ""
