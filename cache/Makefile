CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2 -pthread
# -O2 is required: without optimization the compiler generates naive
# code that does not exhibit realistic cache behavior.

TARGET  = cache_lab

all: $(TARGET)

$(TARGET): cache_lab.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
