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

int i8ClientSockets[255] = {0}; 
uint16_t u16NumclientConn = 0; 

struct stUserCommand
{
	uint16_t i8MessageLen;
	uint16_t i8Offset;
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
		u16BytesSent = send(i8ClientSockets[u16NumclientConn], finalToSend, strlen(finalToSend), 0 );  
		//printf("u16NumclientConn %d \n", u16NumclientConn);
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

	char buffer[stUserCommandConfig.i8MessageLen];

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

	while(1) // main loop
	{
		// setting up the select() function which will help us determine which client has sent data to the server 
		FD_ZERO(&readfds);         // clears the readfds set, a set is used by select() to keep track of file descriptors 
		FD_SET(sockfd, &readfds);  // adding the sockfd file descriptor to the sockfd set, this is adding the listening socket 
		max_fd = sockfd; 

		// then we run select (waiting for activity)
		int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

		if (activity < 0) 
		{
			perror("Err: select");
            continue;
		}

		// then check which FDs are ready 
		// if server's FD is ready then can be ready to accept client connection 
		if (FD_ISSET(sockfd, &readfds))
		{
			sin_size = sizeof(their_addr); 
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size); // accept() is blocking, means that if no conn req, 
																					// the below codes will not cont execution 
			if (new_fd == -1) 
			{
				perror("Err: accept"); 
				continue; 
			}

			// increment the number of clients 
			u16NumclientConn++;

			i8ClientSockets[u16NumclientConn] = new_fd;

			inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s); 
			printf("server: got connection from %s\n", s); 
			printf("server: you are client number %d\n", u16NumclientConn); 

			// process data recv from client 
			while(1) 
			{
				numBytesRecv = recv(new_fd, strToRecv, sizeof(strToRecv)-1, MSG_PEEK); 

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

				strToRecv[numBytesRecv] = '\0'; // Null-terminate received string
				// printf("server: received \n%s\n", strToRecv);
				if(!boPrint)
				{
					//printf("server: received \n%s\n", strToRecv);
					//boPrint = true; 
				}


				// process the data based on the L and O from CLI
				// to know where is the length information, we would need to use the offset 
				char cMsgAfterOffset[255];
				char cMsgLenConf[255]; 
			
				//printf("stUserCommandConfig.i8MessageLen: %d \n", stUserCommandConfig.i8MessageLen);
				strcpy(cMsgAfterOffset, (strToRecv+stUserCommandConfig.i8Offset));

				if(!boPrint)
				{
					//printf("cMsgAfterOffset = strToRecv[i8Offset]: %s \n", cMsgAfterOffset); 
					// printf("%d \n", (stUserCommandConfig.i8MessageLen+1));
					//boPrint = true; 
				}

				strncpy(cMsgLenConf, cMsgAfterOffset, (stUserCommandConfig.i8MessageLen)); 
				cMsgLenConf[stUserCommandConfig.i8MessageLen] = '\0';
				if(!boPrint)
				{
					// printf("cMsgLenConf: %s huehuehue \n", cMsgLenConf); 
					// printf("size of cMsgLenConf: %s \n", cMsgLenConf); 
					//boPrint = true; 
				}
				
				// convert cMsgLenConf from ascii to decimal 
				uint16_t u16MsgLenExpected =  0;
				u16MsgLenExpected =(uint16_t) asciiToDecimal(cMsgLenConf) + stUserCommandConfig.i8MessageLen; // for now we assume that the length config in the data send 
																					 						  // is pure data length, means it dun include length of the lenght config 
				if(!boPrint)
				{				
					// printf("size of u16MsgLenExpected: %d \n", u16MsgLenExpected); 
				}

				//printf("u16MsgLenExpected: %d \n", u16MsgLenExpected); 
				//u16MsgLenExpected = u16MsgLenExpected + (uint16_t)stUserCommandConfig.i8MessageLen + (uint16_t)stUserCommandConfig.i8Offset; 

				if(!boPrint)
				{
					// printf("u16MsgLenExpected: %d \n", u16MsgLenExpected); 
					//printf("numBytesRecv: %d \n", numBytesRecv); 

					//boPrint = true; 
				}
				
				// check whether the recv length matches the expected length 

				// while(1)
				// {
					while(totalBytesRecv < u16MsgLenExpected)
					{
						numBytesRecv = recv(new_fd, strToRecv, sizeof(strToRecv)-1, 0); 
						printf("u16Msg expected %d \n", u16MsgLenExpected); 
						printf("u16MsgLenRead: %d \n", totalBytesRecv); 

						totalBytesRecv += numBytesRecv; 
						if(!boPrint)
						{
							printf("retry recv function \n"); 
							//boPrint = true; 
						}
					// }

					if(!boPrint)
					{
						printf("final numBytesRecv: %d \n", totalBytesRecv); 
						printf("success!\n");
						boPrint = true; 
					}

					processSendToWhichClient(stUserCommandConfig.i8ReplyToWhichClient, strToRecv);
				}
			}
		}
	}

	return 0; 
}