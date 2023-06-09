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
	int value = 1; // initial value for semaphores

	int file_descriptor_buffer = shm_open("/buffer", O_CREAT | O_RDWR, 0600);
	ftruncate(file_descriptor_buffer, N * sizeof(int));
	char *buffer_memory_segment = mmap(NULL, N * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor_buffer, 0);

	int file_descriptor_empty = shm_open("/empty", O_CREAT | O_RDWR, 0600);
	ftruncate(file_descriptor_empty, N * sizeof(int));
	char *empty_memory_segment = mmap(NULL, N * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor_empty, 0);

	int file_descriptor_full = shm_open("/full", O_CREAT | O_RDWR, 0600);
	ftruncate(file_descriptor_full, N * sizeof(int));
	char *full_memory_segment = mmap(NULL, N * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor_full, 0);

	int file_descriptor_info = shm_open("/info", O_CREAT | O_RDWR, 0600);
	ftruncate(file_descriptor_info, 5 * sizeof(int));
	char *info_memory_segment = mmap(NULL, 5 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor_info, 0);

	// check if memory segments are opened correctly
	if (buffer_memory_segment == MAP_FAILED || empty_memory_segment == MAP_FAILED || full_memory_segment == MAP_FAILED || info_memory_segment == MAP_FAILED)
	{
		perror("mmap");
		exit(1);
	}

	// initialize info to match the current state of the memory segments
	if (info_memory_segment[0] == 0)
	{
		info_memory_segment[0]++;
		for (int i = 0; i < N; i++)
		{
			empty_memory_segment[i] = i;
			full_memory_segment[i] = -1;
		}
		for (int i = 1; i < 5; i++)
		{
			info_memory_segment[i] = 0;
		}
	}

	// open semaphores
	sem_t *semaphore_producer = sem_open("/Sp", O_CREAT, 0600, N);
	sem_t *semaphore_consumer = sem_open("/Sr", O_CREAT, 0600, 0);
	sem_t *semaphore_empty = sem_open("/Se", O_CREAT, 0600, 1);
	sem_t *semaphore_full = sem_open("/Sf", O_CREAT, 0600, 1);

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
		float delay = ((float)rand() / (float)(RAND_MAX) + 0.5) * SLEEP;
		sem_wait(semaphore_producer);

		sem_wait(semaphore_empty);
		prod_index = empty_memory_segment[info_memory_segment[2]];
		info_memory_segment[2] = (info_memory_segment[2] + 1) % N;
		sem_post(semaphore_empty);

		printf("Production: buffer[%d] = %d\n", prod_index, value);
		buffer_memory_segment[prod_index] = value++;
		sleep(delay);

		sem_wait(semaphore_full);
		full_memory_segment[info_memory_segment[3]] = prod_index;
		info_memory_segment[3] = (info_memory_segment[3] + 1) % N;
		sem_post(semaphore_full);

		sem_post(semaphore_consumer);
	}

	return 0;
}