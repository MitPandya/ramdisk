CC := gcc
CFLAGS := -g -Wall
OBJS := $(patsubst %.c, %.o, $(wildcard *.c))
 
TARGET := ramdisk
 
.PHONY: all clean
 
all: $(TARGET)
 
$(TARGET): $(OBJS)
	    $(CC) $(CFLAGS) `pkg-config --libs fuse` -o $@ $^
 
.c.o:
	    $(CC) $(CFLAGS) `pkg-config --cflags fuse` -c $<
 
clean:
	    rm -f $(TARGET) $(OBJS)