#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

#define FOOD         10000
#define PHILOSOPHERS 5
#define FORKS        PHILOSOPHERS
#define COORDINATOR  PHILOSOPHERS

typedef struct philosopher {
	int number;
	int consumed;
	struct philosopher *all_philosophers;
} Philosopher;

typedef struct fork {
	sem_t semph;
} Fork;

Fork *forks;
sem_t waiter;
int food, count, finish;
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
	food = FOOD, count = 0, finish = 0;

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
	sem_destroy(&waiter);
	pthread_mutex_destroy(&lock);
	return 0;
}


void *dinner(void *args)
{
	Philosopher *philosopher = ((Philosopher*) args);

	if(philosopher->number == COORDINATOR)
	{
		philosopher->number = -1;
		while(1)
		{
			while(philosopher->number == -1 && finish == 0) continue;
			if(finish == 1) break;

			sem_wait(&forks[philosopher->number].semph);
			printf("Philosopher %d got fork %d (left)\n", philosopher->number, philosopher->number);
			sem_wait(&forks[(philosopher->number + 1) % PHILOSOPHERS].semph);
			printf("Philosopher %d got fork %d (right)\n", (philosopher->number + 1) % PHILOSOPHERS, (philosopher->number + 1) % PHILOSOPHERS);

			philosopher->number = -1;
			sem_post(&waiter);
		}
	}
	else
	{
		while(1)
		{
			sem_wait(&waiter);

			while((philosopher->all_philosophers[COORDINATOR]).number != -1) continue;

			philosopher->all_philosophers[COORDINATOR].number = philosopher->number;
			
			while((philosopher->all_philosophers[COORDINATOR]).number == philosopher->number) continue;

			printf("Philosopher %d is eating.\n", philosopher->number);
			pthread_mutex_lock(&lock);
				if(food > 0) { 
					food--;
					philosopher->consumed++;
				}
			pthread_mutex_unlock(&lock);

			sem_post(&forks[philosopher->number].semph);
			printf("Philosopher %d released fork %d (left)\n", philosopher->number, philosopher->number);
			sem_post(&forks[(philosopher->number + 1) % PHILOSOPHERS].semph);
			printf("Philosopher %d released fork %d (right)\n", (philosopher->number + 1) % PHILOSOPHERS, (philosopher->number + 1) % PHILOSOPHERS);			

			printf("Philosopher %d is thinking.\n", philosopher->number);
			if(food == 0) break;
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
		if(sem_init(&forks[i].semph, 0, 1))
		{
			printf("Error creating semaphore.\n");
			exit(-1);
		}
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
