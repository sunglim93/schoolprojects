#include <sys/un.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/wait.h>
#include <linux/limits.h>
#include <ctype.h>
#include <sys/stat.h>
#include "myftp.h"
#define MY_PORT_NUMBER_S "3253"
#define MY_PORT_NUMBER_I 3253

int openConnectData(int socketfd, const char *argv){ //helper function to open up a data connection
	write(socketfd, "D\n", 2); //send "D" to the server to open up data connection
	char socketData[8];
	memset(socketData, '\0', sizeof(socketData));
	read(socketfd, socketData, 7); //read socket number from the server
	char newSocket[6];
	memset(newSocket, '\0', sizeof(newSocket));
	if (socketData[0] == 'A'){
		int j = 0;
		int start = 1;
		for (int i = 1; i < 6; i++){ //set start point for copying over port number when port number is padded
			if (socketData[i] == '0'){
				start++; 
			}else{
				break;
			}
		}
		for (int i = start; i < 6; i++){ //copy over port number to newSocket
			newSocket[j] = socketData[i];
			j++;
		}
	}else{
		printf("Error");
		fflush(stdout);
		return -1;
	}
	int datafd; //connect to new data connection
	struct addrinfo hints2, *actualdata2; 
	memset(&hints2, 0, sizeof(hints2));
	int err2;
	hints2.ai_socktype = SOCK_STREAM;
	hints2.ai_family = AF_INET;

	err2 = getaddrinfo(argv, newSocket, &hints2, &actualdata2);
	if(err2 != 0){ //error checking
		printf("Error: %s\n", gai_strerror(err2));
		fflush(stdout);
		exit(1);
	}
	datafd = socket(actualdata2 -> ai_family, actualdata2 -> ai_socktype, 0);
	if (datafd == -1){
		fprintf(stderr, "Data connection: socket() failed with error %s\n", strerror(errno));
		fflush(stdout);
		close(socketfd);
		exit(1);
	}
	if(connect(datafd, actualdata2->ai_addr, actualdata2->ai_addrlen) < 0){
		fprintf(stderr, "%s\n", strerror(errno));
		fflush(stdout);
		exit(1);
	}
	return datafd;
}

int main(int argc, char const *argv[]){
	if (argv[1] == NULL){
		printf("Usage: ./myftp <hostname | IP address>\n");
		return -1;
	}

	int socketfd; //connecting to specified port
	struct addrinfo hints, *actualdata; 
	memset(&hints, 0, sizeof(hints));
	int err;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	err = getaddrinfo(argv[1], MY_PORT_NUMBER_S, &hints, &actualdata);
	if(err != 0){ //error checking
		printf("Error: %s\n", gai_strerror(err));
		fflush(stdout);
		exit(1);
	}
	socketfd = socket(actualdata -> ai_family, actualdata -> ai_socktype, 0);
	if (socketfd == -1){
		fprintf(stderr, "Control connection: socket() failed with error %s\n", strerror(errno));
		fflush(stdout);
		exit(1);
	}
	if(connect(socketfd, actualdata->ai_addr, actualdata->ai_addrlen) < 0){
		fprintf(stderr, "%s\n", strerror(errno));
		fflush(stdout);
		exit(1);
	}

	printf("Connected to server as %s\n", argv[1]);
	fflush(stdout);

	while(1){
		printf("MFTP>"); //prompt user for command
		fflush(stdout);

		char *input; //read input from user
		input = calloc(PATH_MAX+6, sizeof(char));
		read(0, input, PATH_MAX+5);
		char *token = strtok(input, " ");

		if (strncmp(token, "cd", 2) == 0){ //cd command
			token = strtok(NULL, " ");
			if (token == NULL || strcmp(token, "\n") == 0){ //error if no parameter
				free(input);
				printf("Command error: expecting a parameter.\n");
				fflush(stdout);
				continue;
			}
			if (token[strlen(token)-1] == '\n'){ //last character will be \n so need to get rid of that
				token[strlen(token)-1] = '\0';
			}
			int err = 0;
			err = chdir(token); //change directory to specified directory
			if (err != 0){ //error if something goes wrong
				fprintf(stderr, "Change directory: %s\n", strerror(errno));
				fflush(stdout);
			}
			free(input);
			continue;
		}

		if (strncmp(token, "rcd", 3) == 0){
			token = strtok(NULL, " ");
			if (token == NULL || strcmp(token, "\n") == 0){ //error if no parameter
				free(input);
				printf("Command error: expecting a parameter.\n");
				fflush(stdout);
				continue;
			}
			char command[PATH_MAX+1] = "C"; //command for server chdir
			strcat(command, token); //concatenate command with path and then send to server
			write(socketfd, command, strlen(command));
			
			char acknow[1028]; //receive error or acknowledgement from server
			read(socketfd, acknow, 1027);

			if (strncmp(acknow, "A", 1) == 0){
				free(input);
				continue;
			}else{
				printf("Error response from server: ");
				for (int i = 1; i < 1027; i++){
					if (acknow[i] == '\n'){
						break;
					}
					printf("%c", acknow[i]);
					fflush(stdout);
				}
				printf("\n");
				free(input);
				continue;
			}
		}

		if (strncmp(token, "ls", 2) == 0){
			free(input);
			int status;
			int status2;
			if(fork()){
				wait(&status);
				continue;
			}else{
				int fd[2];
				pipe(fd);
				int rdr, wtr;
				rdr = fd[0]; 
				wtr = fd[1];
				if(fork()){
					close(0);
					dup(rdr); 
					close(rdr);
					close(wtr);
					wait(&status2);
					execlp("more", "more", "-20", NULL);
					fprintf(stderr, "%s\n", strerror(errno));
					exit(1);
				}else{
					close(1);
					dup(wtr); 
					close(wtr);
					close(rdr);
					execlp("ls", "ls", "-l", NULL);
					fprintf(stderr, "%s\n", strerror(errno));
					exit(1);
				}
			}
		}

		if (strncmp(input, "rls", 3) == 0){ //rls command
			free(input);
			if (fork()){
				wait(NULL);
				continue;
			}else{
				int datafd;
				datafd = openConnectData(socketfd, argv[1]);
				write(socketfd, "L\n", 2);
				char *acknow;
				acknow = calloc(1028, sizeof(char));
				read(socketfd, acknow, 1027);
				if (strncmp(acknow, "A", 1) == 0){
					if (fork()){
						wait(NULL);
						free(acknow);
						exit(0);
					}else{
						close(0);
						dup(datafd);
						execlp("more", "more", "-20", NULL);
						fprintf(stderr, "%s\n", strerror(errno));
						exit(1);
					}
				}else{
					printf("Error response from server: ");
					for (int i = 1; i < 1027; i++){
						if (acknow[i] == '\n'){
							break;
						}
						printf("%c", acknow[i]);
						fflush(stdout);
					}
					printf("\n");
					free(acknow);
				}
			}

		}

		if (strncmp(token, "show", 4) == 0){
			token = strtok(NULL, " ");
			char newCommand[PATH_MAX+2] = "G";
			strcat(newCommand, token);
			free(input);
			if (fork()){
				wait(NULL);
				continue;
			}else{
				int datafd;
				datafd = openConnectData(socketfd, argv[1]);
				write(socketfd, newCommand, strlen(newCommand));
				char acknow[1028] = {0};
				read(socketfd, acknow, 1027);
				if (strncmp(acknow, "A", 1) == 0){
					close(0);
					dup(datafd);
					execlp("more", "more", "-20", NULL);
					fprintf(stderr, "%s\n", strerror(errno));
					exit(1);
				}else{
					char errorMessage[1027] = {0};
					int k = 0;
					for(int i = 1; i < 1027; i++){
						errorMessage[k] = acknow[i];
						if (acknow[i] == '\n'){
							break;
						}
						k++;
					}
					printf("Error response from server: %s", errorMessage);
					fflush(stdout);
					exit(1);
				}
			}
		}

		if (strncmp(token, "get", 3) == 0){
			int relFlag = 0; //flag for a file in the cwd
			token = strtok(NULL, " ");

			char backup[PATH_MAX] = {0};
			if (strncmp(token, "/", 1) != 0){ //for cwd files
				strcpy(backup, token);
				relFlag = 1;
			}

			char newCommand[PATH_MAX+2] = "G";
			strcat(newCommand, token);
			char filePath[PATH_MAX+2];
			strcpy(filePath, newCommand);

			char *fileName2 = {0};
			if (relFlag == 0){ //if it isn't a file in the cwd
				char *fileName = {0};
				fileName = strtok(filePath, "/");
				while (fileName != NULL){
					fileName2 = fileName;
					fileName = strtok(NULL, "/");
				}
			}else{
				fileName2 = backup;
			}
			fileName2[strlen(fileName2)-1] = '\0';

			if (access(fileName2, F_OK) == 0){
				printf("Opening/creating local file: File exists.\n");
				fflush(stdout);
				free(input);
				continue;
			}

			free(input);
			if (fork()){
				wait(NULL);
				continue;
			}else{
				int datafd;
				datafd = openConnectData(socketfd, argv[1]);
				write(socketfd, newCommand, strlen(newCommand)); //write Gfilepath to server
				char acknow[1028] = {0};
				read(socketfd, acknow, 1027);
				if (strncmp(acknow, "A", 1) == 0){
					close(0);
					dup(datafd);
					FILE *fp;
					fp = fopen(fileName2, "w");
					char buf[101];
					memset(buf, '\0', sizeof(buf));
					while(read(datafd, buf, 100) != 0){
						fwrite(buf, 1, strlen(buf), fp);
						memset(buf, '\0', sizeof(buf));
					}
					fclose(fp);
					exit(0);
				}else{
					char errorMessage[1027] = {0};
					int k = 0;
					for(int i = 1; i < 1027; i++){
						errorMessage[k] = acknow[i];
						if (acknow[i] == '\n'){
							break;
						}
						k++;
					}
					printf("Error response from server: %s", errorMessage);
					fflush(stdout);
					exit(1);
				}
			}
		}

		if(strncmp(token, "put", 3) == 0){
			token = strtok(NULL, " "); //get filepath

			if(token[0] == '/'){
				free(input);
				printf("File is not a local file.\n");
				continue;

			}
			char filePath[PATH_MAX+1] = {0};
			strcpy(filePath, token);
			filePath[strlen(filePath)-1] = '\0';

			struct stat area, *s = &area;
			if(lstat(filePath, s) == 0){ //check if the pathname is a directory
				if (!S_ISDIR(s->st_mode)){
					if (!S_ISREG(s->st_mode)){
						printf("File is not regular.\n");
						free(input);
						continue;
					}
				}else{
					printf("File is a directory.\n");
					free(input);
					continue;
				}
			}
			if (access(filePath, R_OK) != 0){ //check if the file is readable
				printf("Error: ");
				fflush(stdout);
				fprintf(stderr, "%s\n", strerror(errno));
				free(input);
				continue;
			}
			char *fileName = strtok(token, "/"); //get file name
			char *fileName2 = {0};
			while (fileName != NULL){
				fileName2 = fileName;
				fileName = strtok(NULL, "/");
			}

			char serverCommand[PATH_MAX] = {0}; //create PfileName
			serverCommand[0] = 'P';
			strcat(serverCommand, fileName2);
			if(fork()){
				wait(NULL);
				continue;
			}else{
				int datafd;
				datafd = openConnectData(socketfd, argv[1]);
				write(socketfd, serverCommand, strlen(serverCommand)); //send PfileName
				char acknow[1028] = {0}; //receive acknowledgement from server
				read(socketfd, acknow, 1027);
				if (strncmp(acknow, "A", 1) == 0){
					if (fork()){
						wait(NULL);
						close(datafd); //close data connection
						exit(0);
					}else{
						close(1); //send file contents to server
						dup(datafd);
						execlp("cat", "cat", filePath, NULL);
						fprintf(stderr, "%s\n", strerror(errno));
						exit(1);
					}

				}else{
					char errorMessage[1027] = {0};
					int k = 0;
					for(int i = 1; i < 1027; i++){
						errorMessage[k] = acknow[i];
						if (acknow[i] == '\n'){
							break;
						}
						k++;
					}
					printf("Error response from server: %s", errorMessage);
					fflush(stdout);
					exit(1);
				}

			}
		}

		if (strncmp(token, "exit", 4) == 0){
			write(socketfd, "Q\n", 2);
			char acknow[3] = {0};
			read(socketfd, acknow, 2);
			if (acknow[0] == 'A'){
				fflush(stdout);
				free(input);
				close(socketfd);
				break;
			}
		}
		token[strlen(token)-1] = '\0';
		printf("Command '%s' is unknown - ignored\n", token);
	}
	return 0;
}