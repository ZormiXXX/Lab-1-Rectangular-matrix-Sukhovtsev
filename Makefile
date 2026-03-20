CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2
LDFLAGS := -lm

TARGET  := lr1
SRCS    := main.c ui.c algebra.c matrix.c tests.c
OBJS    := $(SRCS:.c=.o)

.PHONY: all run test clean rebuild

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

rebuild: clean all