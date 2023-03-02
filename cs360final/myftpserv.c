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
#include <sys/stat.h>
#define MY_PORT_NUMBER_S "3253"
#define MY_PORT_NUMBER_I 3253

int main(int argc, char const *argv[]){
	struct sockaddr_in servAddr;
	int listenfd;
	int socketfd;
	socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd == -1){
		fprintf(stderr, "Control connection: socket() failed with error %s\n", strerror(errno));
		fflush(stdout);
		exit(1);
	}
	int controlErr;
	controlErr = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	if (controlErr == -1){
		fprintf(stderr, "Control connection: setsockopt failed with error %s\n", strerror(errno));
		fflush(stdout);
		exit(1);
	}

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(MY_PORT_NUMBER_I);
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socketfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){ //bind socket
		perror("Error");
		exit(1);
	}
	int controlListenErr;
	controlListenErr = listen(socketfd, 4); //start listening for control connections
	if (controlListenErr == -1){
		fprintf(stderr, "Control connection: listen() failed with error %s\n", strerror(errno));
		fflush(stdout);
		exit(1);
	}

	while(1){
		struct sockaddr clientAddr;
		socklen_t length = sizeof(struct sockaddr_in);
		listenfd = accept(socketfd, (struct sockaddr*)&clientAddr, &length); //start accepting control connections
		if (listenfd == -1){
			fprintf(stderr, "Control connection: accept() failed with error %s\n", strerror(errno));
			fflush(stdout);
			exit(1);
		}
		
		if (fork()){ //parent
			int hostInfo;
			char hostName[NI_MAXHOST] = {0}; //host name
			hostInfo = getnameinfo((struct sockaddr*)&clientAddr, sizeof(clientAddr), hostName, sizeof(hostName), NULL, 0, NI_NUMERICSERV);
			if (hostInfo != 0){ //error
				printf("Error: %s\n", gai_strerror(hostInfo));
				fflush(stdout);
				exit(1);
			}
			printf("%s has connected.\n", hostName);
			continue;
		
		}else{ //child
			while(1){
				char *command;
				command = calloc(PATH_MAX+6, sizeof(char));
				char tmpCommand[2] = {0};

				/*Reading server command from client*/
				for (int i = 0; i < PATH_MAX+5; i++){
					if(read(listenfd, tmpCommand, 1)== 0){
						printf("Control EOF detected, exiting\n");
						printf("Fatal error, exiting\n");
						fflush(stdout);
						free(command);
						close(listenfd);
						exit(1);
					}
					strcat(command, tmpCommand);
					if (tmpCommand[0] == '\n'){
						break;
					}
				}

				/*Error checking for commands that need a data connection*/
				if (strncmp(command, "L", 1) == 0 || strncmp(command, "G", 1) == 0 || strncmp(command, "P", 1) == 0){
					write(listenfd, "ENo Data connection present\n", 2);
					free(command);
					continue;
				}
				/*Exit command*/
				if (strncmp(command, "Q", 1) == 0){
					write(listenfd, "A\n", 2);
					printf("Quitting\n");
					fflush(stdout);
					free(command);
					close(listenfd);
					exit(0);
				}
				/*Change directory command*/
				if (strncmp(command, "C", 1) == 0){
					int j = 0;
					char parameter[PATH_MAX] = {0};
					for (int i = 1; i < strlen(command); i++){ //copy path name to parameter to use with chdir()
						if (command[i] == '\n'){
							break;
						}
						parameter[j] = command[i];
						j++;
					}
					int err;
					err = chdir(parameter);

					if (err != 0){ //If there is an error, send E with error message
						char errMessage[1027] = {0};
						errMessage[0] = 'E';
						strcat(errMessage, strerror(errno));
						errMessage[strlen(errMessage)] = '\n';
						printf("cd to %s failed with error %s", parameter, errMessage);
						write(listenfd, errMessage, strlen(errMessage));
						free(command);
						continue;
					}
					printf("Changed current directory to %s\n", parameter);
					write(listenfd, "A\n", 2);
					free(command);
					continue;
				}

				if (strncmp(command, "D", 1) == 0){ //open up data connection
					free(command);
					struct sockaddr_in dataAddr; //open up a new socket for the data connection
					int datacfd;
					int dSocketfd;
					dSocketfd = socket(AF_INET, SOCK_STREAM, 0);
					if (dSocketfd == -1){
						fprintf(stderr, "Data connection: socket() failed with error %s\n", strerror(errno));
						fflush(stdout);
						close(listenfd);
						exit(1);
					}
					int dataErr;
					dataErr = setsockopt(dSocketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
					if (dataErr == -1){
						fprintf(stderr, "Data connection: setsockopt failed with error %s\n", strerror(errno));
						close(listenfd);
						exit(1);
					}

					memset(&dataAddr, 0, sizeof(dataAddr));
					dataAddr.sin_family = AF_INET;
					dataAddr.sin_port = 0;
					dataAddr.sin_addr.s_addr = htonl(INADDR_ANY);

					if (bind(dSocketfd, (struct sockaddr*) &dataAddr, sizeof(dataAddr)) < 0){ //bind socket
						perror("Error");
						exit(1);
					}

					unsigned int dataLength = sizeof(dataAddr);
					getsockname(dSocketfd,(struct sockaddr*) &dataAddr, &dataLength);

					char temp[6] = {0};
					sprintf(temp, "%d", ntohs(dataAddr.sin_port)); //extract new port number

					int tmpBuf = 1; //figure out how many 0's are needed for padding
					if (strlen(temp) != 5){
						int tmpNum = 5 - strlen(temp);
						tmpBuf = tmpBuf + tmpNum;
					}

					char str[8];
					memset(str, '0', sizeof(str)); //formatting the string correctly to send back to the client
					str[0] = 'A';
					str[6] = '\n';
					str[7] = '\0';
					for (int i = tmpBuf; i < 6; i++){
						str[i] = temp[i-tmpBuf];
					}
					write(listenfd, str, 7);
					int dataListenError;
					dataListenError = listen(dSocketfd, 1); //start listening
					if (dataListenError == -1){
						fprintf(stderr, "Data connection: listen() failed with error %s\n", strerror(errno));
						fflush(stdout);
						exit(1);
					}
					
					struct sockaddr clientDataAddr;
					socklen_t dataConLength = sizeof(struct sockaddr_in);
					datacfd = accept(dSocketfd, (struct sockaddr*)&clientDataAddr, &dataConLength); //start accepting data connection
					if (datacfd == -1){
						fprintf(stderr, "Data connection: accept() failed with error %s\n", strerror(errno));
						fflush(stdout);
						close(listenfd);
						exit(1);
					}
					char *newCommand; //read a new command from the client after establishing a data connection
					newCommand = calloc(PATH_MAX+2, sizeof(char));
					char tmpNewCommand[2] = {0};

					for (int i = 0; i < PATH_MAX+1; i++){
						read(listenfd, tmpNewCommand, 1);
						if (strncmp(tmpNewCommand, "\n",1) == 0){
							break;
						}
						strcat(newCommand, tmpNewCommand);
					}
					/*the rls command. Fork and redirect ls to the data connection*/
					if (strncmp(newCommand, "L", 1) == 0){
						if (fork()){
							wait(NULL);
							write(listenfd, "A\n", 2);
							free(newCommand);
							close(datacfd);
							continue;
						}else{
							close(1);
							dup(datacfd);
							execlp("ls", "ls", "-l", NULL);
							/*Send error message from server to client if necessary*/
							fprintf(stderr, "ls failed with error %s\n", strerror(errno));
							char errMessage[1027] = {0};
							errMessage[0] = 'E';
							strcat(errMessage, strerror(errno));
							errMessage[strlen(errMessage)] = '\n';
							write(listenfd, errMessage, strlen(errMessage));
							exit(1);
						}
					}
					/*the get command. the path name is extracted and then the file is
					checked to see if it is a regular readable file.*/
					if (strncmp(newCommand, "G", 1) == 0){
						int j = 0;
						char parameter[PATH_MAX] = {0};
						for (int i = 1; i < strlen(newCommand); i++){
							if (newCommand[i] == '\n'){
								break;
							}
							parameter[j] = newCommand[i];
							j++;
						}

						printf("Reading file %s\n", parameter);
						fflush(stdout);

						struct stat area, *s = &area;
						if(lstat(parameter, s) == 0){ //check if the pathname is a directory
							if (!S_ISDIR(s->st_mode)){
								if(!S_ISREG(s->st_mode)){
									write(listenfd, "EFile is not regular.\n", 22);
									free(command);
									close(datacfd);
									continue;
								}
							}else{
								write(listenfd, "EFile is a directory.\n", 22);
								free(command);
								close(datacfd);
								continue;
							}
						}
						if (access(parameter, R_OK) != 0){ //check if the file is readable
							char error[1028] = {0};
							error[0] = 'E';
							strcat(error, strerror(errno));
							error[strlen(error)] = '\n';
							write(listenfd, error, strlen(error)); 
							free(command);
							close(datacfd);
							continue;
						}
						printf("transmitting file %s to client\n", parameter);
						write(listenfd, "A\n", 2);
						if (fork()){
							wait(NULL);
							free(command);
							close(datacfd);
							continue;
						}else{
							close(1);
							dup(datacfd);
							execlp("cat", "cat", parameter, NULL);
							fprintf(stderr, "cat failed with error %s\n", strerror(errno));
							char error[1028] = {0};
							error[0] = 'E';
							strcat(error, strerror(errno));
							error[strlen(error)] = '\n';
							write(listenfd, error, strlen(error)); 
							exit(1);
						}
					}
					if (strncmp(newCommand, "P", 1) == 0){
						
						char fileName[PATH_MAX] = {0};
						int j = 0;
						for (int i = 1; i < strlen(newCommand); i++){
							if (newCommand[i] == '\n'){
								break;
							}
							fileName[j] = newCommand[i];
							j++;
						}

						printf("Writing file %s\n", fileName);
						fflush(stdout);

						if (access(fileName, F_OK) == 0){ //error if file already exists
							write(listenfd, "EFile exists\n", 13);
							continue;
						}

						write(listenfd, "A\n", 2);

						close(0);
						dup(datacfd);
						FILE *fp;
						fp = fopen(fileName, "w"); //create new file with file name
						char buf[101];
						memset(buf, '\0', sizeof(buf));
						printf("Receiving file %s from client\n", fileName);
						fflush(stdout);
						while(read(datacfd, buf, 100) != 0){ //read and write data from client
							fwrite(buf, 1, strlen(buf), fp);
							memset(buf, '\0', sizeof(buf));
						}
						fclose(fp);
						continue;
					}
				}
			}
			exit(0);
		}
	}
}
