SHELL = /bin/sh

SOURCE =  main.cpp Cache.cpp
CFLAG = -Wall -g
CC = g++
INCLUDES = -I. 
LIBS = -lm
EXECUTABLE=cachesim

${EXECUTABLE}:${SOURCE}
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${SOURCE} ${LIBS}

clean:
	-rm -f *.o $(EXECUTABLE)
