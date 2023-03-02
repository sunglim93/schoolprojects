/***********************************************************************
name:
	assignment4 -- acts as a pipe using ":" to seperate programs.
description:	
	See CS 360 Processes and Exec/Pipes lecture for helpful tips.
***********************************************************************/

/* Includes and definitions */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <errno.h>

/**********************************************************************
./assignment4 <arg1> : <arg2>

    Where: <arg1> and <arg2> are optional parameters that specify the programs
    to be run. If <arg1> is specified but <arg2> is not, then <arg1> should be
    run as though there was not a colon. Same for if <arg2> is specified but
    <arg1> is not.
**********************************************************************/


int main(int argc, char *argv[]){
	int fd[2];
	pipe(fd);
	int status;
	int rdr, wtr;
	rdr = fd[0]; 
	wtr = fd[1];
	char *leftArg[50];
	char *rightArg[50];
	int p = 0;
	int c = 0;
/*Parse through argv to get arguments*/
	for(int i = 1; i < argc; i++){
		if (strcmp(argv[i], ":") != 0){
			leftArg[p] = argv[i];
			p++;
		}else{
			i++;
			for (int j = i; j < argc; j++){
				if (argv[j] == NULL){
					break;
				}
				rightArg[c] = argv[j];
				c++;
			}
			break;
		}
	}
/*Add NULL to end of args to pass into execvp*/
	leftArg[p] = NULL;
	rightArg[c] = NULL;
/*Parent*/
	if(fork()){
		close(0);
		dup(rdr); 
		close(rdr);
		close(wtr);
		wait(&status);
		if(rightArg[0] == NULL){ //if no right arg, exit
			exit(0);
		}else{
			execvp(rightArg[0], rightArg);
			fprintf(stderr, "%s\n", strerror(errno));
			return -errno;
		}
/*Child*/
	}else{
		if (leftArg[0] == NULL){ //Exit if no left side arg is passed in
			exit(0);
		}
		if (rightArg[0] == NULL){ //Need stdout open to exect just left side
			close(wtr);
			close(rdr);
			execvp(leftArg[0], leftArg);
			fprintf(stderr, "%s\n", strerror(errno));
			return -errno;
		}
		close(1);
		dup(wtr); 
		close(wtr);
		close(rdr);
		execvp(leftArg[0], leftArg);
		fprintf(stderr, "%s\n", strerror(errno));
		return -errno;
	}
	return 0;
}