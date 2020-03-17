# Makefile file.
# Adding files we would like to compile.
OBJS = *.cpp

# Name of desired executable
OBJ_NAME = v4l-simple

# Desired compiler
CC = g++ -O3

# Compiler flags I would like to add!
COMPILER_FLAGS = -w `pkg-config --cflags libjpeg` `pkg-config --cflags opencv4`

# Libraries we are linking against.
LINKER_FLAGS = `pkg-config --libs libjpeg` `pkg-config --libs opencv4`

all: $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)

install:

