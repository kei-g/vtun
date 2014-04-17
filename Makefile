CC=clang
CFLAGS=-O3 -Wall -Werror -DDEBUG
LDFLAGS=-Wl,-s
LIBS=-lssl
TARGETS=client server

all: $(TARGETS:%=vtun_%)

vtun_client: client.o vtun.o
	$(CC) $(LDFLAGS) -o $@ client.o vtun.o $(LIBS)

vtun_server: server.o vtun.o
	$(CC) $(LDFLAGS) -o $@ server.o vtun.o $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGETS:%=vtun_%) $(TARGETS:%=%.o) vtun.o
