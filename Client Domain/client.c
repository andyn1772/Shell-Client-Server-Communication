// The 'client.c' code goes here.
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 9999 
#include "Md5.c"  // Feel free to include any other .c files that you need in the 'Client Domain'.

#define SA struct sockaddr

char *cmd[80];

/*
int open_clientfd(char *hostname, char* port){
	int clientfd;
	struct addrinfo hints, *listp, *p;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_flags |= AI_ADDRCONFIG;
	getaddrinfo(hostname, port, &hints, &listp);
	
	for (p = listp; p; p = p->ai_next){
		if ((clientfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0){
			continue;
		}
		if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1){
			break;
		}
	}

	freeaddrinfo(listp);
	if (!p){
		return -1;
	}else{
		printf("Connected");
		return clientfd;
	}
};
*/
int getcommand(char *command[], char *uinput){
    char *reslt;
    reslt = strtok(uinput, " ");
    int i = 0;
    while (reslt != NULL){
        command[i] = reslt;
        reslt = strtok(NULL, " ");
        i++;
    }
	return i;
};


int main(int argc, char *argv[])
{	
	
	int client_socket;
    struct sockaddr_in serv_addr;

	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (client_socket < 0){
		printf("\n Socket creation error \n");
        return -1;
	}
	serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

	int addr_status = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr);
	if (addr_status <= 0){
		printf("\nInvalid address/ Address not supported \n");
        return -1;
	}
	int connect_status = connect(client_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (connect_status < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
	
	
	FILE *fp;
	char line[256];
	char path[256] = "Local Directory/";

	ssize_t sent_size;
    char buffer[1024];
    int received_size;

	
    strcat(path, argv[1]);

	fp = fopen(path, "r");
	if (fp == NULL){
		printf("Exited");
		exit(EXIT_FAILURE);
	}
	
	char result;
	int appendmode = 0;
	// num_arg is the number of arguments in command. Important for sending multiple lines to append
	int num_arg;
	printf("Welcome to ICS53 Online Cloud Storage\n");
	while (fgets(line, sizeof(line), fp)){
		line[strcspn(line, "\r\n")] = 0;
		
		printf("> ");
		num_arg = getcommand(cmd, line);
		// Pause
		if (strncmp(cmd[0], "pause", 5) == 0){
			printf("Pause");
			sleep(atoi(cmd[1]));
		// Append
		}else if (strncmp(cmd[0], "append", 6) == 0){
			printf("Append\n");
			// Send type of operation 
			send(client_socket, cmd[0], strlen(cmd[0])+1, 0);
			recv(client_socket, buffer, 1024, 0);
			printf("%s\n", cmd[0]);
			// Send file name
			send(client_socket, cmd[1], strlen(cmd[1])+1, 0);
			recv(client_socket, buffer, 1024, 0);
			printf("%s\n", cmd[1]);
			// If file exist then get next line
			if (strncmp(buffer, "exist", 5) == 0){
				appendmode = 1;
				fgets(line, sizeof(line), fp);
				line[strcspn(line, "\r\n")] = 0;
				num_arg = getcommand(cmd, line);
				// If next line is close, send message to server
				if (strncmp(cmd[0], "close", 5) == 0){
					char* message = "close";
					send(client_socket, message, strlen(message), 0);
					recv(client_socket, buffer, 1024, 0);
				}else{
					while(1){
						printf("Appending> ");
						if (strncmp(cmd[0], "pause", 5) == 0){
							char* message = "paused";
							send(client_socket, message, strlen(message), 0);
							recv(client_socket, buffer, 1024, 0);
							sleep(atoi(cmd[1]));
							fgets(line, sizeof(line), fp);
							line[strcspn(line, "\r\n")] = 0;
							num_arg = getcommand(cmd, line);
						}else if (strncmp(cmd[0], "close", 5) == 0){
							char* message = "close";
							send(client_socket, message, strlen(message), 0);
							recv(client_socket, buffer, 1024, 0);
							break;
						}else{
							printf("%d\n", num_arg);
							for (int j = 0; j < num_arg; j++){
								//printf("%s\n", cmd[j]);
								if (j == num_arg - 1){
									char* message = "end";
									send(client_socket, message, strlen(message), 0);
									recv(client_socket, buffer, 1024, 0);

									send(client_socket, cmd[j], strlen(cmd[j])+1, 0);
									recv(client_socket, buffer, 1024, 0);
								}else{
									send(client_socket, cmd[j], strlen(cmd[j])+1, 0);
									recv(client_socket, buffer, 1024, 0);
								}
							}
							fgets(line, sizeof(line), fp);
							line[strcspn(line, "\r\n")] = 0;
							num_arg = getcommand(cmd, line);
						}
					}
				}
			// If file does not exist
			}else{
				printf("File [%s] could not be found in remote directory.\n", cmd[1]);
			}
		// Upload
		}else if (strncmp(cmd[0], "upload", 6) == 0){
			printf("Upload\n");

			// Send operation

			FILE *uptr;
			char file_chunk[1024];
			char upload_path[256] = "Local Directory/";
			strcat(upload_path, cmd[1]);
			
			
			uptr = fopen(upload_path, "r");
			if (uptr == NULL){
				printf("File [%s] could not be found in local directory.\n", cmd[1]);
			}else{
				send(client_socket, cmd[0], strlen(cmd[0])+1, 0);
				recv(client_socket, buffer, 1024, 0);
				// Send file name
				send(client_socket, cmd[1], strlen(cmd[1])+1, 0);
				recv(client_socket, buffer, 1024, 0);
				
				
				fseek(uptr, 0L, SEEK_END);
				int file_size = ftell(uptr);
				fseek(uptr, 0L, SEEK_SET);

				int total_bytes = 0;
				int current_chunk_size;
				ssize_t sent_bytes;
				while (total_bytes < file_size){
					memset(&file_chunk, 0, 1024);
					current_chunk_size = fread(&file_chunk, sizeof(char), 1024, uptr);
					sent_bytes = send(client_socket, &file_chunk, current_chunk_size, 0);
					total_bytes = total_bytes + sent_bytes;
				}
				char *exit = "exit";
				send(client_socket, exit, strlen(exit), 0);
				recv(client_socket, buffer, 1024, 0);
				printf("%d %s\n", file_size, buffer);
				fclose(uptr);
			}
		// Download
		}else if (strncmp(cmd[0], "download", 8) == 0){
			printf("Download\n");

			// Send operation
			send(client_socket, cmd[0], strlen(cmd[0])+1, 0);
			recv(client_socket, buffer, 1024, 0);

			// Send file name
			send(client_socket, cmd[1], strlen(cmd[1])+1, 0);
			recv(client_socket, buffer, 1024, 0);

			if (strncmp(buffer, "exist", 5) == 0){
				int received_size;
				char file_chunk[1024];
				int n;
				FILE *dptr;
				char download_path[256] = "Local Directory/";
				int total_bytes = 0;

				strcat(download_path, cmd[1]);
				dptr = fopen(download_path, "w");
				if (dptr == NULL){
					perror("Error");
				}
				while(1){
					memset(&file_chunk, 0, 1024);
					received_size = recv(client_socket, file_chunk, 1024, 0);
					if (strncmp(file_chunk, "exit", 4) == 0){
						fclose(dptr);
						break;
					}
					total_bytes = total_bytes + received_size;
					fwrite(&file_chunk, sizeof(char), received_size, dptr);
					char *mess = "received";
					send(client_socket, mess, strlen(mess), 0);
				}
				char* finished = "bytes uploaded successfully";
				send(client_socket, finished, strlen(finished), 0);
				printf("%d bytes downloaded successfully.");
			}else{
				printf("File [%s] could not be found in remote directory.\n", cmd[1]);
			}
		// Delete
		}else if (strncmp(cmd[0], "delete", 6) == 0){
			printf("Delete\n");
			// Send operation
			send(client_socket, cmd[0], strlen(cmd[0])+1, 0);
			recv(client_socket, buffer, 1024, 0);

			// Send file name
			send(client_socket, cmd[1], strlen(cmd[1])+1, 0);
			recv(client_socket, buffer, 1024, 0);

			if (strncmp(buffer, "exist", 5) == 0){
				printf("File deleted successfully.\n");
			}else{
				printf("File [%s] could not be found in remote directory.\n", cmd[1]);
			}

		// Syncheck
		}else if (strncmp(cmd[0], "syncheck", 8) == 0){
			printf("Sync Check Report:\n");
			char hashed[1024];
			memset(hashed, 0, 1024);
			int exist = 0;
			char filepath[256] = "Local Directory/";
			FILE *filep;
			strcat(filepath, cmd[1]);
			filep = fopen(filepath, "r");
			if (filep == NULL){
				exist = 0;
			}else{
				exist = 1;
				MDFile(filepath, &hashed);
				fseek(filep, 0L, SEEK_END);
				int file_size = ftell(filep);
				printf("- Local Directory:\n-- File Size: %d bytes.\n", file_size);
			}
			

			send(client_socket, cmd[0], strlen(cmd[0])+1, 0);
			recv(client_socket, buffer, 1024, 0);

			send(client_socket, cmd[1], strlen(cmd[1])+1, 0);
			recv(client_socket, buffer, 1024, 0);

			if (strncmp(buffer, "exist", 5) == 0){
				char *m1 = "bytes";
				send(client_socket, m1, strlen(m1), 0);
				// getting file bytes from remote
				recv(client_socket, buffer, 1024, 0);

				printf("- Remote Directory:\n-- File Size: %d bytes.\n", atoi(buffer));
				if (exist == 1){
					char *m2 = "hash";
					send(client_socket, m2, strlen(m2), 0);
					// getting file bytes from remote
					recv(client_socket, buffer, 1024, 0);
					if (strncmp(hashed, buffer, strlen(hashed)) == 0){
						printf("-- Sync Status: synced.\n");
					}else{
						printf("-- Sync Status: unsynced.\n");
					}
				}
				char *m3 = "lock";
				send(client_socket, m1, strlen(m1), 0);
				// getting file bytes from remote
				recv(client_socket, buffer, 1024, 0);
				if (strncmp(buffer, "locked", 6) == 0){
					printf("-- Lock Status: locked.\n");
				}else{
					printf("-- Lock Status: unlocked.\n");
				}
			}
		// Quit
		}else if (strncmp(cmd[0], "quit", 4) == 0){
			printf("Quit\n");
			char* message = "close";
			send(client_socket, message, strlen(message), 0);
			recv(client_socket, buffer, 1024, 0);
			break;
		// Close
		}else if (strncmp(cmd[0], "close", 5) == 0){
			printf("Close\n");
			char* message = "close";
			send(client_socket, message, strlen(message), 0);
			recv(client_socket, buffer, 1024, 0);
			if (appendmode != 1){
				printf("Not in appending mode!\n");
			}
		}else{
			continue;
		}
		// Question: Would I need a sleep here?
	}

	fclose(fp);
	close(client_socket);

	/*
	printf("I am the client.\n");
	printf("Input file name: %s\n", argv[1]);
	printf("My server IP address: %s\n", argv[2]);
	md5_print();
	printf("-----------\n");
	*/

	return 0;
}
