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


bool processSendToWhichClient(int u8ClientConf , char * finalToSend, int client_fd)
{
	uint16_t u16BytesSent = 0; 
	bool boSendClientResult = false; 

	if(u8ClientConf == DEF_CONF_REPLY_SENDER)
	{
		u16BytesSent = send(client_fd, finalToSend, strlen(finalToSend), 0 );  
		// printf("u16NumclientConn %d \n", u16NumclientConn);
		// printf("yayayaa %s \n", finalToSend); 
	}

	else if (u8ClientConf == 2) // send to latest connection
	{
		u16BytesSent = send(i8ClientSockets[u16NumclientConn-1], finalToSend, strlen(finalToSend), 0 ); // this is the current client (reason is this value holds the latest connection)
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


	fd_set readfds, masterfds; // select can monitor 3 sets of descriptors 
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

	FD_ZERO(&readfds); // master fd set 
	FD_ZERO(&masterfds); // backup fd set, we need this cus the select() func will change the master fd set
	FD_SET(sockfd, &masterfds); // we are setting the first element of the master fd set to be our listening sockfd 
	max_fd = sockfd; // so far since we dont have any client connection, we know that the listeninf sockfd is highest fd value

	// printf("newfd: %d \n", new_fd);
	// printf("maxfd: %d \n", max_fd);
	// printf("sockfd: %d \n", sockfd); 

	// now we want to process incoming connections and the data recv from connected clients 
	while (1)
	{
		// copy the master fd set to the working fd set  
		readfds = masterfds; 

		// run select function, this function will monitor several sockets at the same time 
		// then it will tell us which of the sockets are ready for reading/ writing/ have exceptions 
		int activity = select(max_fd+1, &readfds, NULL, NULL, NULL); // the last 3 parameter is false as we dont monitor for write, expection
																// and dont want to set any timeout, now is wait infinitely 
		
		// printf("new activity detected: %d \n", activity);

		if(activity == -1)
		{
			perror("Err: select error \n"); 
			exit(1); // meas we will exit the program 
		}

		// now we have an update set of fd, which includes the server's sockfd and the clients fd 
		// we will want to process them to know whether any new connection or existing connections 
		// have any activity, we use the FD_ISSET to check, as long as any is set means there is actions 

		for(int i = 0; i <= max_fd; i++)
		{
			//printf("i = %d \n", i);
			// check if there are any activity 
			if(FD_ISSET(i, &readfds)) 
			{
				// printf("hi \n");

				// check if the activity is the listening sockfd 
				if (i == sockfd)
				{
					// accept the new client connection
					new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

					if(new_fd == -1)
					{
						perror("Err: accept error \n");
						continue; 
					}

					// save the new client's fd to the set 
					FD_SET(new_fd, &masterfds); 
					if(new_fd > max_fd) // keep track of the max fd in the set 
					{
						max_fd = new_fd; 
					}

					// printf("newfd: %d \n", new_fd);
					// printf("maxfd: %d \n", max_fd);
					// printf("sockfd: %d \n", sockfd); 

					inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
					printf("server: got connection from %s\n", s);

					// add in the new fd for the client into our array of client fds 
					if(u16NumclientConn < MAX_CLIENT_CONN)
					{
						i8ClientSockets[u16NumclientConn] = new_fd; 
						u16NumclientConn++; // increment our client number count 
						printf("ok i hv accepted and has set the client fd to the set \n"); 
					}

					else
					{
						printf("Err: maximum number of clients reached \n"); 
						close(new_fd); // we close the connection since we already hit our maximum 
					}

					// printf("ok im done with setting the client fd to the set \n"); 
				}
			
				// if there is activity but not the server's socket's fd
				// this means that our connected client have information for us 
				else
				{
					// printf("ok recv data from client \n"); 

					// we will recv the information from the client 
					numBytesRecv = recv(i, strToRecv, sizeof(strToRecv), MSG_PEEK); 
					
					if(numBytesRecv == -1) // error occured 
					{
						perror("Err: recv error and will close this client's connection \n");
						close(i); 
						// clear this client's fd from the set 
						FD_CLR(i, &masterfds);
					}

					if (numBytesRecv == 0) // connection closed by client 
					{
						printf("Client disconnected, will proceed to close the fd for this client \n"); 
						close(i); 
						// clear this client's fd from the set 
						FD_CLR(i, &masterfds);
					}

					// printf("recv and no error for now \n");

					// processing the number of bytes to read 
					char cMsgAfterOffset[255], cMsgLenConf[255];
					strcpy(cMsgAfterOffset, strToRecv + stUserCommandConfig.u16Offset);
					strncpy(cMsgLenConf, cMsgAfterOffset, stUserCommandConfig.u16MessageLen);
					cMsgLenConf[stUserCommandConfig.u16MessageLen] = '\0';
					uint16_t u16MsgLenExpected = asciiToDecimal(cMsgLenConf) + stUserCommandConfig.u16MessageLen;

					uint16_t u16ExNumBytesHeader = stUserCommandConfig.u16Offset + stUserCommandConfig.u16MessageLen; 

					// Read full data sent 
					uint16_t u16DataByteRecvd = 0;
					while (u16DataByteRecvd < u16MsgLenExpected) 
					{
						select(i+1, &readfds, NULL, NULL, NULL); 

						int recvd = recv(i, strToRecv + u16DataByteRecvd, u16MsgLenExpected - u16DataByteRecvd, 0);
						if (recvd <= 0)
						{
							break;
						}
						u16DataByteRecvd += recvd;
						printf("recvd: %d \n", recvd); 
						printf("u16DataByteRecvd: %d \n", u16DataByteRecvd); 
						printf("u16MsgLenExpected: %d \n", u16MsgLenExpected); 
						//printf("im inside the bytes extraction loop \n");
					}

					if(u16DataByteRecvd < 0)
					{
						printf("Err: data recv error \n");
					}

					// printf("extracted the data based on the length and the offset ooo \n");
					strToRecv[u16DataByteRecvd] = '\0';
					printf("Full message: %s\n", strToRecv);

					// Send response
					processSendToWhichClient(stUserCommandConfig.i8ReplyToWhichClient, strToRecv, i);

				}
			}
		}
	}

	return 0; 
}