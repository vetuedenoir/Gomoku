
# =============================================================================
# PLATFORM DETECTION
# =============================================================================

UNAME_S   := $(shell uname -s)
NPROC     := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

ifeq ($(UNAME_S),Linux)
	CXX        := c++
	CXXFLAGS_OS := -D_GNU_SOURCE
	RPATH_FLAG  = -Wl,-rpath,$(CURDIR)/$(SFML_LIB)
else ifeq ($(UNAME_S),Darwin)
	CXX        := c++
	CXXFLAGS_OS :=
	RPATH_FLAG  :=
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
# SFML (built from source)
# =============================================================================

SFML_URL      := https://github.com/SFML/SFML.git
SFML_TAG      := 3.0.0
SFML_DIR      := libs/SFML
SFML_BUILD    := $(SFML_DIR)/build
SFML_INC      := $(SFML_DIR)/include
SFML_LIB      := $(SFML_BUILD)/lib
SFML_SENTINEL := $(SFML_BUILD)/.built

# =============================================================================
# COMPILER FLAGS
# =============================================================================

# SFML 3.0 requires C++17
CXXFLAGS  := -std=c++17 -Wall -Wextra -Werror \
             -I$(INC_DIR) -I$(SFML_INC) \
             $(CXXFLAGS_OS) \
             -MMD -MP

LDFLAGS   := -L$(SFML_LIB) \
             -lsfml-graphics -lsfml-window -lsfml-system \
             $(RPATH_FLAG)

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

.PHONY: all clean fclean re sfml

all: $(SFML_SENTINEL) $(NAME)

$(NAME): $(OBJS)
	@echo "$(CYAN)Linking $(NAME)...$(NOC)"
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "$(GREEN)$(NAME) built successfully$(NOC)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "$(CYAN)Compiling $<...$(NOC)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --------------------------------------------------------------------------
# SFML: clone + build with CMake
# --------------------------------------------------------------------------

sfml: $(SFML_SENTINEL)

$(SFML_SENTINEL):
	@if [ ! -d "$(SFML_DIR)/.git" ]; then \
		echo "$(CYAN)Cloning SFML $(SFML_TAG)...$(NOC)"; \
		git clone --depth 1 --branch $(SFML_TAG) $(SFML_URL) $(SFML_DIR); \
	fi
	@echo "$(CYAN)Configuring SFML with CMake...$(NOC)"
	cmake -S $(SFML_DIR) -B $(SFML_BUILD) \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SHARED_LIBS=ON \
		-DSFML_BUILD_EXAMPLES=OFF \
		-DSFML_BUILD_TEST_SUITE=OFF
	@echo "$(CYAN)Building SFML (using $(NPROC) jobs)...$(NOC)"
	cmake --build $(SFML_BUILD) -- -j$(NPROC)
	@touch $(SFML_SENTINEL)
	@echo "$(GREEN)SFML built successfully$(NOC)"

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

-include $(DEPS)
