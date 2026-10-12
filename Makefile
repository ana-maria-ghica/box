CC = gcc 
CFLAGS = -std=c11 -Wall -Wextra -Werror

.PHONY: all clean

SOURCES = src/main.c src/cli.c src/container.c src/child.c src/mounts.c
OBJS = $(SOURCES:.c=.o)

all: box

box: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o box

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ 

clean:
	rm -f box src/*.o