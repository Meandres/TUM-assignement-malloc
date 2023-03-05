OBJS= main.o m_alloc.o
CFLAGS=-g
CC=gcc

main: ${OBJS}
	${CC} ${CFLAGS} -o $@ ${OBJS}

clean:
	rm -f *.o main

