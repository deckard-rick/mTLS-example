all: server client

server: server.cpp
	g++ -o bin/server server.cpp -lpthread -L/usr/local/ssl/lib -lssl -lcrypto

client: client.cpp
		g++ -o bin/client client.cpp -lpthread -L/usr/local/ssl/lib -lssl -lcrypto
