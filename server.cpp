// Server side C/C++ program to demonstrate Socket programming
//Source: https://www.geeksforgeeks.org/socket-programming-cc/

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#define PORT 8080

class ClearingServerSocket {
public:
  void setup(sockaddr_in *t_address);
  void loop();
private:
  int server_fd;
  struct sockaddr_in *address;
};

void ClearingServerSocket::setup(sockaddr_in *t_address)
{
  address = t_address;
	int opt = 1;

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
												&opt, sizeof(opt)))
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 8080
	if (bind(server_fd, (struct sockaddr *)address, sizeof(*address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
}

void ClearingServerSocket::loop()
{
  int valread = 0;
  char buffer[1024] = {0};
  char hello[] = "Hello from server";
  int new_socket = 0;
  int addrlen = sizeof(address);

  if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
					(socklen_t*)&addrlen))<0)
	{
		perror("accept");
		exit(EXIT_FAILURE);
	}

  valread = read( new_socket , buffer, 1024);
	printf("%s\n",buffer );
	send(new_socket , hello , strlen(hello) , 0 );
	printf("Hello message sent\n");
}

int main(int argc, char const *argv[])
{
  ClearingServerSocket socket;

  struct sockaddr_in address;
  address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

  socket.setup(&address);
  socket.loop();

  return 0;
}
