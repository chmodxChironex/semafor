CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
LFLAGS = -pthread -lrt
EXECUTABLE = proj2
DEPENDENCIES = proj2.h
SOURCE = proj2.c
OBJECTS = $(SOURCE:.c=.o)

.PHONY: all clean zipit

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ $(LFLAGS)

$(OBJECTS): $(SOURCE) $(DEPENDENCIES)
	$(CC) $(CFLAGS) -c $(SOURCE) -o $@ $(LFLAGS)

clean:
	rm -f $(EXECUTABLE)
	rm -f $(OBJECTS)
	rm -f proj2.out
	rm -f proj2.zip

zipit:
	zip proj2.zip $(SOURCE) $(DEPENDENCIES) Makefile
