# Makefile file.
# Adding files we would like to compile.
OBJS = *.c

# Name of desired executable
OBJ_NAME = v4l-sdl2

# Desired compiler
CC = gcc

# Compiler flags I would like to add!
COMPILER_FLAGS = -w

# Libraries we are linking against.
LINKER_FLAGS = -lSDL2 -lSDL2_image

all: $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)

install:

