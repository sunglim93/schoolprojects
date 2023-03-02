.SUFFIXES: .c .o
CC = gcc
CCFLAGS = -Wall

all: server client

server: myftpserv.c
	gcc -Wall -o myftpserve myftpserv.c

client: myftp.c myftp.h
	gcc -Wall -o myftp myftp.c myftp.h

clean: 
	rm -f myftp
	rm -f myftpserve