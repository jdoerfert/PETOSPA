PROGS     = main
CFLAGS   += -Wall -pedantic
LDFLAGS  +=
CPPFLAGS +=
FLAGS    += ${CFLAGS} ${LDFLAGS} ${CPPFLAGS}

all: ${PROGS}

${PROGS}: % : %.c
	${CC} ${FLAGS} -c $<
	${CC} ${FLAGS} -o a.out *.o

clean:
	${RM} *.o core ${PROGS}
