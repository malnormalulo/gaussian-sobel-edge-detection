CC = gcc
CFLAGS = -Wall -Wextra -g -std=c11 -D_GNU_SOURCE

TARGET = main

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c -lm

clean:
	rm -f $(TARGET)
