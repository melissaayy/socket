#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h> 
#include <unistd.h> 
#include <netdb.h>

#define MYPORT "7000"
//#define BACKLOG 10 
#define MAXDATASIZE 255

void * get_in_addr(struct sockaddr *sa)
{
	if(sa -> sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr); 
	}

	else 
	{
		return &(((struct sockaddr_in6*)sa)->sin6_addr); 
	}
}

int main(int argc, char const* argv[])
{
	struct addrinfo hints, *res, *p; 
	int sRetVal = 0; 
	char buf[MAXDATASIZE]; 
	char s[INET6_ADDRSTRLEN];
	int numbytes; 
	struct sockaddr_storage peer_addr; 
	socklen_t peer_addr_len = sizeof(peer_addr);
	int sockfd; 
	char* strToSend = "bellow huhu client side here";

	memset(&hints, 0, sizeof(hints)); 
	hints.ai_family = AF_UNSPEC; 
	hints.ai_flags = AI_PASSIVE; 
	hints.ai_socktype = SOCK_STREAM; 

	sRetVal = getaddrinfo(NULL, MYPORT, &hints, &res); 
	if (sRetVal == -1)
	{
		printf("Error: get address info! \n"); 
	}


	// loop through the linked list we obtained 
	for(p = res; p!=NULL; p = p-> ai_next)
	{
		// check for socket description
		sockfd = socket(res->ai_family, res->ai_socktype, res-> ai_protocol); 
		if (sockfd == -1)
		{
			printf("Error: get socket description info! \n"); 
			continue; 
		}

		// check for connection 
		sRetVal = connect(sockfd, p-> ai_addr, p-> ai_addrlen); 
		if (sRetVal == -1)
		{
			close(sockfd); // close client socket since no connection available 
			continue;
			printf("Error: no connection! \n"); 
		} 

		break; 
	}

	if(p == NULL)
	{
		printf("Error: failed to establish connection! \n"); 
	}



	//inet_ntop(p -> ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
	//inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s); 

	getpeername(sockfd, (struct sockaddr *)&peer_addr, &peer_addr_len);
	inet_ntop(peer_addr.ss_family, get_in_addr((struct sockaddr *)&peer_addr), s, sizeof s);

	printf("client: connecting to %s \n", s); 

	freeaddrinfo(res); // since we are done with the connection, we can free the linked list space 

	if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) 
	{
		perror("Error: recv error"); 
	}

	buf[numbytes] = '\0'; 

	printf("client: received '%s' \n", buf); 

	send(sockfd, strToSend, strlen(strToSend), 0);

	close(sockfd); 

	return 0; 
}
