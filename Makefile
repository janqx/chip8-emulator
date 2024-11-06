TARGET = chip8.exe

C_SOURCES := $(wildcard *.c)
C_OBJECTS := $(patsubst %.c, %.o, $(C_SOURCES))

CC = gcc -std=c11
RM = rm -rf

CFLAGS = -O2 -Wall -lSDL2main -lSDL2 -lSDL2_mixer

all: $(TARGET)

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(C_OBJECTS)
	$(CC) $^ $(CFLAGS) -o $@

.PHONY:clean
clean:
	$(RM) $(C_OBJECTS) $(TARGET)
