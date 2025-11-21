#include <sys/socket.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <netdb.h>
#include <arpa/inet.h> 
#include <unistd.h> 
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#define BACKLOG 10 
#define DEF_CONF_REPLY_SENDER 1
#define MAX_CLIENT_CONN 255
#define MAX_FILE_LOG 2 //9999

int i8ClientSockets[MAX_CLIENT_CONN]; 
uint16_t u16NumclientConn = 0; 
// fd_set masterfds; 

struct stUserCommand
{
	uint16_t u16MessageLen;
	uint16_t u16Offset;
	const char * cpPortNum;
	int i8MsgFormat; 
	int i8ReplyToWhichClient;
}; 

struct stCreatedSocket
{
	int iSocketFd;
	//struct sockaddr_storage stClientAddr; 
	int iListenResult;
};


void vLogErrToFile(FILE *fp, const char* msg)
{
	fprintf(fp, "Err -> %s: %s \n", msg, strerror(errno)); 
	fflush(fp); 
}

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
struct stUserCommand stGetCommandLineInput(char const* argv[], FILE * filePtr)
{
	struct stUserCommand stProcessedUserCommand; 
	char *strPortSlice; 
	// cli param 1: size of message length 
	stProcessedUserCommand.u16MessageLen = (uint16_t) asciiToDecimal(argv[1] + 1);
	//printf("stProcessedUserCommand.u16MessageLen: %d \n", stProcessedUserCommand.u16MessageLen);
	fprintf(filePtr, "stProcessedUserCommand.u16MessageLen: %d \n", stProcessedUserCommand.u16MessageLen);

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
	fprintf(filePtr, "stProcessedUserCommand.cpPortNum: %s \n", stProcessedUserCommand.cpPortNum);

	// cli param 4: read message format 
	stProcessedUserCommand.i8MsgFormat = asciiToDecimal(argv[4] + 1); 
	//printf("stProcessedUserCommand.i8MsgFormat: %d \n", stProcessedUserCommand.i8MsgFormat);
	fprintf(filePtr, "stProcessedUserCommand.i8MsgFormat: %d \n", stProcessedUserCommand.i8MsgFormat);

	// cli param 5: reply to sender 

	if(argv[5] != NULL)
	{
		stProcessedUserCommand.i8ReplyToWhichClient = asciiToDecimal(argv[5] + 1); 
	}

	else 
	{
		stProcessedUserCommand.i8ReplyToWhichClient = DEF_CONF_REPLY_SENDER; 
	}
	//printf("stProcessedUserCommand.i8ReplyToWhichClient: %d \n", stProcessedUserCommand.i8ReplyToWhichClient);
	fprintf(filePtr, "stProcessedUserCommand.i8ReplyToWhichClient: %d \n", stProcessedUserCommand.i8ReplyToWhichClient);

	fflush(filePtr);
	return stProcessedUserCommand; 
}

/*
 * Function: Process which client to send data to based on the CLI parameter 
 * Input: Command line parameters
 * Output: Data sent to choosen client  
*/ 
bool processSendToWhichClient(int u8ClientConf , char * finalToSend, int client_fd, uint16_t u16ExLentoSend, int max_fd, FILE * filePtr)
{
	uint16_t u16BytesSent = 0; 
	uint16_t u16TtlBytesSent = 0; 
	bool boSendClientResult = false; 

	if(u8ClientConf == DEF_CONF_REPLY_SENDER)
	{
		client_fd = client_fd; 
		// printf("u16NumclientConn %d \n", u16NumclientConn);
		// printf("yayayaa %s \n", finalToSend); 
	}

	else if (u8ClientConf == 2) // send to latest connection
	{
		//client_fd = i8ClientSockets[u16NumclientConn-1];
		//printf("u16NumclientConn-1: %d \n", u16NumclientConn-1); 
		// if (client_fd == -1)
		// {					
		for (uint16_t j = (MAX_CLIENT_CONN - 1) ; j >= 0 ; j--)  // this we start from the end of the loop until we find a non -1 value then we break the loop 
		{
			if (i8ClientSockets[j] != -1) 
			{
				client_fd = i8ClientSockets[j];
				break; 
			}
		}

		// }
		// client_fd = max_fd; 
		//u16BytesSent = send(i8ClientSockets[u16NumclientConn-1], finalToSend, strlen(finalToSend), 0 ); // this is the current client (reason is this value holds the latest connection)
		//printf("potato \n");
	}

	else if( (u8ClientConf == 0)) // send to first available client 
	{

		// loop through the client fds array and find the first non negative 1 value 
		for(int i = 0; i < MAX_CLIENT_CONN; i++)
		{
			// not -1 means that there is a valid client fd 
			if(i8ClientSockets[i] != -1)
			{
				client_fd = i8ClientSockets[i];
				break; 
			}	
		}

		// client_fd = i8ClientSockets[0];
		// client_fd = masterfds[1];
		//u16BytesSent = send(i8ClientSockets[0], finalToSend, strlen(finalToSend), 0 );
		//printf("i8ClientSockets[0]: %d \n", i8ClientSockets[0]);
	}

	// cyclically send until the expected number to send is reached 
	while (u16TtlBytesSent < u16ExLentoSend)
	{
		u16BytesSent = send(client_fd, finalToSend, strlen(finalToSend), 0 );  
		if(u16BytesSent == -1)
		{
			//perror("data sent to client failed");
			// fprintf(filePtr, "data sent to client failed \n");
			vLogErrToFile(filePtr, "Data sent to client failed "); 
			boSendClientResult = false; 
			break; 
		}

		u16TtlBytesSent += u16BytesSent; 
		// printf("u16BytesSent %d \n", u16BytesSent); 
		// printf("u16TtlBytesSent %d \n", u16TtlBytesSent); 
		// printf("u16ExLentoSend %d \n", u16ExLentoSend); 
		fprintf(filePtr, "u16TtlBytesSent: %d \n", u16TtlBytesSent);

	}

	if(u16BytesSent == -1)
	{
		boSendClientResult = false; 
	}

	else
	{
		boSendClientResult = true; 
	}

	fflush(filePtr);
	return boSendClientResult; 
}

/*
 * Function: Setup server socket, bind and start listening 
 * Input: CLI parameters  
 * Output: Listening port result 
*/ 
struct stCreatedSocket iSetupListeningSocket(const char * cpPortNum, uint16_t u16MessageLen, FILE * filePtr) // struct sockaddr_storage * pClientAddr)
{
	// initialise the array used to store all the client's Fd 
	for (int i = 0; i < MAX_CLIENT_CONN; i++)
	{
		i8ClientSockets[i] = -1; 
	}
	
	struct stCreatedSocket stServerSocket; 

	// handle socket connection
	struct addrinfo hints, *res, *p; 
	int sockfd, new_fd; 
	//struct sockaddr_storage their_addr; 
	//socklen_t sin_size = sizeof(stServerSock.stClientAddr); 
	char s[INET6_ADDRSTRLEN];
	char* strToSend = "Hello from server! banana \n";
	char* strToSendBack; 
	uint16_t totalBytesRecv; 
	pid_t tForkPid, proccessPidArr[100];
	char buffer[u16MessageLen];

	memset(&hints,0, sizeof(hints)); 
	hints.ai_family = AF_UNSPEC; 
	hints.ai_socktype =  SOCK_STREAM; 
	hints.ai_flags = AI_PASSIVE; // use my IP

	if(getaddrinfo(NULL, cpPortNum , &hints, &res) == -1)  
	{
		//perror("Err: get address info error \n"); 
		//fprintf(filePtr, "Err: get address info error \n"); 
		vLogErrToFile(filePtr, "Get address info error "); 
	} 

	for (p = res; p != NULL; p = p-> ai_next) 
	{
		sockfd = socket(p -> ai_family, p -> ai_socktype , p -> ai_protocol); // ai_protocol is 0 bcuz we used AF_UNSPEC means can either be IPv4 or IPv6 and we will let it decide by itself 
		stServerSocket.iSocketFd = sockfd;
		if(sockfd == -1)
		{ 
			//perror("Err: get sock fd error \n"); 
			// fprintf(filePtr, "Err: get sock fd error \n"); 
			vLogErrToFile(filePtr, "Get sock fd error "); 
			continue; 
		}
		
		// this is used to enable us to reuse the port while the system is still waiting for the port after a connection is closed 
		// TCP sockets go through a TIME_WAIT state after closing.
		// This is part of TCP’s design to ensure all packets in flight are properly handled and avoid confusion with new connections.
		// During TIME_WAIT, the port is not fully reusable unless you set special options.
		int yes = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
		{  
			//perror("Err: setsockopt");
			// fprintf(filePtr, "Err: setsockopt \n"); 
			vLogErrToFile(filePtr, "Setsockopt error when freeing the port"); 			
			exit(1);
		}

		if(bind(sockfd, p->ai_addr, p-> ai_addrlen) == -1)
		{
			close(sockfd); 
			//perror("Err: sock binding\n"); 
			// fprintf(filePtr, "Err: sock binding\n"); 
			vLogErrToFile(filePtr, "Socket port binding error"); 			
			continue;
		}

		break; 
	}

	freeaddrinfo(res); 

	if (p == NULL) 
	{
		//perror("Err: server fail to bind \n");
		//fprintf(filePtr, "Err: server fail to bind \n"); 
		vLogErrToFile(filePtr, "No socket created"); 			
		exit(1); 
	} 

	stServerSocket.iListenResult = listen(sockfd, BACKLOG); 

	return stServerSocket; 
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
	int fileNum = 1; 

	// save the messages into a file 
	char fileNameLog[100] = "hostsimlog"; 
	FILE *fpDataLog = fopen(fileNameLog, "w"); 
	if(!fpDataLog)
	{
		//perror("fopen fail \n");
		//fprintf(fpDataLog, "fopen fail \n"); 
		vLogErrToFile(fpDataLog, "Failed to open log file"); 			
	}

	// get command line configuration
	struct stUserCommand stUserCommandConfig; 
	stUserCommandConfig = stGetCommandLineInput(argv, fpDataLog); 
	char s[INET6_ADDRSTRLEN];

	int new_fd; 
	struct stCreatedSocket stServerSock = iSetupListeningSocket(stUserCommandConfig.cpPortNum, stUserCommandConfig.u16MessageLen, fpDataLog);
	struct sockaddr_storage their_addr; 
	socklen_t sin_size = sizeof(their_addr); 

	if(stServerSock.iListenResult == -1)
	{
		//perror("Err: listen \n"); 
		// fprintf(fpDataLog, "Err: listen \n"); 
		vLogErrToFile(fpDataLog, "Listening socket error"); 			
		exit(1); 
	}

	fd_set readfds, masterfds; 
	int max_fd; 

	FD_ZERO(&readfds); // master fd set 
	FD_ZERO(&masterfds); // backup fd set, we need this cus the select() func will change the master fd set
	FD_SET(stServerSock.iSocketFd, &masterfds); // we are setting the first element of the master fd set to be our listening sockfd (Note: max element for FD_SET is 1024 )
	max_fd = stServerSock.iSocketFd; // so far since we dont have any client connection, we know that the listeninf sockfd is highest fd value

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
		int activity = select(max_fd+1, &readfds, NULL, NULL, NULL); // the last 3 parameter is false as we dont monitor for write, exception
																     // and dont want to set any timeout, now is wait infinitely 
		
		// printf("new activity detected: %d \n", activity);

		if(activity == -1)
		{
			//perror("Err: select error \n"); 
			//fprintf(fpDataLog, "Err: select error \n"); 
			vLogErrToFile(fpDataLog, "Socket select error"); 			

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
				if (i == stServerSock.iSocketFd)
				{
					// accept the new client connection
					new_fd = accept(stServerSock.iSocketFd, (struct sockaddr *)&their_addr, &sin_size);

					if(new_fd == -1)
					{
						//perror("Err: accept error \n");
						//fprintf(fpDataLog, "Err: accept error \n"); 
						vLogErrToFile(fpDataLog, "Socket accpet error"); 			
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
					//printf("server: got connection from %s\n", s);
					fprintf(fpDataLog, "server: got connection from %s\n", s); 

					bool boMaxClientReached = true; 

					// add in the new fd for the client into our array of client fds 
					for (int k = 0; k < MAX_CLIENT_CONN; k++)
					{
						if(i8ClientSockets[k] == -1)
						{
							//printf("k: %d \n", k); 
							i8ClientSockets[k] = new_fd; 
							boMaxClientReached = false; 
							//printf("i8ClientSockets[%d]: %d \n", k , i8ClientSockets[k]); 
							break; 
						}
					}

					if (boMaxClientReached == true)
					{
						// printf("Err: maximum number of clients reached \n"); 
						fprintf(fpDataLog, "Err: maximum number of clients reached \n"); 
						close(new_fd); // we close the connection since we already hit our maximum 
					}

					// printf("ok im done with setting the client fd to the set \n"); 
				}
			
				// if there is activity but not the server's socket's fd
				// this means that our connected client have information for us 
				else
				{
					// printf("ok recv data from client \n"); 
					char strToPeek[stUserCommandConfig.u16Offset+1000]; // making sure our peek buffer is bigger than the offset length

					uint16_t numBytesPeeked = 0; 

					// we will recv the information from the client 
					//umBytesPeeked = recv(i, strToPeek, sizeof(strToPeek), MSG_PEEK); // TODO: mels need to do handling for offset != 0, might hv case where data len recv is less than the offset
																					// 		 this means that the data do not contain any length information 
					
					// printf("right outside while loop \n"); 
					// printf("stUserCommandConfig.u16Offset: %d \n", stUserCommandConfig.u16Offset);

					uint16_t u16ExNumBytesHeader = stUserCommandConfig.u16Offset + stUserCommandConfig.u16MessageLen; 

					while (numBytesPeeked < u16ExNumBytesHeader) 
					{
						// printf("right inside while loop \n"); 

						//select(i+1, &readfds, NULL, NULL, NULL); 
						// printf("right inside while loop, after select \n"); 

						// consume the data in the OS buffer 
						int recvdPeek = recv(i, strToPeek + numBytesPeeked, u16ExNumBytesHeader - numBytesPeeked, MSG_PEEK);
						numBytesPeeked += recvdPeek;
						// printf("recvdPeek: %d \n", recvdPeek); 
						// printf("numBytesPeeked: %d \n", numBytesPeeked); 
						// printf("u16ExNumBytesHeader %d \n", u16ExNumBytesHeader); 

						if (recvdPeek <= 0)
						{
							//printf("hrmmm \n");
							fprintf(fpDataLog, "hrmmm \n"); 
							break;
						}

						// printf("im inside the bytes extraction loop \n");
					}

					if(numBytesPeeked == -1) // error occured 
					{
						//perror("Err: recv error and will close this client's connection \n");
						// fprintf(fpDataLog, "Err: recv error and will close this client's connection \n"); 
						vLogErrToFile(fpDataLog, "Server receive error"); 			
						close(i); 
						// clear this client's fd from the set 
						FD_CLR(i, &masterfds);

						// clear this client's fd in the client fd array 
						for (int k = 0; k < MAX_CLIENT_CONN; k++)
						{
							if (i8ClientSockets[k] == i)
							{
								i8ClientSockets[k] = -1;
								//printf("i8ClientSockets[%d]: %d \n", k , i8ClientSockets[k]); 

								break; 
							}
						}
						break; 
					}

					if (numBytesPeeked == 0) // connection closed by client 
					{
						//printf("Client disconnected, will proceed to close the fd for this client \n"); 
						fprintf(fpDataLog, "Client disconnected, will proceed to close the fd for this client \n");
						close(i); 
						// clear this client's fd from the set 
						FD_CLR(i, &masterfds);
						
						// clear this client's fd in the client fd array 
						for (int k = 0; k < MAX_CLIENT_CONN; k++)
						{
							if (i8ClientSockets[k] == i)
							{
								i8ClientSockets[k] = -1;

								//printf("i8ClientSockets[%d]: %d \n", k , i8ClientSockets[k]); 
								break; 
							}
						}
						break; 
					}

					// printf("recv and no error for now \n");

					// processing the number of bytes to read 
					char cMsgAfterOffset[255], cMsgLenConf[255];
					strcpy(cMsgAfterOffset, strToPeek + stUserCommandConfig.u16Offset);
					strncpy(cMsgLenConf, cMsgAfterOffset, stUserCommandConfig.u16MessageLen);
					cMsgLenConf[stUserCommandConfig.u16MessageLen] = '\0';
					uint16_t u16MsgLenExpected = asciiToDecimal(cMsgLenConf) + stUserCommandConfig.u16MessageLen;

					char strToRecv[u16MsgLenExpected+1000]; // making sure the buffer size is bigger than our data size 

					// Read full data sent 
					uint16_t u16DataByteRecvd = 0;
					while (u16DataByteRecvd < u16MsgLenExpected) 
					{
						//select(i+1, &readfds, NULL, NULL, NULL); 

						// consume the data in the OS buffer 
						int recvd = recv(i, strToRecv + u16DataByteRecvd, u16MsgLenExpected - u16DataByteRecvd, 0);
						if (recvd <= 0)
						{
							break;
						}
						u16DataByteRecvd += recvd;
						// printf("recvd: %d \n", recvd); 
						// printf("u16DataByteRecvd: %d \n", u16DataByteRecvd); 
						// printf("u16MsgLenExpected: %d \n", u16MsgLenExpected); 
						//printf("im inside the bytes extraction loop \n");
					}

					//strToRecv[u16MsgLenExpected] = '\0'; // null terminate the string 

					if(u16DataByteRecvd < 0)
					{
						//printf("Err: data recv error \n");
						fprintf(fpDataLog, "Err: data recv error \n");
					}

					else if (u16DataByteRecvd == 0)
					{
						//printf("client disconnected \n"); // to put into the hostsim.log 
						fprintf(fpDataLog, "client disconnected \n");
					}

					else if (u16DataByteRecvd >= u16MsgLenExpected)
					{
						// printf("extracted the data based on the length and the offset ooo \n");
						strToRecv[u16DataByteRecvd] = '\0';
						//printf("Full message: %s\n", strToRecv); // dump the message recv into a separate file (each msg recv is an individual file, eg: hoostsimrecv.1, limit the number of file to 9999, after that rewrite file from 1)

						// save the messages into a file 
						char fileNameRecv[100];
						//printf("fileNum: %d \n", fileNum);
						snprintf(fileNameRecv, sizeof(fileNameRecv), "hostsimrecv.%d", fileNum);

						FILE *fpDataRecv = fopen(fileNameRecv, "w"); 
						if(!fpDataRecv)
						{
							//perror("fopen fail \n");
							vLogErrToFile(fpDataLog, "Failed to open data log file"); 			
						}

						fprintf(fpDataRecv, "%s", strToRecv); 
						fclose(fpDataRecv); 

						if (fileNum >= MAX_FILE_LOG)
						{
							fileNum = 0; 
						}
						fileNum++; 
					}

					// printf("extracted the data based on the length and the offset ooo \n");
					// strToRecv[u16DataByteRecvd] = '\0';
					//printf("Full message: %s\n", strToRecv);

					// Send response
					processSendToWhichClient(stUserCommandConfig.i8ReplyToWhichClient, strToRecv, i, u16MsgLenExpected, max_fd, fpDataLog);

				}
			}
		}

		// flush the buffer so that the logs can be saved into the file without having to close the file 
		fflush(fpDataLog);
	}

	fclose(fpDataLog); 
	//printf("program closing \n"); 
	fprintf(fpDataLog, "program closing \n");
	return 0; 
}