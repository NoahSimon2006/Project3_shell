CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu11

tush: tush.c parser.c parser.h
	$(CC) $(CFLAGS) -o tush tush.c parser.c

clean:
	rm -f tush
