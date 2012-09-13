SHELL = /bin/sh

SOURCE =  main.cpp Cache.cpp
INC = Cache.h
CFLAG = -Wall -g
CC = g++
INCLUDES = -I. 
LIBS = -lm
EXECUTABLE=cachesim

${EXECUTABLE}:${SOURCE} ${INC}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${SOURCE} ${LIBS}

clean:
	-rm -f *.o $(EXECUTABLE)
