.PHONY: all clean

all: server client
server: server.c
	gcc -Wall -o server server.c utils/base64.c -lpthread
client: client.c
	gcc -Wall -o client client.c utils/base64.c -lpthread
clean:
	@rm -rf server client 
