#include <sys/socket.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <arpa/inet.h> 
#include <unistd.h> 

#define MYPORT "7000" 
#define BACKLOG 210 

void * get_in_addr(struct sockaddr * sa) 
{
	if(sa -> sa_family == AF_INET) 
	{
		return &(((struct sockaddr_in*)sa)->sin_addr); 
	} 

	else
	{
		return &(((struct sockaddr_in6 *)sa)->sin6_addr); 
	} 
} 


int main(int argc, char const* argv[]) 
{
	struct addrinfo hints, *res, *p; 
	int sockfd, new_fd; 
	socklen_t sin_size; 
	struct sockaddr_storage their_addr; 
	char s[INET6_ADDRSTRLEN];
	char* strToSend = "Hello from server! banana";
	char strToRecv[100]; 
	int numBytesRecv; 

	memset(&hints,0, sizeof(hints)); 
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype =  SOCK_STREAM; 
	hints.ai_flags = AI_PASSIVE; // use my IP

	if(getaddrinfo(NULL, MYPORT, &hints, &res) == -1)
	{
		perror("Err: get address info error \n"); 
	} 


	for (p = res; p != NULL; p = p-> ai_next) 
	{
		sockfd = socket(p -> ai_family, p -> ai_socktype , p -> ai_protocol);
		if(sockfd == -1)
		{ 
			perror("Err: get sock fd error \n"); 
			continue; 
		}
		
		int yes = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if(bind(sockfd, p->ai_addr, p-> ai_addrlen) == -1)
		{
			close(sockfd); 
			perror("Err: sock binding\n"); 
			continue;
		}

		break; 
	}

	freeaddrinfo(res); 

	if (p == NULL) 
	{
		perror("Err: server fail to bind \n");
		exit(1); 
	} 

	if(listen(sockfd, BACKLOG) == -1)
	{
		perror("Err: listen \n"); 
		exit(1); 
	}

	while(1) // main accept() loop
	{
		sin_size = sizeof(their_addr); 
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size); 
		
		if (new_fd == -1) 
		{
			perror("Err: accept"); 
			continue; 
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s); 
		printf("server: got connection from %s\n", s); 
		send(new_fd, strToSend, strlen(strToSend), 0);

		// Child process id
		pid_t childpid;


		while(1)
		{
			numBytesRecv = recv(new_fd, strToRecv, sizeof(strToRecv)-1, 0); 

			if (numBytesRecv == -1)
			{
				perror("Err: recv error \n"); 
				close(new_fd); 
				continue;
			}

			// Null-terminate received string
			strToRecv[numBytesRecv] = '\0';
			printf("number of bytes recv: %d \n", numBytesRecv); 
			printf("server: received '%s'\n", strToRecv);

			// Compare received string with "close"
			if (strcmp(strToRecv, "close") == 0) {
				printf("server: received close command, shutting down.\n");
				close(new_fd);
				close(sockfd);
				//break; // Exit the loop and terminate server
				exit(1); 
			}
		}
	}

	return 0; 
}



