#include <sys/socket.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <arpa/inet.h> 
#include <unistd.h> 
#include <stdint.h>
#include <stdbool.h>

#define BACKLOG 10 
#define DEF_CONF_REPLY_SENDER 1
#define MAX_CLIENT_CONN 255
int i8ClientSockets[MAX_CLIENT_CONN] = {}; 
uint16_t u16NumclientConn = 0; 

struct stUserCommand
{
	uint16_t u16MessageLen;
	uint16_t u16Offset;
	const char * cpPortNum;
	int i8MsgFormat; 
	int i8ReplyToWhichClient;
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
	stProcessedUserCommand.u16MessageLen = (uint16_t) asciiToDecimal(argv[1] + 1);
	//printf("stProcessedUserCommand.u16MessageLen: %d \n", stProcessedUserCommand.u16MessageLen);

	// cli param 2: offset
	stProcessedUserCommand.u16Offset = (uint16_t) asciiToDecimal(argv[2] + 1); 
	//printf("stProcessedUserCommand.u16Offset: %d \n", stProcessedUserCommand.u16Offset);

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
		stProcessedUserCommand.i8ReplyToWhichClient = asciiToDecimal(argv[5] + 1); 
	}

	else 
	{
		stProcessedUserCommand.i8ReplyToWhichClient = DEF_CONF_REPLY_SENDER; 
	}
	printf("stProcessedUserCommand.i8ReplyToWhichClient: %d \n", stProcessedUserCommand.i8ReplyToWhichClient);

	return stProcessedUserCommand; 
}


bool processSendToWhichClient(int u8ClientConf , char * finalToSend)
{
	uint16_t u16BytesSent = 0; 
	bool boSendClientResult = false; 

	if(u8ClientConf == DEF_CONF_REPLY_SENDER)
	{
		u16BytesSent = send(i8ClientSockets[u16NumclientConn-1], finalToSend, strlen(finalToSend), 0 );  
		printf("u16NumclientConn %d \n", u16NumclientConn);
		printf("yayayaa %s \n", finalToSend); 
	}

	else if (u8ClientConf == 2) // send to latest connection
	{
		//u16BytesSent = send(i8ClientSockets[u16NumclientConn-1], finalToSend, strlen(finalToSend), 0 ); // this is the current client (reason is this value holds the latest connection)
		//printf("potato \n");
	}

	else if( (u8ClientConf == 0)) // send to first available client 
	{
		//u16BytesSent = printf("i8ClientSockets[0]: %d \n", i8ClientSockets[0]);
		//send(i8ClientSockets[0], finalToSend, strlen(finalToSend), 0 ); 
	}

	if(u16BytesSent == -1)
	{
		boSendClientResult = false; 
	}

	else
	{
		boSendClientResult = true; 
	}

	return boSendClientResult; 
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

bool boPrint = false; 

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
	char strToRecv[30000]; 
	char* strToSendBack; 
	uint16_t numBytesRecv; 
	uint16_t totalBytesRecv; 
	pid_t tForkPid, proccessPidArr[100];


	fd_set readfds; // select can monitor 3 sets of descriptors 
	int max_fd; 

	char buffer[stUserCommandConfig.u16MessageLen];

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
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) { // check whether this is really needed 
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

	while (1)
	{
		FD_ZERO(&readfds);
		FD_SET(sockfd, &readfds);
		max_fd = sockfd;

		// Add all active client sockets to the fd set
		for (int i = 0; i < MAX_CLIENT_CONN; i++) {
			int temp_fd = i8ClientSockets[i];
			if (temp_fd > 0) {
				FD_SET(temp_fd, &readfds);
				if (temp_fd > max_fd) max_fd = temp_fd;
			}
		}

		// Wait for activity
		int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
		if (activity < 0) {
			perror("Err: select");
			continue;
		}

		// Accept new client
		if (FD_ISSET(sockfd, &readfds)) {
			sin_size = sizeof(their_addr);
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (new_fd == -1) {
				perror("Err: accept");
				continue;
			}

			if (u16NumclientConn < MAX_CLIENT_CONN) {
				i8ClientSockets[u16NumclientConn++] = new_fd;
			} else {
				printf("Max clients reached\n");
				close(new_fd);
			}

			inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
			printf("server: got connection from %s\n", s);
		}

		// Handle data from clients
		for (int i = 0; i < MAX_CLIENT_CONN; i++) 
		{
			int client_fd = i8ClientSockets[i];
			if (client_fd > 0 && FD_ISSET(client_fd, &readfds)) 
			{

				// need to know how much data to peek, also we need to cyclically read data until we 
				// get the length we want, else if we only read once, we are assuming we get the data until
				// the byte containing the offset and the length data (length may not be in the first few bytes of data sent)
				
				// to know how to much bytes we need to read 
				uint16_t u16ExNumBytesHeader = stUserCommandConfig.u16Offset + stUserCommandConfig.u16MessageLen; 
				while (1)
				{
					numBytesRecv = recv(client_fd, strToRecv, sizeof(strToRecv) - 1, MSG_PEEK);
					if (numBytesRecv < u16ExNumBytesHeader)
					{
						continue; // we have recvd the more than the header data we need 
					}

					if (numBytesRecv == -1) 
					{
						perror("Err: recv error");
						close(client_fd);
						i8ClientSockets[i] = 0;
						break;
					}
					if (numBytesRecv == 0) {
						printf("Client disconnected\n");
						close(client_fd);
						i8ClientSockets[i] = 0;
						break;
					}

					// if not enough bytes are recvd, then we will break loop and re-enter when more bytes recvd are
					// detected by select() function 
					break; 
				}
				

				strToRecv[numBytesRecv] = '\0';
				//printf("Peeked: %s\n", strToRecv);

				// Extract expected length
				char cMsgAfterOffset[255], cMsgLenConf[255];
				strcpy(cMsgAfterOffset, strToRecv + stUserCommandConfig.u16Offset);
				strncpy(cMsgLenConf, cMsgAfterOffset, stUserCommandConfig.u16MessageLen);
				cMsgLenConf[stUserCommandConfig.u16MessageLen] = '\0';
				uint16_t u16MsgLenExpected = asciiToDecimal(cMsgLenConf) + stUserCommandConfig.u16MessageLen;

				// Read full data sent 
				uint16_t u16DataByteRecvd = 0;
				while (u16DataByteRecvd < u16MsgLenExpected) {
					int recvd = recv(client_fd, strToRecv + u16DataByteRecvd, u16MsgLenExpected - u16DataByteRecvd, 0);
					if (recvd <= 0)
					{
						break;
					}
					u16DataByteRecvd += recvd;
				}

				if(u16DataByteRecvd < 0)
				{
					printf("Err: data recv error \n");
				}

				strToRecv[u16DataByteRecvd] = '\0';
				printf("Full message: %s\n", strToRecv);

				// Send response
				processSendToWhichClient(stUserCommandConfig.i8ReplyToWhichClient, strToRecv);
			}
		}
	}

	return 0; 
}