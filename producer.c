#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>

int N = 4;
float SLEEP = 2.0;

int main()
{
	srand(time(NULL));

	int fd_buff;   // file descriptor for buffer
	int fd_empty;  // file descriptor for empty
	int fd_full;   // file descriptor for full
	int fd_info;   // file descriptor for info
	int value = 1; // value for semaphores

	char *buffer; // pointer to buffer memory segment
	char *empty;  // pointer to empty memory segment
	char *full;	  // pointer to full memory segment

	sem_t *semaphore_producer; // pointer to producer semaphore
	sem_t *semaphore_consumer; // pointer to consumer semaphore
	sem_t *semaphore_empty;	   // pointer to empty semaphore
	sem_t *semaphore_full;	   // pointer to full semaphore

	int *info; // pointer to info

	// open shared memory
	fd_buff = shm_open("/buffer", O_CREAT | O_RDWR, 0600);
	ftruncate(fd_buff, N * sizeof(int));
	buffer = mmap(NULL, N * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd_buff, 0);

	// open empty and full memory segments
	fd_empty = shm_open("/empty", O_CREAT | O_RDWR, 0600);
	ftruncate(fd_empty, N * sizeof(int));
	empty = mmap(NULL, N * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd_empty, 0);

	fd_full = shm_open("/full", O_CREAT | O_RDWR, 0600);
	ftruncate(fd_full, N * sizeof(int));
	full = mmap(NULL, N * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd_full, 0);

	// open info memory segment
	fd_info = shm_open("/info", O_CREAT | O_RDWR, 0600);
	ftruncate(fd_info, 5 * sizeof(int));
	info = mmap(NULL, 5 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd_info, 0);

	// check if memory segments are opened correctly
	if (buffer == MAP_FAILED || empty == MAP_FAILED || full == MAP_FAILED || info == MAP_FAILED)
	{
		perror("mmap");
		exit(1);
	}

	// initialize info to match the current state of the memory segments
	if (info[0] == 0)
	{
		info[0]++;
		for (int i = 0; i < N; i++)
		{
			empty[i] = i;
			full[i] = -1;
		}
		for (int i = 1; i < 5; i++)
		{
			info[i] = 0;
		}
	}

	// open semaphores
	semaphore_producer = sem_open("/Sp", O_CREAT, 0600, N);
	semaphore_consumer = sem_open("/Sr", O_CREAT, 0600, 0);
	semaphore_empty = sem_open("/Se", O_CREAT, 0600, 1);
	semaphore_full = sem_open("/Sf", O_CREAT, 0600, 1);

	// check if semaphores are opened correctly
	if (semaphore_producer == SEM_FAILED || semaphore_consumer == SEM_FAILED || semaphore_empty == SEM_FAILED || semaphore_full == SEM_FAILED)
	{
		perror("sem_open");
		exit(1);
	}

	int prod_index;
	// producer loop
	while (1)
	{
		float speed = ((float)rand() / (float)(RAND_MAX) + 0.5) * SLEEP; // generate random speed for producer

		sem_wait(semaphore_producer); // wait for producer semaphore

		sem_wait(semaphore_empty);	 // wait for empty semaphore
		prod_index = empty[info[2]]; // get index from empty memory segment
		info[2] = (info[2] + 1) % N; // update empty index
		sem_post(semaphore_empty);	 // post empty semaphore

		printf("Production: buffer[%d] = %d\n", prod_index, value); // print production
		buffer[prod_index] = value++;								// update buffer
		sleep(speed);												// sleep for random speed

		sem_wait(semaphore_full);	 // wait for full semaphore
		full[info[3]] = prod_index;	 // update full memory segment
		info[3] = (info[3] + 1) % N; // update full index
		sem_post(semaphore_full);	 // post full semaphore

		sem_post(semaphore_consumer); // post consumer semaphore
	}

	return 0;
}