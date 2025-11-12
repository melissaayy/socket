#include <sys/socket.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <arpa/inet.h> 
#include <unistd.h> 
#include <stdint.h>

#define BACKLOG 10 
#define DEF_CONF_REPLY_SENDER 1

struct stUserCommand
{
	uint16_t i8MessageLen;
	uint16_t i8Offset;
	const char * cpPortNum;
	int i8MsgFormat; 
	int i8ReplyToSender;
}; 

/*
 * Function: Converting numbers in ASCII format to decimal
 * Input: pointer to a string 
 * Output: number in the string but in decimal format 
*/ 
int asciiToDecimal(const char *str) {
    int result = 0;
    char ch;

    while ((ch = *str++) != '\0') {
        if (ch >= '0' && ch <= '9') {
            result = result * 10 + (ch - '0');
        } else {
            // Invalid character for decimal number
            return -1;
        }
    }

    return result;
}

/*
 * Function: Obtaining and processing command line input 
 * Input: command line parameters
 * Output: Configuration for socket and data processing 
*/ 
struct stUserCommand stGetCommandLineInput(char const* argv[])
{
	struct stUserCommand stProcessedUserCommand; 
	char *strPortSlice; 
	// cli param 1: size of message length 
	stProcessedUserCommand.i8MessageLen = (uint16_t) asciiToDecimal(argv[1] + 1);
	//printf("stProcessedUserCommand.i8MessageLen: %d \n", stProcessedUserCommand.i8MessageLen);

	// cli param 2: offset
	stProcessedUserCommand.i8Offset = (uint16_t) asciiToDecimal(argv[2] + 1); 
	//printf("stProcessedUserCommand.i8Offset: %d \n", stProcessedUserCommand.i8Offset);

	// cli param 3: port number 
		// strPortSlice = (char *)(argv[3]+1);  		// this is for receiving more than one port number, need more handling, thinking of using an enum 
		// printf("strPortSlice: %s \n", strPortSlice);
		
		// //char str[] = "6000:1456";
		// char *token = strtok(strPortSlice, ":");
		// while (token != NULL) {
		//     printf("%s\n", token);
		//     token = strtok(NULL, ":");
		// }
	stProcessedUserCommand.cpPortNum = (argv[3] + 1); 
	//printf("stProcessedUserCommand.cpPortNum: %s \n", stProcessedUserCommand.cpPortNum);

	// cli param 4: read message format 
	stProcessedUserCommand.i8MsgFormat = asciiToDecimal(argv[4] + 1); 
	//printf("stProcessedUserCommand.i8MsgFormat: %d \n", stProcessedUserCommand.i8MsgFormat);

	// cli param 5: reply to sender 

	if(argv[5] != NULL)
	{
		stProcessedUserCommand.i8ReplyToSender = asciiToDecimal(argv[5] + 1); 
	}

	else 
	{
		stProcessedUserCommand.i8ReplyToSender = DEF_CONF_REPLY_SENDER; 
	}
	printf("stProcessedUserCommand.i8ReplyToSender: %d \n", stProcessedUserCommand.i8ReplyToSender);

	return stProcessedUserCommand; 
}

/*
 * Function: Obtain IP address (IPv4 or IPv6) 
 * Input: pointer to sockaddr structure  
 * Output: IP address (IPv4 or IPv6) 
*/ 
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
	// get command line configuration
	struct stUserCommand stUserCommandConfig; 
	stUserCommandConfig = stGetCommandLineInput(argv); 

	// handle socket connection
	struct addrinfo hints, *res, *p; 
	int sockfd, new_fd; 
	socklen_t sin_size; 
	struct sockaddr_storage their_addr; 
	char s[INET6_ADDRSTRLEN];
	char* strToSend = "Hello from server! banana \n";
	char strToRecv[stUserCommandConfig.i8MessageLen]; 
	char* strToSendBack; 
	uint16_t numBytesRecv; 
	uint16_t u16NumclientConn = 0; 
	pid_t childpid;

	memset(&hints,0, sizeof(hints)); 
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype =  SOCK_STREAM; 
	hints.ai_flags = AI_PASSIVE; // use my IP

	if(getaddrinfo(NULL, stUserCommandConfig.cpPortNum , &hints, &res) == -1) // this line is giving segentation fault 
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
		u16NumclientConn ++; // id-ing which client has connected 

		if (new_fd == -1) 
		{
			perror("Err: accept"); 
			continue; 
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s); 
		printf("server: got connection from %s\n", s); 
		send(new_fd, strToSend, strlen(strToSend), 0);

		// Child process id
		if((childpid = fork()) == 0)
		{
			printf("server: you are client number %d\n", u16NumclientConn); 

			// closing server socket ID cux the child done need the listening socket 
			close(sockfd); 

			// send confimation message to the client 
			send(new_fd, "hi client, you are now connected \n", strlen("hi client, you are now connected \n"), 0 ); 

			while(1)
			{
				numBytesRecv = recv(new_fd, strToRecv, sizeof(strToRecv)-1, 0); 
				// printf("server: numBytesRecv %d\n", numBytesRecv); 

				if (numBytesRecv == -1)
				{
					perror("Err: recv error \n"); 
					close(new_fd); 
					continue;
				}

				if(numBytesRecv == 0)
				{
					printf("client has disconnected \n"); 
					break; 
				}

				// Null-terminate received string
				strToRecv[numBytesRecv] = '\0'; // this wun work cuz im recv raw hexa value, its not a string 
				//printf("number of bytes recv: %d \n", numBytesRecv); 
				printf("server: received \n%s\n", strToRecv);


				// need to process the string we recv 
				// chop up the string, the first byte will tell us the offset
				// the next 4 bytes will tell us the length of data we would want to extract and process 
				// need to know whether to send the data back as what format 
				// NOTE: need to check how to ensure that the data recv is per expectation, cuz recv many not always be complete 
		

				// Validate bounds
				// if (stUserCommandConfig.i8Offset + stUserCommandConfig.i8MessageLen > numBytesRecv) {
				// 	fprintf(stderr, "Invalid offset/length: out of bounds\n");
				// 	continue;
				// }

				// process the data to be sent back (using the offset and the length)
				// strToSendBack = strToRecv + stUserCommandConfig.i8MessageLen + stUserCommandConfig.i8Offset; 
				// strToSendBack = strToRecv + stUserCommandConfig.i8Offset; 
				// printf("server: send back '%s'\n", strToSendBack);

				// check which sender to send back to 

				
				// send back the recv string to the user 
				// (this is initial implementation to ensure i can send back to user)
				// later on we will send the manipulated data back 
				send(new_fd, strToRecv, strlen(strToRecv), 0 ); 


				// compare received string with "close"
				if (strcmp(strToRecv, "clear") == 0) {
					//printf("server: received close command, shutting down.\n");
					//close(new_fd);
					//close(sockfd);
					////break; // exit the loop and terminate server
					//exit(1); 
					system("clear");  // For Linux/macOS
				}
			}

			close(new_fd);
			printf("server: client number %d disconnected\n", u16NumclientConn); 
			//u16NumclientConn --; 
			exit(0);
			
		}
	}

	return 0; 
}