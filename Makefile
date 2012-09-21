SHELL = /bin/sh

SOURCE =  main.cpp Cache.cpp
INC = Cache.h
CFLAGS = -Wall -g
CC = g++
INCLUDES = -I. 
LIBS = -lm
EXECUTABLE=cachesim

${EXECUTABLE}:${SOURCE} ${INC} autotest.cpp singlecache.cpp
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${SOURCE} ${LIBS}
	${CC} ${CFLAGS} ${INCLUDES} -o autotest autotest.cpp ${LIBS}
	${CC} ${CFLAGS} ${INCLUDES} -o singlecache singlecache.cpp Cache.cpp ${LIBS}

clean:
	-rm -f *.o $(EXECUTABLE) autotest singlecache
