# Some color codes. Not really gonna use all, but it's neat to have
# anyway.
red := \e[0;31m
RED := \e[1;31m
blue := \e[0;34m
BLUE := \e[1;34m
cyan := \e[0;36m
CYAN := \e[1;36m
green := \e[0;32m
GREEN := \e[1;32m
yellow := \e[0;33m
YELLOW := \e[1;33m
PURPLE := \e[1;35m
purple := \e[0;35m
reset_color := \e[0m


ifeq ($(TARGET),WINDOWS)
	COMPILER ?= i686-w64-mingw32-g++
else
	# DEFAULT 
	COMPILER ?= g++
endif

CPPFLAGS := -O3 -pedantic -Wall -Wextra -Werror -I ./headers -I ./portsock/headers/

main_sources := $(shell ls | grep cpp)
main_targets := $(patsubst %.cpp,%.o,$(main_sources))

# This project depends on portsock, therefore we need to also build portsock
# before linking.
portsock_link := https://github.com/haxala1r/portsock.git

# unnecessary, I know.
ifeq ($(TARGET),WINDOWS)
	target_file ?= BlockChain.exe
else
	target_file ?= BlockChain.elf
endif

.PHONY: build build-portsock clean

build: $(target_file)
	@echo "$(GREEN)Build complete!$(reset_color)"

clean:
	rm -rf portsock
	rm -f $(main_targets)
	rm -f $(target_file)

# Clones and builds portsock.
portsock/libtps.a:
	@echo "$(GREEN)Building portsock$(reset_color)"
	rm -rf portsock
	git clone https://github.com/haxala1r/portsock.git
	cd portsock; TARGET=$(TARGET) make build
	
# Compile everything, build portsock and link everyhing up.
$(target_file): portsock/libtps.a $(main_targets)
	@# Use the compiler to link the source files.
	@# using mingw, windows needs a couple extra flags.
	@# namely: "-lws2_32" to link winsock2, and "-mconsole" to use the
	@# console subsystem.
ifeq ($(TARGET),WINDOWS)
	$(COMPILER) $^ portsock/libtps.a -lws2_32 -mconsole -static-libstdc++ -static-libgcc -o $@
else 
	$(COMPILER) $^ portsock/libtps.a -o $@
endif

# Generic rule for compiling cpp files 
%.o : %.cpp
	@echo "$(CYAN)[BUILD] -> Compiling $^ $(PURPLE)"
	$(COMPILER) $(CPPFLAGS) -c $^ -o $@
