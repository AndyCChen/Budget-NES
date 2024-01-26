# files to compile
SRC = main.c src/bus.c src/cartridge.c src/cpu_ram.c src/cpu.c src/log.c

# compiler to use
CC = gcc

# includes paths
INCLUDE_PATHS = -IC:/msys64/ucrt64/bin/../include/SDL2 

# library paths
LIBRARY_PATHS = -LC:/msys64/ucrt64/bin/../lib

# compilier flags
COMPILIER_FLAGS = -Wall -Wextra

# linker flags
LINKER_FLAGS = -lmingw32 -mwindows -lSDL2main -lSDL2

# name of executable
OBJ_NAME = nes

all:
	$(CC) $(SRC) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(LINKER_FLAGS) -o $(OBJ_NAME)