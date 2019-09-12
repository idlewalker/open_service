CC ?= gcc
CFLAGS += -I.\
		  -I../include\
		  -W -Wall -g

LDFLAGS = -lpthread

SOBJS = server.o common.o
COBJS = client.o common.o

STARGET = raven_helper
CTARGET = service_load

${STARGET} : ${SOBJS}
	${CC} $^ -o $@ ${LDFLAGS} -Ofast

${CTARGET} : ${COBJS}
	${CC} $^ -o $@ ${LDFLAGS} -Ofast

%.o : %.c
	${CC} -c $< ${CFLAGS} -Ofast

all: ${STARGET} ${CTARGET}
	ctags -R *

clean:
	rm -fr *.o ${STARGET} ${CTARGET} tags

.PHONY: clean
