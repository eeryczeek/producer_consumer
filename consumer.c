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

	sem_t *producer_semaphore; // pointer to producer semaphore
	sem_t *consumer_semaphore; // pointer to consumer semaphore
	sem_t *empty_semaphore;	   // pointer to empty semaphore
	sem_t *full_semaphore;	   // pointer to full semaphore

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

	// check if memory segments are opened successfully
	if (buffer == MAP_FAILED || empty == MAP_FAILED || full == MAP_FAILED || info == MAP_FAILED)
	{
		perror("mmap");
		exit(1);
	}

	// initialize info to match the state of the memory segments
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
	producer_semaphore = sem_open("/Sp", O_CREAT, 0600, N);
	consumer_semaphore = sem_open("/Sr", O_CREAT, 0600, 0);
	empty_semaphore = sem_open("/Se", O_CREAT, 0600, 1);
	full_semaphore = sem_open("/Sf", O_CREAT, 0600, 1);

	// check if semaphores are opened successfully
	if (producer_semaphore == SEM_FAILED || consumer_semaphore == SEM_FAILED || empty_semaphore == SEM_FAILED || full_semaphore == SEM_FAILED)
	{
		perror("sem_open");
		exit(1);
	}

	int cons_index;
	// consumer loop
	while (1)
	{
		float speed = ((float)rand() / (float)(RAND_MAX) + 1) * SLEEP; // random speed

		sem_wait(consumer_semaphore); // wait for consumer semaphore

		sem_wait(full_semaphore);	 // wait for full semaphore
		cons_index = full[info[4]];	 // get index of consumer
		info[4] = (info[4] + 1) % N; // update index of consumer
		sem_post(full_semaphore);	 // post full semaphore

		printf("Reading: %d from buffer[%d]\n", buffer[cons_index], cons_index); // print value read from buffer
		buffer[cons_index] = value++;											 // write to buffer
		sleep(speed);															 // sleep for random speed

		sem_wait(empty_semaphore);	 // wait for empty semaphore
		full[info[1]] = cons_index;	 // update index of full
		info[1] = (info[1] + 1) % N; // update index of full
		sem_post(empty_semaphore);	 // post empty semaphore

		sem_post(producer_semaphore); // post producer semaphore
	}

	return 0;
}