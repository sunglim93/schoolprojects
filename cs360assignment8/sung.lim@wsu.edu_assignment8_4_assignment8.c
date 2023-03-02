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
#define MY_PORT_NUMBER_S "4999"
#define MY_PORT_NUMBER_I 4999
//struct for keeping track of logins and the login counts
struct logins{
    char *login;
    int loginCount;
};

int main(int argc, char const *argv[]){
    struct logins loginList[100]; //initialize login list and init all values to NULL
    for (int i = 0; i < 100; i++){ 
        loginList[i].login = NULL;
    }

    if (strcmp(argv[1], "server") == 0){ //start up server
        struct sockaddr_in servAddr;
        int listenfd;
        int socketfd;
        socketfd = socket(AF_INET, SOCK_STREAM, 0);
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

        memset(&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(MY_PORT_NUMBER_I);
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(socketfd, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){ //bind socket
            perror("Error");
            exit(1);
        }

        listen(socketfd, 1); //start listening

        while(1){
            struct sockaddr clientAddr;
            int length = sizeof(struct sockaddr_in);
            listenfd = accept(socketfd, (struct sockaddr*)&clientAddr, &length); //start accepting
            if (fork()){
                int hostInfo;
                char hostName[NI_MAXHOST]; //host name
                hostInfo = getnameinfo((struct sockaddr*)&clientAddr, sizeof(clientAddr), hostName, sizeof(hostName), NULL, 0, NI_NUMERICSERV);
                if (hostInfo != 0){ //error
                    printf("Error: %s\n", gai_strerror(hostInfo));
                    fflush(stdout);
                    exit(1);
                }
                for (int i = 0; i < 100; i++){
                    if (loginList[i].login == NULL){ //if it's a new host, create new struct
                        struct logins login;
                        login.login = hostName;
                        login.loginCount = 1;
                        loginList[i] = login;
                        printf("%s %d\n", loginList[i].login, loginList[i].loginCount);
                        fflush(stdout);
                        break;
                    }
                    if (strcmp(loginList[i].login, hostName) == 0){//if the host already exists, increment count
                        loginList[i].loginCount++;
                        printf("%s %d\n", loginList[i].login, loginList[i].loginCount);
                        fflush(stdout);
                        break;
                    }
                }
            }else{
                time_t curtime = time(NULL); //child writes the current time to the client
                write(listenfd, ctime(&curtime), strlen(ctime(&curtime)));
                fflush(stdout);
                exit(0);
            }
        }
    }

    if (strcmp(argv[1], "client") == 0){ //client things
        int socketfd;
        struct addrinfo hints, *actualdata; 
        memset(&hints, 0, sizeof(hints));
        int err;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family = AF_INET;

        err = getaddrinfo(argv[2], MY_PORT_NUMBER_S, &hints, &actualdata);
        if(err != 0){ //error checking
            printf("Error: %s\n", gai_strerror(err));
            fflush(stdout);
            exit(1);
        }
        socketfd = socket(actualdata -> ai_family, actualdata -> ai_socktype, 0);
        if(connect(socketfd, actualdata->ai_addr, actualdata->ai_addrlen) < 0){
            printf("problem\n");
            fflush(stdout);
            exit(1);
        }
        char time[19];
        read(socketfd,time, 18); //print out time received from the host
        time[18] = '\n';
        write(1, time, 19);
        fflush(stdout);
    }
    return 0;
}