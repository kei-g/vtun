CC=clang
CFLAGS=-O3 -Wall -Werror -DDEBUG
LDFLAGS=-Wl,-s
LIBS=-lssl
TARGETS=vtun
OBJS=client.o conf.o ioctl.o server.o vtun.o

all: $(TARGETS)

$(TARGETS): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGETS) $(OBJS)
