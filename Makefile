CC=clang
CFLAGS=-O3 -Wall -Werror -march=corei7 -msse4 -maes
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
	ln /usr/local/sbin/$(TARGETS) /usr/local/sbin/$(TARGETS)-keygen
	make clean
