all: 
	gcc server.c -Wall -Wextra -g -o server
	gcc client.c -Wall -Wextra -g -o client
