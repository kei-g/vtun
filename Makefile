CC=clang
CFLAGS=-D _WITH_DPRINTF -O3 -Wall -Werror -march=native
LD=clang
LDFLAGS=-Wl,-s
LIBS=-lssl
TARGETS=vtun
OBJS=base64.o \
	client.o \
	codec.o \
	conf.o \
	ioctl.o \
	server.o \
	sig.o \
	vtun.o \
	xfer.o

all: $(TARGETS)

$(TARGETS): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

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
