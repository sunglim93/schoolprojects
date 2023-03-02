#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#define PN 5 //Macro to define how many philosophers one wants.

//create mutex semaphores
pthread_mutex_t chopsticks[PN];
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
struct philosopher{ //struct that keeps track of the philosopher eat/think time
	int philoID;
	int eatTime;
	int thinkTime;
	int leftChop;
	int rightChop;
};
void *createPhilo(void* arg){
	//init philosopher struct
	struct philosopher philo;
	int num = *((int*)arg);
	philo.philoID = num;
	philo.eatTime = 0;
	philo.thinkTime = 0;
	//init philosopher L/R chopstick
	if (num == 0){
		philo.leftChop = 4;
	}else{
		philo.leftChop = num - 1;
	}
	philo.rightChop = num;
	srand(num);
/*
Think/eat loop. Try to pick up the left chopstick and if 
it's not possible, keep looping to the top of the loop until
the chopstick is available. If the left chopstick can be picked
up, try to pick up the right chopstick. If the R chopstick is
not available, put down the L chopstick and go back to the top
of the loop. If both can be picked up, eat for a specified amount
of time and then put the chopsticks down. Think for a specified 
amount of time and then repeat the loop until 100s of eat time
has been achieved. Exit loop if eat time is achieved. 
*/
	while(1){
		int err = 0; //error variable for error handling
		int leftChop = pthread_mutex_trylock(&chopsticks[philo.leftChop]);
		if (leftChop == 16){
			continue;
		}
		if (leftChop != 0 && leftChop != 16){
			fprintf(stderr, "Error: %d\n", -leftChop);
			exit(1);
		}
		if (leftChop == 0){
			int rightChop = pthread_mutex_trylock(&chopsticks[philo.rightChop]);
			if (rightChop == 16){
				err = pthread_mutex_unlock(&chopsticks[philo.leftChop]);
				if (err != 0){
					fprintf(stderr, "Error: %d\n", -err);
					exit(1);
				}else{
					continue;
				}
			}
			if (rightChop != 0 && rightChop != 16){
				fprintf(stderr, "Error: %d\n", -rightChop);
				exit(1);
			}
		}
		int eatTime = randomGaussian(11, 7);
		if (eatTime < 0){
			eatTime = 0;
		}
		printf("Philosopher %d eating for %d seconds (total = %d)\n", philo.philoID, eatTime, philo.eatTime);
		philo.eatTime = philo.eatTime + eatTime;
		sleep(eatTime);
		err = pthread_mutex_unlock(&chopsticks[philo.leftChop]);
		if (err != 0){
			fprintf(stderr, "Error: %d\n", -err);
			exit(1);
		}
		err = pthread_mutex_unlock(&chopsticks[philo.rightChop]);
		if (err != 0){
			fprintf(stderr, "Error: %d\n", -err);
			exit(1);
		}
		if (philo.eatTime >= 100){
			printf("Philosopher %d has finished eating\n", philo.philoID);
			pthread_exit(0);
		}
		int thinkTime = randomGaussian(9, 3);
		if (thinkTime < 0){
			thinkTime = 0;
		}
		printf("Philosopher %d thinking for %d seconds (total = %d)\n", philo.philoID, thinkTime, philo.thinkTime);
		philo.thinkTime = philo.thinkTime + thinkTime;
		sleep(thinkTime);
	}
}

int main(int argc, char *argv[]){
	int n = PN; //from the macro 
	int numbers[n]; //to assign philosopher number
	int error = 0; //variable for error handling
	for(int i = 0; i < n; i++){ // initialize mutex semaphores
		error = pthread_mutex_init(&chopsticks[i], NULL);
		if (error != 0){
			fprintf(stderr, "Error: %d\n", -error);
			exit(1);
		}
	}
	//create an array of thread ID's for philosophers
	pthread_t threadIDs[n];
	for (int i = 0; i < n; i++){
	//pass in function and philosopher number into thread
		numbers[i] = i; //set philo number
		error = pthread_create(&threadIDs[i], NULL, &createPhilo, (void*)&numbers[i]);
		if (error != 0){
			fprintf(stderr, "Error: %d\n", -error);
			exit(1);
		}
	}
	//wait for all threads to finish
	for(int i = 0; i < n; i++){
		error = pthread_join(threadIDs[i], NULL);
		if (error != 0){
			fprintf(stderr, "Error: %d\n", -error);
			exit(1);
		}
	}
	//cleanup
	for(int i = 0; i < n; i++){
		error = pthread_mutex_destroy(&chopsticks[i]);
		if (error != 0){
			fprintf(stderr, "Error: %d\n", -error);
			exit(1);
		}
	}
	return 0;
}