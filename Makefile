CC=clang
CFLAGS=-O3 -Wall -Werror
LDFLAGS=-Wl,-s
LIBS=-lssl
TARGETS=vtun
OBJS=3des.o \
	base64.o \
	client.o \
	conf.o \
	ioctl.o \
	server.o \
	sig.o \
	vtun.o \
	xfer.o

all: $(TARGETS)

$(TARGETS): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGETS) $(OBJS)

install: $(TARGETS)
	cp $(TARGETS) /usr/local/sbin/
	make clean
