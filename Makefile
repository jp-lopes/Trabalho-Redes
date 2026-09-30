all: 
	gcc src/server.c -Wall -Wextra -g -I include -pthread -o bin/server
	gcc src/client.c -Wall -Wextra -g -I include -pthread -o bin/client
