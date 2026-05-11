NAME	= webserv


CXX			= c++
CXXFLAGS	= -std=c++98 -Wall -Wextra -Werror -fsanitize=address
INCLUDES	= -I include
PREFIX		= /usr/local
USER_PREFIX	= $(HOME)/.local


SRCS			= src/main.cpp \
				src/Server.cpp \
				src/cgi/Cgi.cpp \
				src/config/Config.cpp \
				src/config/ConfigDirectives.cpp \
				src/config/Route.cpp \
				src/config/TokenStream.cpp \
				src/config/VirtualHost.cpp \
				src/core/Client.cpp \
				src/core/IOReactor.cpp \
				src/core/ListeningSocket.cpp \
				src/core/Pipe.cpp \
				src/core/Socket.cpp \
				src/http/HttpPipeline.cpp \
				src/http/HttpRequest.cpp \
				src/http/HttpRequestBody.cpp \
				src/utils/HttpRequestBodyUtils.cpp \
				src/utils/HttpRequestHeadersUtils.cpp \
				src/utils/HttpRequestLineUtils.cpp \
				src/http/HttpResponse.cpp \
				src/http/HttpStatusCodes.cpp \
				src/utils/MimeUtils.cpp \
				src/utils/NetworkUtils.cpp \
				src/utils/StringUtils.cpp

OBJDIR	= obj
OBJS	= $(patsubst src/%, $(OBJDIR)/%, $(SRCS:.cpp=.o))


CAN_WRITE	= $(shell [ -w $(PREFIX)/bin ] || [ -w $(PREFIX) ] && echo yes || echo no)
ACTUAL_ROOT	= $(if $(filter yes,$(CAN_WRITE)),$(PREFIX),$(USER_PREFIX))
TARGET_RULE	= $(if $(filter yes,$(CAN_WRITE)),install-sys,install-user)

CXXFLAGS	+= -DInstallDir=\"$(ACTUAL_ROOT)\"


RESET	= $(shell tput sgr0 2>/dev/null)
BOLD	= $(shell tput bold 2>/dev/null)
RED		= $(shell tput setaf 1 2>/dev/null)
GREEN	= $(shell tput setaf 2 2>/dev/null)
YELLOW	= $(shell tput setaf 3 2>/dev/null)
CYAN	= $(shell tput setaf 6 2>/dev/null)


all: .installed


.installed: $(NAME)
	@$(MAKE) -s $(TARGET_RULE)
	@touch .installed

$(NAME): $(OBJS)
	@echo "$(BOLD)$(CYAN)Linking $(NAME)...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	@echo "$(BOLD)$(GREEN)✓ $(NAME) built successfully$(RESET)"

$(OBJDIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@echo "$(YELLOW)Compiling $<...$(RESET)"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@


define install_assets
	@echo "$(BOLD)$(CYAN)Installing webserv assets to $(1)/webserv...$(RESET)"
	@if [ ! -d "$(1)/webserv" ]; then \
		git clone -q https://github.com/ismail-el-moufid/webserv-assets.git $(1)/webserv > /dev/null 2>&1 || true; \
	else \
		echo "$(YELLOW)Assets already exist. Pulling latest...$(RESET)"; \
		git -C $(1)/webserv pull -q > /dev/null 2>&1 || true; \
	fi
endef


install-sys:
	@echo "$(BOLD)$(CYAN)Installing system-wide to $(PREFIX)/bin...$(RESET)"
	@mkdir -p $(PREFIX)/bin
	@cp $(NAME) $(PREFIX)/bin/
	$(call install_assets,$(PREFIX))
	@echo "$(BOLD)$(GREEN)✓ Installed successfully$(RESET)"
	@$(call print_path_info,$(PREFIX))

install-user:
	@echo "$(BOLD)$(CYAN)Installing for user to $(USER_PREFIX)/bin...$(RESET)"
	@mkdir -p $(USER_PREFIX)/bin
	@cp $(NAME) $(USER_PREFIX)/bin/
	@chmod 755 $(USER_PREFIX)/bin/$(NAME)
	$(call install_assets,$(USER_PREFIX))
	@echo "$(BOLD)$(GREEN)✓ Installed successfully$(RESET)"
	@$(call print_path_info,$(USER_PREFIX))

define print_path_info
	@if echo "$$PATH" | grep -q "$(1)/bin"; then \
		echo "$(BOLD)Run using: $(GREEN)$(NAME)$(RESET)"; \
	else \
		echo "$(BOLD)Run using: $(GREEN)$$(echo $(1)/bin/$(NAME) | sed 's|^$$HOME|~|')$(RESET)"; \
	fi
endef


clean:
	@echo "$(BOLD)$(RED)Removing objects...$(RESET)"
	@rm -rf $(OBJDIR)

fclean: clean
	@echo "$(BOLD)$(RED)Removing $(NAME) and installation receipt...$(RESET)"
	@rm -f $(NAME) .installed

re: fclean all

.PHONY: all clean fclean re install-sys install-user
