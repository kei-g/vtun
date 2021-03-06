CC=clang
CFLAGS=-O3 -Wall -Werror -D_WITH_DPRINTF -march=corei7 -msse4 -maes
LDFLAGS=-Wl,-s
LIBS=-lcrypto
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
	cat rc > /usr/local/etc/rc.d/vtun
	chmod 555 /usr/local/etc/rc.d/vtun
	cp $(TARGETS) /usr/local/sbin/
	rm -f /usr/local/sbin/$(TARGETS)-keygen
	ln /usr/local/sbin/$(TARGETS) /usr/local/sbin/$(TARGETS)-keygen
	make clean
