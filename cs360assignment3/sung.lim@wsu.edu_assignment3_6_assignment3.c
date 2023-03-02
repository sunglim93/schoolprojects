/***********************************************************************
name:
	readable -- recursively count readable files.
description:	
	See CS 360 Files and Directories lectures for details.
***********************************************************************/

/* Includes and definitions */

/**********************************************************************
Take as input some path. Recursively count the files that are readable.
If a file is not readable, do not count it. Return the negative value
of any error codes you may get. However, consider what errors you want
to respond to.
**********************************************************************/
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <linux/limits.h>

int readable(char* inputPath) {
	char *cwdp = inputPath;
	struct stat area, *s = &area;
/*Testing if inputPath is a relative or 
absolute path. If relative, get current
working directory and set it to cwdp*/
	if (cwdp == NULL){
		char cwd[PATH_MAX];
		cwdp = getcwd(cwd, PATH_MAX);
	}else{
		if (lstat(cwdp, s) == 0){//Testing if input path is a file or a directory
			if (!S_ISDIR(s->st_mode)){
				if (S_ISREG(s->st_mode)){
					return 1; //If readable, return 1, if not, return 0
				}else{
					return 0;
				}
			}
		}
	}
	int count = 0;
	int error;
	error = chdir(cwdp);
	if (error != 0){ //check if you can chdir() into cwdp
		return -errno;
	}
	DIR *dirp;
	dirp = opendir(".");
/*While loop to recursively go through each directory.
If the file is regular and readable, increment the count.
If the path is a directory, open the directory and check its
contents.*/
	while(1){
		struct dirent *rd = readdir(dirp);
		if ((rd) == NULL){
			closedir(dirp);
			return count;
		}
		char *ptr;
		ptr = rd->d_name;
		if (strcmp(ptr, ".") == 0 || strcmp(ptr, "..") == 0){
			continue;
		}
		if (lstat(ptr, s) == 0){
			if(S_ISLNK(s->st_mode)){
				continue;
			}
			if (S_ISREG(s->st_mode)){
				if(access(ptr, R_OK) == 0){
					count++;
				}
			}
			if(S_ISDIR(s->st_mode)){
				//printf("This is a directory\n");
				if (access(ptr, R_OK) == 0){
					count = count + readable(ptr);
					chdir("..");
				}
			}else{
				continue;
			}
		}	
	}
	closedir(dirp);
    return count;
}