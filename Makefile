all:
	gcc apps/server.c src/*.c -I include -Wall -Wextra -g -pthread -o bin/server
	gcc apps/client.c src/*.c -I include -Wall -Wextra -g -pthread -o bin/client