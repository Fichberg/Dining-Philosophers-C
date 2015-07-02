#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>

#define FOOD         10000
#define PHILOSOPHERS 5
#define FORKS        PHILOSOPHERS
#define COORDINATOR  PHILOSOPHERS
#define AVAILABLE    1

typedef struct philosopher {
	int number;
	int consumed;
	struct philosopher *all_philosophers;
} Philosopher;

typedef struct fork {
	int available;
	sem_t semph;
} Fork;

Fork *forks;
sem_t waiter, next_client, forks_available;
int food, next, count, remaining, finish;
pthread_mutex_t lock; /*food*/


void *dinner(void*);
void initiate_philosophers(Philosopher*);
void initiate_forks();

int main(int argc, char **argv)
{
	int i;
	pthread_t *philosopher_threads;
	Philosopher *args;

	philosopher_threads = malloc((PHILOSOPHERS + 1) * sizeof(*args));
	args = malloc((PHILOSOPHERS + 1) * sizeof(*args));
	forks = malloc(FORKS * sizeof(*forks));

	initiate_philosophers(args);
	initiate_forks();
	food = FOOD, count = 0, finish = 0, next = -1; remaining = PHILOSOPHERS;

	if(pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("Error creating lock mutex.\n");
		exit(1);
	}

	if(sem_init(&waiter, 0, 1))
	{
		printf("Error creating semaphore.\n");
		return -1;
	}

	if(sem_init(&forks_available, 0, 1))
	{
		printf("Error creating semaphore.\n");
		return -1;
	}

	if(sem_init(&next_client, 0, 0))
	{
		printf("Error creating semaphore.\n");
		return -1;
	}


	for(i = 0; i <= PHILOSOPHERS; i++)
	{
		if (pthread_create(&philosopher_threads[i], NULL, dinner, &args[i]))
		{
			printf("Error creating thread.");
			abort();
		}
	}
	for(i = 0; i <= PHILOSOPHERS; i++)
	{
		if (pthread_join(philosopher_threads[i], NULL))
		{
			printf("Error joining thread.");
			abort();
		}
	}


	printf("Food = %d\n", food);
	for(i = 0; i < PHILOSOPHERS; i++) printf("Philosopher %d consumed %d\n", i, args[i].consumed);

	free(args); args = NULL;
	free(philosopher_threads); philosopher_threads = NULL;
	for(i = 0; i < FORKS; i++) sem_destroy(&forks[i].semph);
	free(forks); forks = NULL;
	sem_destroy(&waiter); sem_destroy(&next_client); sem_destroy(&forks_available);
	pthread_mutex_destroy(&lock);
	return 0;
}


void *dinner(void *args)
{
	int right_fork, left_fork;
	Philosopher *philosopher = ((Philosopher*) args);


	if(philosopher->number == COORDINATOR)
	{
		while(1)
		{
			sem_wait(&next_client);
			left_fork  =  next;
			right_fork = (next + 1) % PHILOSOPHERS;		

			while(remaining >= PHILOSOPHERS) continue;
			sem_wait(&forks_available);
			remaining++;
			sem_post(&forks_available);
			forks[left_fork].available = AVAILABLE;
			sem_post(&forks[left_fork].semph);
			printf("PHILOSOPHERS #%d released fork %d (left)\n", next, left_fork);
			

			while(remaining >= PHILOSOPHERS) continue;
			sem_wait(&forks_available);
			remaining++;
			sem_post(&forks_available);
			forks[right_fork].available = AVAILABLE;
			sem_post(&forks[right_fork].semph);
			printf("PHILOSOPHERS #%d released fork %d (right)\n", next, right_fork);
			sem_post(&waiter);
		}
	}
	else
	{
		left_fork  =  philosopher->number;
		right_fork = (philosopher->number + 1) % PHILOSOPHERS;
		while(1)
		{
			printf("Philosopher %d is THINKING\n", philosopher->number);

			sem_wait(&waiter);
			next = philosopher->number;
			sem_post(&next_client);
			
			while(remaining <= 1) continue;
			sem_wait(&forks_available);
			remaining--;
			sem_post(&forks_available);
			while(!forks[left_fork].available) sem_wait(&forks[left_fork].semph);
			forks[left_fork].available = !AVAILABLE;
			printf("PHILOSOPHERS #%d got fork %d (left)\n", philosopher->number, left_fork);
			
			while(remaining == 0) continue;
			sem_wait(&forks_available);
			remaining--;
			sem_post(&forks_available);
			while(!forks[right_fork].available) sem_wait(&forks[right_fork].semph);
			forks[right_fork].available = !AVAILABLE;
			printf("PHILOSOPHERS #%d got fork %d (right)\n", philosopher->number, right_fork);

			printf("Philosopher %d is EATING\n", philosopher->number);
		}
	}

	pthread_mutex_lock(&lock);
		count++;
		if(count == PHILOSOPHERS) finish = 1;
	pthread_mutex_unlock(&lock);

	return NULL;
}

void initiate_forks()
{
	int i;

	for(i = 0; i < PHILOSOPHERS; i++)
	{
		forks[i].available = AVAILABLE;
		if(sem_init(&forks[i].semph, 0, 1))
		{
			printf("Error creating semaphore.\n");
			exit(-1);
		}
		sem_wait(&forks[i].semph);
	}	
}

void initiate_philosophers(Philosopher *args)
{
	int i;

	for(i = 0; i < FORKS; i++)
	{
		args[i].number = i;
		args[i].consumed = 0;
		args[i].all_philosophers = args;
	}

	args[COORDINATOR].number = COORDINATOR;
	args[COORDINATOR].all_philosophers = args;
}
