CC=gcc
SRCS=main.c game.c snake.c ai.c ui.c fileio.c
OBJS=$(SRCS:.c=.o)
TARGET=snake_xenzia.exe
INC=-I/ucrt64/include/SDL2
LIBS=-L/ucrt64/lib -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_mixer -lm
CFLAGS=-std=c99 -Wall -Wno-unused-parameter -O2 $(INC)
all: $(TARGET)
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LIBS)
	@echo Done
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
clean:
	rm -f $(OBJS) $(TARGET)
