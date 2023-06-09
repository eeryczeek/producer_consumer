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

	// check if memory segments are opened successfully
	if (buffer_memory_segment == MAP_FAILED || empty_memory_segment == MAP_FAILED || full_memory_segment == MAP_FAILED || info_memory_segment == MAP_FAILED)
	{
		perror("mmap");
		exit(1);
	}

	// initialize info to match the state of the memory segments
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
	sem_t *producer_semaphore = sem_open("/Sp", O_CREAT, 0600, N);
	sem_t *consumer_semaphore = sem_open("/Sr", O_CREAT, 0600, 0);
	sem_t *empty_semaphore = sem_open("/Se", O_CREAT, 0600, 1);
	sem_t *full_semaphore = sem_open("/Sf", O_CREAT, 0600, 1);

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
		float speed = ((float)rand() / (float)(RAND_MAX) + 1) * SLEEP;

		sem_wait(consumer_semaphore);

		sem_wait(full_semaphore);
		cons_index = full_memory_segment[info_memory_segment[4]];
		info_memory_segment[4] = (info_memory_segment[4] + 1) % N;
		sem_post(full_semaphore);

		printf("Reading: %d from buffer[%d]\n", buffer_memory_segment[cons_index], cons_index);
		buffer_memory_segment[cons_index] = value++;
		sleep(speed);

		sem_wait(empty_semaphore);
		full_memory_segment[info_memory_segment[1]] = cons_index;
		info_memory_segment[1] = (info_memory_segment[1] + 1) % N;
		sem_post(empty_semaphore);

		sem_post(producer_semaphore);
	}

	return 0;
}