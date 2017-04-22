all : ping-request ping-reply

ping-request : ping-request.c
	gcc -o $@ $^
ping-reply : ping-reply.c
	gcc -o $@ $^

clean:
	rm -rf ping-request ping-reply
