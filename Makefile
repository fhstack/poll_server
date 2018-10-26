all:poll_server

poll_server:poll_server.c
	gcc -o $@ $^

.PHONY:clean

clean:
	rm -f poll_server
