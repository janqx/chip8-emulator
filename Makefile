TARGET = chip8-emulator

C_SOURCES = $(wildcard *.c)
C_OBJECTS = $(patsubst %.c, %.o, $(C_SOURCES))

CC = gcc -std=c11
RM = rm -rf

SDL2_HOME = C:/Users/Administrator/Desktop/projects/x86_64-w64-mingw32

CFLAGS = -O2 -Wall -I $(SDL2_HOME)/include -L $(SDL2_HOME)/lib -lmingw32 -lSDL2main -lSDL2

all: $(TARGET)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(C_OBJECTS)
	$(CC) $^ $(CFLAGS) -o $@

.PHONY:clean
clean:
	$(RM) $(C_OBJECTS) $(TARGET)
