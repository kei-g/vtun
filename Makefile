CC=clang
CFLAGS=-O3 -Wall -Werror -DDEBUG
LDFLAGS=-Wl,-s
LIBS=-lssl
TARGETS=client server
OBJS=conf.o vtun.o

all: $(TARGETS:%=vtun_%)

vtun_client: client.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ client.o $(OBJS) $(LIBS)

vtun_server: server.o vtun.o
	$(CC) $(LDFLAGS) -o $@ server.o $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGETS:%=vtun_%) $(TARGETS:%=%.o) $(OBJS)
