#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define FILENAMESIZE 256
#define GET_REQ 02
#define PUT_REQ 01
#define QUIT_REQ 12
#define ACK 11
#define BUFSIZE 512

int GET(int client_socket, char *A_file_name);
int PUT(int client_socket, char *A_file_name);

int main(int argc, char* argv[]){
	if (argc!=3){
		printf("servadd servport is need\n");
		exit(1);
	}

	char *serverIP = argv[1];
	char operand[50];
	in_port_t serverPort = atoi(argv[2]);

	int client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client_socket == -1){
		printf(" socket init error \n");
		return -1;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(serverIP);
	server_addr.sin_port = htons(serverPort);
	if(connect(client_socket,(sockaddr*)&server_addr, sizeof(server_addr)) == -1){
		printf(" connection error \n");
		return -1;
	}

	int op;

	printf(" FTP START \n");
	
	while(1){
		printf("1 : PUT\n");
		printf("2 : GET\n");
		printf("3 : QUIT\n");
		
		
		printf("INPUT OPERATION : ");
		scanf("%d",&op);

		if(op==1){
			printf("input the file name : ");
			scanf("%s", operand);
			PUT(client_socket, operand);
		}
		else if(op==2){
			printf("input the file name : ");
			scanf("%s", operand);
			GET(client_socket, operand);
		}
		else if(op==3){
			uint8_t msgType = QUIT_REQ;
			ssize_t numByte_sent = send(client_socket, &msgType, sizeof(msgType), 0);
			if(numByte_sent == -1){
				printf("sending error\n");
				return -1;
			}
			else if (numByte_sent != sizeof(msgType)){
				printf("sent unexpected number of bytes\n");
				return -1;
			}
			break;
		}
		else
			printf("wrong op\n");
	}
	close(client_socket);
	return 0;
}


int PUT(int client_socket, char *A_file_name){
	uint8_t msgType = PUT_REQ;
	ssize_t numByte_sent = send(client_socket, &msgType, sizeof(msgType), 0);
	if(numByte_sent == -1){
		printf("send error");
		return -1;
	}
	else if ( numByte_sent != sizeof(msgType)){
		printf("sent unexpected number of bytes : msg type\n");
		return -1;
	}

	char file_name[FILENAMESIZE];
	memset(file_name, 0, FILENAMESIZE);
	strcpy(file_name, A_file_name);
	numByte_sent = send(client_socket, file_name, FILENAMESIZE, 0);
	
	if(numByte_sent == -1){
		printf("send error");
		return -1;
	}
	else if ( numByte_sent != FILENAMESIZE){
		printf("sent unexpected number of bytes : file size\n");
		return -1;
	}
	
	struct stat sb;
	if(stat(file_name,&sb)<0){
		printf("stat error");
		return -1;
	}
	uint32_t file_size = sb.st_size;
	uint32_t net_file_size = htonl(file_size);
	numByte_sent = send(client_socket, &net_file_size, sizeof(net_file_size),0);

	if(numByte_sent == -1){
		printf("send error");
		return -1;
	}
	else if ( numByte_sent != sizeof(net_file_size)){
		printf("sent unexpected number of bytes : filesize \n");
		return -1;
	}

	FILE *fp = fopen(file_name,"r");
	if(fp==NULL){
		printf("file open error");
		return -1;
	}

	while(!feof(fp)){
		char buffer[BUFSIZE];
		size_t numByte_read = fread(buffer, sizeof(char), BUFSIZE, fp);
		if(ferror(fp)){
			printf("file read error");
			return -1;
		}
		numByte_sent = send(client_socket, buffer, numByte_read, 0);
		if(numByte_sent == -1){
			printf("send error");
			return -1;
		}
		else if ( numByte_sent != numByte_read){
			printf("sent unexpected number of bytes");
			return -1;
		}
	}
	fclose(fp);

	ssize_t numByte_recv = recv(client_socket, &msgType, sizeof(msgType), MSG_WAITALL);
	if(numByte_recv == -1){
		printf("receive error");
		return -1;
	}
	else if ( numByte_recv == 0 ){
		printf("peer connection closed");
		return -1;
	}
	else if ( numByte_recv != sizeof(msgType)){
		printf("received unexpected number of bytes");
		return -1;
	}

	if(msgType == ACK)
		printf("%s is uploaded.(%u Bytes)\n",file_name,file_size);
	else
		printf("%s(%uBytes) upload fail", file_name,file_size);

}


int GET(int client_socket, char *A_file_name){
	uint8_t msgType = GET_REQ;
	ssize_t numByte_sent = send(client_socket, &msgType, sizeof(msgType), 0);
	if(numByte_sent == -1){
		printf("send error");
		return -1;
	}
	else if ( numByte_sent != sizeof(msgType)){
		printf("sent unexpected number of bytes : message type\n");
		return -1;
	}

	char file_name[FILENAMESIZE];
	memset(file_name, 0, FILENAMESIZE);
	strcpy(file_name, A_file_name);
	numByte_sent = send(client_socket, file_name, FILENAMESIZE, 0);
	
	if(numByte_sent == -1){
		printf("send error");
		return -1;
	}
	else if ( numByte_sent != FILENAMESIZE){
		printf("sent unexpected number of bytes : file name\n");
		return -1;
	}

	printf("file name : %s\n",file_name);
	uint32_t net_file_size;
	uint32_t file_size;
	ssize_t numByte_recv = recv(client_socket, &net_file_size, sizeof(net_file_size), MSG_WAITALL);
	if(numByte_recv == -1){
		printf("received error");
		return -1;
	}
	else if (numByte_recv == 0 ){
		printf("peer connection end");
		return -1;
	}
	else if ( numByte_recv != sizeof(net_file_size)){
		printf("received unexpected number of bytes");
		return -1;
	}
	file_size = ntohl(net_file_size);


	FILE *fp = fopen(file_name,"w");
	if(fp==NULL){
		printf("file open error");
		return -1;
	}
	uint32_t recv_file_size = 0;
	while(recv_file_size < file_size){
		char buffer[BUFSIZE];
		size_t numByte_recv = recv(client_socket, buffer, BUFSIZE, 0);
		if(numByte_recv == -1){
			printf("receive error");
			return -1;
		}
		else if ( numByte_recv == 0){
			printf("peer connection closed");
			return -1;
		}

		fwrite(buffer, sizeof(char), numByte_recv, fp);

		if(ferror(fp)){
			printf("file write error");
			return -1;
		}
		recv_file_size += numByte_recv;
	}
	printf("file size : %u DOWNLOAD COMPLETE\n", file_size);
	fclose(fp);


	msgType = ACK;
	numByte_sent = send(client_socket, &msgType, sizeof(msgType), 0);
	if(numByte_sent == -1){
		printf("send error\n");
		return -1;
	}
	else if ( numByte_sent != sizeof(msgType)){
		printf("sent unexpected number of bytes : ACK\n");
		return -1;
	}
}
