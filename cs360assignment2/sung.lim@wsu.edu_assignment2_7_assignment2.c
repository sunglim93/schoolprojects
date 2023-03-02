/***********************************************************************
name:
	lineNum -- see if a word is in the online dictionary
description:	
	See CS 360 IO lecture for details.
***********************************************************************/

/* Includes and definitions */

/**********************************************************************
Search for given word in dictionary using file descriptor fd.
Return the line number the word was found on, negative of the last line searched
if not found, or the error number if an error occurs.
**********************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
/*Helper function for printing out the error string
since strlen() cannot be used.*/
int myStrLen(char *string){
	int count = 0;
	while(1){
		if (string[count] != '\0'){
			count++;
		}else{
			return count;
		}
	}
}
int lineNum(char *dictionaryName, char *word, int dictWidth) {
	int fd;
	int fileSize;
	int wordNum = 0;
	char buffer[dictWidth];
	char wordBuffer[dictWidth];
	int low = 0;
	int high;
	int mid;
	fd = open(dictionaryName, O_RDONLY);
/*Error handling. Prints out the error string and returns
the error number from errno.*/
	if (fd < 0){
		write(2, strerror(errno),myStrLen(strerror(errno)));
		write(2, "\n", 1);
		return errno;
	}
/*for loop that resets the wordBuffer for consecutive 
function calls*/
	for(int p = 0; p < dictWidth; p++){
		wordBuffer[p] = ' ';
	}
	wordBuffer[dictWidth-1] = '\0';
/*Series of for loops for creating a query to 
strcmp() later on*/
	for(int i = 0; i < dictWidth; i++){
		if (word[i] != 0){
			wordBuffer[i] = word[i];
			wordNum++;
		}else{
			break;
		}
	}
	if (wordNum == dictWidth){
		wordBuffer[dictWidth-1] = '\0';
	}else{
		for(int j = wordNum; j < dictWidth; j++){
			if (j == dictWidth-1){
				wordBuffer[j] = '\0';
			}else{
				wordBuffer[j] = ' ';
			}
		}
	}
/*Binary search implementation. If the query is found, returns
the line number it is found on*/
	fileSize = (lseek(fd, 0, SEEK_END))/dictWidth;
	high = fileSize-1;
	mid = (low + high)/2;
	while (low <= high){
		lseek(fd, mid*dictWidth, SEEK_SET);
		read(fd, buffer, dictWidth);
		buffer[dictWidth-1] = '\0'; //null terminator so that whatever is read matches the query
		int cmp = strcmp(wordBuffer, buffer);
		if (cmp == 0){
			close(fd);
			return mid+1;
		}
		if (cmp > 0){
			low = mid+1;
		}else{
			high = mid-1;
		}
		mid = (low + high)/2;
	}
	close(fd);
	return -mid-1; //returns the last line searched if query isn't found
}
