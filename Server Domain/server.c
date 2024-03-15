// The 'server.c' code goes here.

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 9999 

char *cmd[80];
char *locks[100];
int total_locks = 0;

#include "Md5.c"  // Feel free to include any other .c files that you need in the 'Server Domain'.

void getcommand(char *command[], char *uinput){
    char *reslt;
    reslt = strtok(uinput, " ");
    int i = 0;
    while (reslt != NULL){
        command[i] = reslt;
        reslt = strtok(NULL, " ");
        i++;
    }
};

int searchlock(char *filename){
	for (int i = 0; i < total_locks; i++){
		if (strncmp(filename, locks[i], strlen(filename)) == 0){
			return 1;
		}
	}
	return 0;
}

void commands(char *client_command, int c_sock){
    getcommand(cmd, client_command);
	char buff[1024];
	char path[256] = "Remote Directory/";

	if (strncmp(cmd[0], "append", 6) == 0){
		//printf("Append\n");

		// Let client know we are in append if statement
		char* message = "InAppend";
		send(c_sock, message, strlen(message), 0);
		// Get ready to receive file name
		recv(c_sock, buff, 1024, 0);
		getcommand(cmd, buff);


		FILE *fp;

		strcat(path, cmd[0]);
		
		fp = fopen(path, "r");
		if (fp == NULL){
			char* message = "no";
			send(c_sock, message, strlen(message), 0);
		}else{
			locks[total_locks] = cmd[0];
			total_locks += 1;
			fclose(fp);
			FILE *fp = fopen(path, "a");
			char* message = "exist";
			send(c_sock, message, strlen(message), 0);
			// Receive the line to append
			int end = 0;
			while(1){
				int received_size = recv(c_sock, buff, 1024, 0);
				
				getcommand(cmd, buff);
				if (strncmp(cmd[0], "close", 5) == 0){
					char* message = "Done";
					send(c_sock, message, strlen(message), 0);
					fclose(fp);
					break;
				}else if (strncmp(cmd[0], "pause", 5) == 0){
					char* message = "Okay";
					send(c_sock, message, strlen(message), 0);
				}else{
					if (received_size == 0){
						break;
					}else if (strncmp(cmd[0], "end", 3) == 0){
						char* message = "Okay";
						send(c_sock, message, strlen(message), 0);
						end = 1;
					}else{
						if (end == 1){
							char* message = "Appended";
							send(c_sock, message, strlen(message), 0);
							fprintf(fp, buff);
							char* newline = "\n";
							fprintf(fp, newline);
							end = 0;
							total_locks -= 1;
						}else{
							char* message = "Appended";
							send(c_sock, message, strlen(message), 0);
							fprintf(fp, buff);
							// add space inbetween each word
							char* space = " ";
							fprintf(fp, space);
						}
					}
				}
			}
		}
	}else if (strncmp(cmd[0], "upload", 6) == 0){
		int received_size;
		char file_chunk[1024];
		int n;
		FILE *uptr;

		// Let client know we are in upload if statement
		char* message1 = "InUpload";
		send(c_sock, message1, strlen(message1), 0);
		
		// Receive file name
		char* message = "Received";
		recv(c_sock, buff, 1024, 0);
		send(c_sock, message, strlen(message), 0);
		// Calling get command tokenizes the buffer we received
		getcommand(cmd, buff);
		// Creates full path
		strcat(path, cmd[0]);
		
		uptr = fopen(path, "w");
		if (uptr == NULL){
			perror("Error");
		}
		while(1){
			memset(&file_chunk, 0, 1024);
			received_size = recv(c_sock, file_chunk, 1024, 0);
			if (strncmp(file_chunk, "exit", 4) == 0){
				fclose(uptr);
				break;
			}
			fwrite(&file_chunk, sizeof(char), received_size, uptr);
		}
		char* finished = "bytes uploaded successfully";
		send(c_sock, finished, strlen(finished), 0);
	}else if (strncmp(cmd[0], "download", 8) == 0){
		int received_size;
		char file_chunk[1024];
		int n;
		FILE *dptr;
		
		// Let client know we are in download if statement
		char* message1 = "InDownload";
		send(c_sock, message1, strlen(message1), 0);

		// Receive file name
		recv(c_sock, buff, 1024, 0);
		// Tokenize buffer to get file name
		getcommand(cmd, buff);

		strcat(path, cmd[0]);

		dptr = fopen(path, "r");
		if (dptr == NULL){
			char *exist = "no";
			send(c_sock, exist, strlen(exist), 0);
		}else{
			char *exist = "exist";
			send(c_sock, exist, strlen(exist), 0);

			fseek(dptr, 0L, SEEK_END);
			int file_size = ftell(dptr);
			fseek(dptr, 0L, SEEK_SET);

			int total_bytes = 0;
			int current_chunk_size;
			ssize_t sent_bytes;
			while (total_bytes < file_size){
				memset(&file_chunk, 0, 1024);
				current_chunk_size = fread(&file_chunk, sizeof(char), 1024, dptr);
				sent_bytes = send(c_sock, &file_chunk, current_chunk_size, 0);
				total_bytes = total_bytes + sent_bytes;
				recv(c_sock, buff, 1024, 0);
			}
			char *exit = "exit";
			send(c_sock, exit, strlen(exit), 0);
			recv(c_sock, buff, 1024, 0);
			fclose(dptr);
		}
	}else if (strncmp(cmd[0], "delete", 6) == 0){
		int received_size;
		FILE *delptr;
		
		// Let client know we are in download if statement
		char* message1 = "InDelete";
		send(c_sock, message1, strlen(message1), 0);

		// Receive file name
		recv(c_sock, buff, 1024, 0);
		// Tokenize buffer to get file name
		getcommand(cmd, buff);

		strcat(path, cmd[0]);

		delptr = fopen(path, "r");
		if (delptr == NULL){
			char *exist = "no";
			send(c_sock, exist, strlen(exist), 0);
		}else{
			fclose(delptr);
			remove(path);
			char *exist = "exist";
			send(c_sock, exist, strlen(exist), 0);
		}
	}else if (strncmp(cmd[0], "syncheck", 8) == 0){
		int received_size;
		FILE *filep;
		
		// Let client know we are in syncheck if statement
		char* message1 = "InSync";
		send(c_sock, message1, strlen(message1), 0);

		recv(c_sock, buff, 1024, 0);
		getcommand(cmd, buff);

		strcat(path, cmd[0]);

		filep = fopen(path, "r");
		if (filep == NULL){
			char *exist = "no";
			send(c_sock, exist, strlen(exist), 0);
		}else{
			char *exist = "exist";
			send(c_sock, exist, strlen(exist), 0);

			recv(c_sock, buff, 1024, 0);
			fseek(filep, 0L, SEEK_END);
			int file_size = ftell(filep);

			fclose(filep);
			char inttostring[1024];
			snprintf(inttostring, 1024, "%d", file_size);
			send(c_sock, inttostring, sizeof(inttostring), 0);
			recv(c_sock, buff, 1024, 0);
			if (strncmp(buff, "hash", 4) == 0){
				char hashed[1024];
				memset(hashed, 0, 1024);
				MDFile(path, &hashed);
				send(c_sock, hashed, strlen(hashed)+1, 0);
				recv(c_sock, buff, 1024, 0);
			}
			int contains = searchlock(cmd[0]);
			if (contains == 1){
				char *lockedup = "locked";
				send(c_sock, lockedup, strlen(lockedup), 0);
			}else{
				char *lockedup = "no";
				send(c_sock, lockedup, strlen(lockedup), 0);
			}
		}
	}else if (strncmp(cmd[0], "quit", 4) == 0){
		char* message = "Okay";
		send(c_sock, message, strlen(message), 0);
	}else if (strncmp(cmd[0], "close", 5) == 0){
		char* message = "Okay";
		send(c_sock, message, strlen(message), 0);
	}
    
};

int main(int argc, char *argv[])
{
	// Question: Do I need to create a new server socket for each new client connection?

	int client_socket, server_socket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == 0){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	// Question: What does the line below do?
//    address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_addr.s_addr = inet_addr(argv[1]);
	address.sin_port = htons(PORT);

	if (bind(server_socket, (struct sockaddr*)&address, sizeof(address)) < 0){
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	// Question: Should I changed parameter 1 to a bigger number if I am running multi threading?
	if (listen(server_socket, 1) < 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}
	client_socket = accept(server_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
	if (client_socket < 0){
		perror("accept");
		exit(EXIT_FAILURE);
	}

	char buffer[1024];
	
	while (1){
		int received_size = recv(client_socket, buffer, 1024, 0);
		if (received_size == 0){
			close(client_socket);
			break;
		}
		commands(buffer, client_socket);
		

	}


	/*
	printf("I am the server.\n");
	printf("Server IP address: %s\n", argv[1]);
	md5_print();
	printf("-----------\n");
	*/

	
	return 0;
}
