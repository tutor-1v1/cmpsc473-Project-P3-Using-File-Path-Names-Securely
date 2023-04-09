#
# File          : Makefile
# Description   : Build file for CMPSC473 project 3

# Environment Setup
INCLUDES=-I. -I/usr/include/
CC=gcc
CFLAGS=$(INCLUDES) -g -Wall -std=gnu99 -Wno-typedef-redefinition

# Rules
%.o : %.c *.h
	${CC} ${CFLAGS} -c $< -o $@

# Project
TARGET=cmpsc473-p3

$(TARGET) : cmpsc473-p3.o cmpsc473-util.o cmpsc473-list.o cmpsc473-filesys.o cmpsc473-disk.o
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: grade
grade:
	./grade.sh

.PHONY: images
images:
	dot -Tpng imgs/disklayout.dot >imgs/disklayout.png

.PHONY: clean
clean:
	rm -f *.o *~ ._* $(TARGET) $(LIBOBJS)
