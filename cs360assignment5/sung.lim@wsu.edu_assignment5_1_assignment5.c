#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <math.h>
#include <errno.h>
#include <string.h>

#define PN 5 //Macro for defining the number of philosophers

struct philosopher{ //struct that keeps track of the philosopher eat/think time
	int philoID;
	int eatTime;
	int thinkTime;
};

int randomGaussian(int mean, int stddev) { //given function
	double mu = 0.5 + (double) mean;
	double sigma = fabs((double) stddev);
	double f1 = sqrt(-2.0 * log((double) rand() / (double) RAND_MAX));
	double f2 = 2.0 * 3.14159265359 * (double) rand() / (double) RAND_MAX;
	if (rand() & (1 << 5)) 
		return (int) floor(mu + sigma * cos(f2) * f1);
	else            
		return (int) floor(mu + sigma * sin(f2) * f1);
}

int main(int argc, char *argv[]){
	int n = PN;
	/*Create chopsticks*/
	int chopsticks = semget(IPC_PRIVATE, PN, IPC_CREAT | IPC_EXCL | 0600); //create chopsticks
	if (chopsticks == -1){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		exit(1);
	}
	unsigned short myArray[PN];
	for (int i = 0; i < PN; i++){
		myArray[i] = 1;
	}
	/*Set chopsticks to 1*/
	if (semctl(chopsticks, 0, SETALL, myArray) == -1){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		exit(1);
	}
	pid_t pids;
	/*Loop to create philosophers*/
	for (int i  = 0; i < PN; ++i){ //fork 5 times
		if (((pids = fork())< 0)){
			fprintf(stderr, "Error: %s\n", strerror(errno));
			exit(1);
		
		}else if(pids == 0){
			struct philosopher philo; //init philo values
			philo.philoID = i;
			philo.eatTime = 0;
			philo.thinkTime = 0;
			int leftChop = 0; //init adjacent chopsticks to the philosopher
			int rightChop = 0;
			if (philo.philoID == 0){ //special case for philo 0
				leftChop = 4;
			}else{
				leftChop = philo.philoID - 1;
				rightChop = philo.philoID;
			}
			/*Create sembufs for adjacent chopsticks*/
			struct sembuf grabChopstick[2] = {{leftChop, -1, 0}, {rightChop, -1, 0}};
			struct sembuf thinking[2] = {{leftChop, 1, 0}, {rightChop, 1, 0}};
			srand(i); //needed to give each process its own seed for the randomGaussian function
			while(1){
				/*Generate think/eat time using randomGaussian.
				Add that to eat/think time and then sleep for that 
				amount of time. Use semop to pick up/put down 
				chopsticks.Once 100s of eat time has been achieved, 
				exit the fork.*/
				int thinkTime = randomGaussian(9, 3);
				if (thinkTime < 0){
					thinkTime = 0;
				}
				printf("Philosopher %d thinking for %d seconds (total = %d)\n", philo.philoID, thinkTime, philo.thinkTime);
				philo.thinkTime = philo.thinkTime + thinkTime;
				sleep(thinkTime);

				int pickUp = semop(chopsticks, grabChopstick, 2);
				if (pickUp != 0 && errno != 43){
					fprintf(stderr, "Error: %s\n", strerror(errno));
					exit(1);
				}
				int eatTime = randomGaussian(11, 7);
				if (eatTime < 0){
					eatTime = 0;
				}
				printf("Philosopher %d eating for %d seconds (total = %d)\n", philo.philoID, eatTime, philo.eatTime);
				philo.eatTime = philo.eatTime + eatTime;
				sleep(eatTime);
				int setDown = semop(chopsticks, thinking, 2);
				if (setDown != 0 && errno != 43){
					fprintf(stderr, "Error: %s\n", strerror(errno));
					exit(1);
				}
				if (philo.eatTime >= 100){
					printf("Philosopher %d has finished eating\n", philo.philoID);
					exit(0);
				}
			}
		}
	}
	int status;
	pid_t pid;
	while (n > 0){ /*Parent waits for all the philosophers to finish*/
		pid = wait(&status);
		--n;
	}
	semctl(chopsticks, 0, IPC_RMID); //clean up semaphores
	return 0;
}