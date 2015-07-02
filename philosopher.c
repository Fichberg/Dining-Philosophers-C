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
	sem_t can_eat;
	struct philosopher *all_philosophers;
} Philosopher;

typedef struct fork {
	int available;
	sem_t available_fork;
} Fork;

Fork *forks;
int food, turn = 0, count = 0, even_philos, odd_philos;
sem_t count_sem;
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
	food = FOOD, count = 0;



	if(pthread_mutex_init(&lock, NULL) != 0)
	{
		printf("Error creating lock mutex.\n");
		exit(1);
	}

	if(sem_init(&count_sem, 0, 1))
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
	for(i = 0; i < FORKS; i++) sem_destroy(&forks[i].available_fork);
	free(forks); forks = NULL;
	sem_destroy(&count_sem);
	pthread_mutex_destroy(&lock);
	return 0;
}


void *dinner(void *args)
{
	Philosopher *philosopher = ((Philosopher*) args);


	if(philosopher->number == COORDINATOR)
	{
		int i;

		odd_philos = PHILOSOPHERS / 2;
		if(PHILOSOPHERS % 2 == 0) even_philos = odd_philos;
		else even_philos = odd_philos + 1;

		while(1)
		{
			printf("TURN %d:\n\n", turn);
			/*Release all odds OR all even philosophers to eat*/
			for(i = turn; i < PHILOSOPHERS; i+=2) sem_post(&((philosopher->all_philosophers[i]).can_eat));
			
			/*Wait them finish to eat*/
			if(turn % 2 == 0) while (count != even_philos) continue;
			else while (count != odd_philos) continue;
			
			/*Collect all forks*/
			for(i = turn; i < PHILOSOPHERS; i+=2) sem_wait(&forks[i].available_fork);
			turn = (turn + 1) % 2;
			count = 0;
		}
	}
	else
	{
		int right_fork, left_fork;
		left_fork  =  philosopher->number;
		right_fork = (philosopher->number + 1) % PHILOSOPHERS;
		while(1)
		{
			sem_wait(&(philosopher->can_eat));
			printf("Philosopher #%d is eating\n", philosopher->number);
			sleep(1);
			sem_post(&forks[left_fork].available_fork); sem_post(&forks[right_fork].available_fork);
			sem_wait(&count_sem);
				count++;
			sem_post(&count_sem);
			/*do_think()*/
			sleep(1);
		}
	}

	return NULL;
}

void initiate_forks()
{
	int i;

	for(i = 0; i < PHILOSOPHERS; i++)
	{
		forks[i].available = AVAILABLE;
		if(sem_init(&forks[i].available_fork, 0, 1))
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
		if(sem_init(&args[i].can_eat, 0, 0))
		{
			printf("Error creating semaphore.\n");
			exit(-1);
		}
	}

	args[COORDINATOR].number = COORDINATOR;
	args[COORDINATOR].all_philosophers = args;
}
