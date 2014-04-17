all:
	clang -DDEBUG -Wl,-s -O3 -Wall -Werror -o vtun-client -lssl client.c
	clang -DDEBUG -Wl,-s -O3 -Wall -Werror -o vtun-server -lssl server.c

clean:
	rm -f vtun-client vtun-server
