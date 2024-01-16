# files to compile
SRC = main.c

# compiler to use
CC = gcc

# includes paths
INCLUDE_PATHS = -IC:\Emu_Dev\SDL\include\SDL2

# library paths
LIBRARY_PATHS = -LC:\Emu_Dev\SDL\lib

# compilier flags
COMPILIER_FLAGS = -Wall -Wextra

# linker flags
LINKER_FLAGS = -lmingw32 -lSDL2main -lSDL2

# name of executable
OBJ_NAME = nes

all:
	$(CC) $(SRC) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILIER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)