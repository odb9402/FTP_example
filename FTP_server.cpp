#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#define FILENAMESIZE 256
#define GET_REQ	02
#define PUT_REQ 01
#define QUIT_REQ 12
#define ACK 11
#define BUFSIZE 512

int GET(int client_socket);
int PUT(int client_socket);

int main(int argc, char *argv[]){
	
	if ( argc != 2 ){
		printf(" Usage : %s <port>\n" , argv[0]);
		exit(1);
	}

	if (atoi(argv[1]) < 5000){
		printf(" It only provide service with port number which bigger than 5000.\n");
		exit(1);
	}

	in_port_t server_port = atoi(argv[1]);


	int server_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (server_socket == -1){
		printf("Socket initialize Error");
		return -1;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr)); // initialize socket struct.
	printf("socket initialize\n");
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_port);
	if ( bind (server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == - 1 ){
		printf("Address binding Error");
		return -1;
	}
	printf("socket bind with port %d\n", server_port);
	if ( listen(server_socket,5) == -1){
		printf("Address binding Error");
		return -1;
	}

	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
	if (client_socket == -1){
		printf("Client socket accept Error");
		return -1;
	}

	printf("Client Connected [%s]:[%d}\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	while(1){
		uint8_t msgType;
		ssize_t numBytes = recv(client_socket, &msgType, sizeof(msgType), MSG_WAITALL);
		
		if (numBytes == -1){
		printf("recv() error");
		return -1;
		}
		else if (numBytes == 0){
		printf("connection is closed.");
		return -1;
		}
		else if (numBytes != sizeof(msgType)){
		printf("received bytes is unexpected.");
		return -1;
		}

		if (msgType == GET_REQ){
			printf("GET request is received. \n");
			GET(client_socket);
		}
		else if (msgType == PUT_REQ){
			printf("PUT request is received. \n");
			PUT(client_socket);
		}
		else if (msgType == QUIT_REQ){
			printf("QUIT request is received.\n");
			break;
		}
		else
			printf("Message is not defined.\n");
	}
	
	close(client_socket);
	close(server_socket);

	return 0;

}



int PUT(int client_socket){
	
	// File name receive
	char file_name[FILENAMESIZE];
	ssize_t numBytes_recv = recv(client_socket, file_name, FILENAMESIZE, MSG_WAITALL);
	if(numBytes_recv == -1){
		printf("receive error");
		return -1;
	}
	else if (numBytes_recv == 0){
		printf("peer connection closed");
		return -1;
	}
	else if (numBytes_recv != FILENAMESIZE){
		printf("receive unexpected number of bytes");
		return -1;
	}
	strcat(file_name, "_up");
	printf("file name : %s\n", file_name);

	// File size receive
	uint32_t net_file_size;
	uint32_t file_size;
	numBytes_recv = recv(client_socket, &net_file_size, sizeof(net_file_size), MSG_WAITALL);
	if ( numBytes_recv == -1 ){
		printf("receive error");
		return -1;
	}
	else if ( numBytes_recv == 0 ){
		printf("peer connection closed");
		return -1;
	}
	else if ( numBytes_recv != sizeof(net_file_size)){
		printf("receive unexpected number of bytes");
		return -1;
	}
	file_size = ntohl(net_file_size);
	printf("file size = %u\n",file_size);


	// File receive
	FILE *fp = fopen(file_name, "w");
	if (fp==NULL){
		printf("file open error");
		return -1;
	}

	uint32_t recv_file_size = 0;
	while (recv_file_size < file_size){
		char buffer[BUFSIZE];
		numBytes_recv = recv(client_socket, buffer, BUFSIZE,0);
		if ( numBytes_recv == -1 ){
			printf("receive error");
			return -1;
		}
		else if( numBytes_recv == 0 ){
			printf("peer connection closed");
			return -1;
		}
		
		fwrite(buffer, sizeof(char), numBytes_recv, fp);
		if(ferror(fp)){
			printf("File write error");
			return -1;
		}
		
		recv_file_size += numBytes_recv;
	}
	fclose(fp);


	// SUCCESS
	uint8_t msgType = ACK;
	ssize_t numByte_sent = send(client_socket, &msgType, sizeof(msgType), 0);
	if (numByte_sent == -1){
		printf("send error");
		return -1;
	}
	else if (numByte_sent != sizeof(msgType)){
		printf("sent unexpected numbers of bytes");
	}
}


int GET(int client_socket){
	//File name received
	char file_name[FILENAMESIZE];
	ssize_t numBytes = recv(client_socket, file_name, FILENAMESIZE, MSG_WAITALL);

	printf("%d",numBytes);	
	if ( numBytes == -1 ){
		printf("recv() error\n");
		return -1;
	}
	else if ( numBytes == 0 ){
		printf("peer connection closed\n");
		return -1;
	}
	else if ( numBytes != FILENAMESIZE ){
		printf("recv unexpected number of bytes\n");
		return -1;
	}
	printf("file name : %s\n", file_name);


	//file_name can be either full path or just file name.
	//File size send
	struct stat sb;		// Linux file state struct :: stat.
	if ( stat(file_name, &sb) < 0){	// Linux file state is stored to sb.
		printf("File state load error");
		return -1;
	}
	
	uint32_t file_size = sb.st_size;
	uint32_t net_file_size = htonl(file_size);
	ssize_t numByte_sent = send(client_socket, &net_file_size, sizeof(net_file_size), 0);
	
	if ( numByte_sent == -1 ){
		printf("");
		return -1;
	}
	else if ( numByte_sent != sizeof(net_file_size)){
		printf("");
		return -1;
	}


	//File send
	FILE *fp = fopen(file_name, "r");
	if (fp == NULL){
		printf("file open error");
		return -1;
	}

	while(!feof(fp))
	{
		char buffer[BUFSIZE];
		size_t numByte_read = fread(buffer, sizeof(char), BUFSIZE, fp);
		if(ferror(fp)){
			printf("file read error.");
			return -1;
		}
		
		numByte_sent = send(client_socket, buffer, numByte_read, 0);
		if(numByte_sent == -1){
			printf("sending error");
			return -1;
		}
		else if ( numByte_sent != numByte_read ){
			printf("sent unexpected number of bytes");
			return -1;
		}
	}
	fclose(fp);

	//SUCCESS
	uint8_t msgType;
	numBytes = recv(client_socket, &msgType, sizeof(msgType), MSG_WAITALL);
	if (numBytes == -1){
		printf("receive error");
		return -1;
	}
	else if (numBytes == 0){
		printf("peer connection closed");
		return -1;
	}
	else if (numBytes != sizeof(msgType)){
		printf("receive unexpected number of bytes");
		return -1;
	}

	if(msgType == ACK){
		printf("%s file transmit success(%u Bytes)\n", file_name, file_size);
	}
	else{
		printf("%s file transmit fail\n", file_name);
	}
}
